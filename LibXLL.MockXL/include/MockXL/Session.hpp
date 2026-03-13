/**
 * @file Session.hpp
 * @brief Scoped RAII owner of a loaded XLL add-in and its MockXL execution context.
 *
 * A `Session` is the sole public entry point for client code that wants to load
 * an XLL and call its registered functions under MockXL.  Creating a `Session`
 * performs the full Excel add-in lifecycle:
 *
 * 1. Load the XLL shared library (`.xll` / `.dll` / `.so`) via Boost.DLL.
 * 2. Resolve and cache `xlAutoFree12` so the add-in can release DLL-allocated
 *    return values.
 * 3. Configure `impl::Excel12Server` with the XLL name, a free callback, and a
 *    symbol resolver for `xlfRegister` calls.
 * 4. Inject the `Excel12Server::dispatch` callback into the XLL via
 *    `SetExcel12EntryPt`, so every `Excel12` / `Excel12v` call made from inside
 *    the XLL is routed to the mock server.
 * 5. Call `xlAutoOpen` exactly as Excel would on add-in load.
 *
 * Destroying the `Session` calls `xlAutoClose` and unloads the library.
 *
 * Only one `Session` may exist at a time.  Attempting to construct a second one
 * while the first is alive throws `std::runtime_error`.
 *
 * ### Expected XLL function signature
 * MockXL dispatches calls through a uniform function-pointer type of the form:
 * @code
 * xll::Any* fn(const xll::Any*, const xll::Any*, ...);
 * @endcode
 * XLL-exported functions **must** therefore be declared with `xll::Any*` as
 * both the return type and every parameter type:
 * @code
 * XLL_FUNCTION xll::Any* XLLAPI MyFunction(const xll::Any* arg1, const xll::Any* arg2);
 * @endcode
 * Using a more specific XLOPER12-derived type (e.g. `xll::Number*`,
 * `xll::String const*`) instead of `xll::Any*` is **binary-compatible** on all
 * supported platforms — every xll type inherits from `XLOPER12` and adds no
 * data members — but constitutes undefined behaviour under the C++ type system,
 * because the function is called through a pointer to a different (though
 * layout-compatible) type.  Undefined Behaviour Sanitizer (`-fsanitize=function`)
 * will report a warning for every such call.  To keep UBSan clean, always use
 * `xll::Any*` in the exported signature and cast to the specific type internally.
 *
 * ### Platform notes
 * - **Windows** – `SetDllDirectoryA` is called before loading so that MinGW
 *   runtime dependencies can be found next to the XLL.
 * - **Linux** – The XLL is loaded with `RTLD_GLOBAL` so that the hidden
 *   `pexcel12` variable defined in `unix/xlcall_cpp.h` binds to the
 *   `SetExcel12EntryPt` call made in step 4.
 *
 * @see impl::Excel12Server
 * @see Registration
 */
#pragma once

#include "Excel12Server.hpp"
#include "fxt/monads/Tap.hpp"
#include "fxt/monads/Tee.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <string>
#include <fxt/monads/Expected.hpp>
#include <fxt/utils/Failure.hpp>
#include <fixed_string.hpp>
#include <iostream>
#include <xlcall.hpp>

#include <boost/dll/shared_library.hpp>
#include <boost/dll/shared_library_load_mode.hpp>
#include <boost/dll/import.hpp>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

namespace MockXL
{

    namespace fs = std::filesystem;

    /**
     * @brief Scoped owner of one loaded XLL add-in.
     *
     * ## Lifetime
     * Constructing a `Session` drives the full Excel add-in load sequence
     * (`xlAutoOpen`).  The destructor drives `xlAutoClose` and releases the
     * shared library.  All intermediate state (registered functions, the
     * `Excel12Server` singleton, the auto-free callback) is owned by this object
     * for its lifetime.
     *
     * ## Thread safety
     * `Session` is **not** thread-safe.  Only one `Session` may exist at a time;
     * the constructor enforces this with an atomic instance counter.
     *
     * ## Required XLL function signature
     * MockXL dispatches every registered function through a uniform
     * function-pointer type whose parameters and return type are all
     * `xll::Any*`.  XLL-exported functions **must** therefore match that
     * signature exactly:
     * @code
     * XLL_FUNCTION xll::Any* XLLAPI MyFunction(const xll::Any* a, const xll::Any* b);
     * @endcode
     * Declaring parameters or the return type as a more specific
     * XLOPER12-derived type (e.g. `xll::Number*`, `xll::String const*`) is
     * binary-compatible but is technically undefined behaviour: the function
     * will be called through a pointer to a different C++ type.
     * Undefined Behaviour Sanitizer (`-fsanitize=function`) will emit a warning
     * for every such call.  Cast to the specific type *inside* the function body
     * to keep UBSan clean.
     *
     * ## Calling registered functions
     * After construction, functions registered by `xlAutoOpen` can be called via
     * the templated `call()` overloads.  Arguments must be derived from `XLOPER12`
     * (e.g. `xll::Number`, `xll::String`, `xll::Bool`).  The return value is an
     * owning `xll::Any`.
     *
     * @code
     * MockXL::Session session{ "path/to/addin.xll" };
     * xll::Number a{ 3.0 }, b{ 4.0 };
     * xll::Any result = session.call<"ADD.NUMBERS">(a, b);
     * @endcode
     */
    class Session
    {
        // ----------------------------------------------------------------
        // Private nested loader — implementation detail, not part of the
        // public API.
        // ----------------------------------------------------------------

        /**
         * @brief RAII wrapper around a Boost.DLL shared library handle.
         *
         * `AddInLoader` is a private implementation detail of `Session`.  It
         * encapsulates everything needed to load an XLL, resolve exported symbols
         * by name, and look up raw function pointers for the `xlfRegister` proc
         * resolver.
         *
         * The class is non-copyable and non-movable; ownership is tied to the
         * enclosing `Session` object.
         */
        class AddInLoader
        {
            boost::dll::shared_library m_lib;  ///< Boost.DLL handle owning the loaded shared library.
            fs::path                   m_path; ///< Absolute path to the XLL, resolved at construction.

        public:
            /**
             * @brief Loads the XLL at @p path.
             *
             * On Windows, `SetDllDirectoryA` is called with the XLL's parent
             * directory before loading, so MinGW runtime dependencies
             * (`libstdc++`, `libgcc`, etc.) are found automatically.
             *
             * On all platforms the library is loaded with `RTLD_NOW |
             * RTLD_GLOBAL` (Boost.DLL translates these to the correct OS flags).
             * `RTLD_GLOBAL` is required on Linux so that the hidden `pexcel12`
             * symbol in the XLL can be written by the `SetExcel12EntryPt` call
             * made immediately after loading.
             *
             * @param path Path to the XLL file.  Relative paths are resolved to
             *             an absolute path via `std::filesystem::absolute`.
             * @throws boost::system::system_error if the library cannot be loaded.
             */
            explicit AddInLoader(const fs::path& path) : m_path(fs::absolute(path))
            {
#ifdef _WIN32
                SetDllDirectoryA(m_path.parent_path().string().c_str());
#endif
                m_lib.load(boost::dll::fs::path{ m_path.string() },
                           boost::dll::load_mode::rtld_now |
                           boost::dll::load_mode::rtld_global);
            }

            /// @cond – compiler-generated special members are suppressed from docs.
            ~AddInLoader() = default;
            AddInLoader(const AddInLoader&)            = delete;
            AddInLoader& operator=(const AddInLoader&) = delete;
            AddInLoader(AddInLoader&&)                 = delete;
            AddInLoader& operator=(AddInLoader&&)      = delete;
            /// @endcond

            /**
             * @brief Resolves a named export and wraps it in a `std::function`.
             *
             * The function-pointer type is deduced from @p FnPtr: if `FnPtr` is
             * `void(*)(int)` then the returned `std::function` has signature
             * `void(int)`.
             *
             * @tparam FnPtr  Function-pointer type whose pointee signature matches
             *                the exported symbol.
             * @param  name   Null-terminated symbol name to look up.
             * @return `fxt::expected` containing the wrapped function on success,
             *         or an `fxt::failure` with a human-readable message if the
             *         symbol is not found.
             */
            template<typename FnPtr>
            auto resolve(const char* name) const
                -> fxt::expected<std::function<std::remove_pointer_t<FnPtr>>, fxt::failure>
            {
                using Sig = std::remove_pointer_t<FnPtr>;
                if (!m_lib.has(name))
                    return fxt::unexpected(fxt::failure{
                        std::string("Symbol not found in ") +
                        m_path.filename().string() + ": " + name });

                auto imported = boost::dll::import_symbol<Sig>(m_lib, name);
                return std::function<Sig>{ [imported]<typename... TArgs>(TArgs&&... args) {
                    return imported(std::forward<TArgs>(args)...);
                }};
            }

            /**
             * @brief Returns the raw function pointer for @p name, or `nullptr`.
             *
             * Unlike `resolve()`, no `std::function` wrapper is created.  Used by
             * the `xlfRegister` proc resolver in `Excel12Server` to cache typed
             * function pointers for later dispatch.
             *
             * @param name Symbol name to look up.
             * @return Raw `void*` pointer to the symbol, or `nullptr` if not found.
             */
            [[nodiscard]] void* resolve_raw(const std::string& name) const noexcept
            {
                if (!m_lib.has(name))
                    return nullptr;
#ifdef _WIN32
                return reinterpret_cast<void*>(
                    ::GetProcAddress(m_lib.native(), name.c_str()));
#else
                return ::dlsym(m_lib.native(), name.c_str());
#endif
            }

            /**
             * @brief Returns the absolute path to the loaded XLL.
             */
            [[nodiscard]] const fs::path& path() const noexcept { return m_path; }
        };

        // ----------------------------------------------------------------
        // Session state
        // ----------------------------------------------------------------

        using xlAutoOpen  = decltype(+[]{ return 0; });           ///< Function-pointer type for `xlAutoOpen`  (returns `int`, no parameters).
        using xlAutoClose = decltype(+[]{ return 0; });           ///< Function-pointer type for `xlAutoClose` (returns `int`, no parameters).
        using xlAutoFree  = decltype(+[](const xll::Any*){ });    ///< Function-pointer type for `xlAutoFree12` (returns `void`, takes `const XLOPER12*`).

        inline static std::atomic<int> s_instance_count { 0 };   ///< Global instance counter; enforces the "at most one Session" invariant.

        AddInLoader m_loader;        ///< Owns the shared-library handle and symbol resolution.
        FAutoFree   m_xlAutoFree {}; ///< Cached `xlAutoFree12` callback, or an empty function if not exported.

    public:
        /**
         * @brief Loads the XLL and drives `xlAutoOpen`.
         *
         * Performs the full add-in initialisation sequence:
         * 1. Enforces the single-instance invariant (throws if another `Session`
         *    already exists).
         * 2. Loads the shared library via `AddInLoader`.
         * 3. Resolves `xlAutoFree12` and stores it in `m_xlAutoFree`.
         * 4. Configures `impl::Excel12Server` with the free callback, XLL name,
         *    and a proc resolver lambda for `xlfRegister`.
         * 5. Calls the XLL's exported `SetExcel12EntryPt` to inject the
         *    `Excel12Server::dispatch` callback.  This is the mechanism by which
         *    `Excel12` / `Excel12v` calls from inside the XLL reach MockXL on
         *    both Windows and Linux.
         * 6. Calls `xlAutoOpen`; throws `std::runtime_error` if the symbol is
         *    not exported.
         *
         * @param xllPath Path to the `.xll` / `.dll` / `.so` file.
         * @throws std::runtime_error if a `Session` already exists, if the XLL
         *         cannot be loaded, or if `xlAutoOpen` is not found.
         */
        explicit Session(const fs::path& xllPath) : m_loader(xllPath)
        {
            if (s_instance_count.fetch_add(1) != 0) {
                s_instance_count.fetch_sub(1);
                throw std::runtime_error("Only one MockXL::Session may exist at a time");
            }
            // resolve returns fxt::expected; value_or({}) gives an empty
            // std::function if xlAutoFree12 is not exported.
            m_xlAutoFree = m_loader.resolve<xlAutoFree>("xlAutoFree12")
                            .value_or(FAutoFree{});

            // Inject the free callback into the server so Function::operator()
            // can call xlAutoFree12 after deep-copying DLL-allocated return values.
            impl::Excel12Server::instance().set_auto_free(m_xlAutoFree);

            // Tell the mock server the XLL's path so xlGetName returns the right value.
            impl::Excel12Server::instance().set_xll_name(xllPath.stem().string() + ".xll");

            // Supply a proc resolver so xlfRegister can cache raw function pointers.
            // This enables call<"EXCEL.NAME">(args...) after xlAutoOpen.
            impl::Excel12Server::instance().set_proc_resolver(
                [this](const std::string& name) { return m_loader.resolve_raw(name); });

            // Inject the dispatch callback into the XLL via SetExcel12EntryPt.
            // On Windows this is the primary (and only) mechanism.
            // On Linux, xlcall_cpp.h now provides a real implementation that stores
            // the pointer in a per-XLL hidden inline variable, so Excel12/Excel12v
            // inside the XLL forward to Excel12Server without any host-side symbols.
            using SetEntryPt = void (*)(EXCEL12PROC);
            if (const auto setter = m_loader.resolve<SetEntryPt>("SetExcel12EntryPt"); setter.has_value()) {
                (*setter)([](const int xlfn, const int coper, LPXLOPER12* rgpxloper12, LPXLOPER12 res) -> int { // NOLINT
                    return MockXL::impl::Excel12Server::instance().dispatch(xlfn, coper, rgpxloper12, res);
                });
            }

            // Drive xlAutoOpen exactly as Excel does on add-in load.
            m_loader.resolve<xlAutoOpen>("xlAutoOpen")
                | fxt::tee([](const auto& fn) { fn(); })
                | fxt::tap_error([](const fxt::failure& err) { throw std::runtime_error{ err.message() }; });
        }

        /**
         * @brief Calls `xlAutoClose` and unloads the XLL.
         *
         * Drives `xlAutoClose` exactly as Excel does when an add-in is unloaded.
         * If the symbol is not exported, the error is printed to `std::cerr` and
         * destruction continues.  The instance counter is decremented so a new
         * `Session` may be created afterwards.
         */
        ~Session()
        {
            s_instance_count.fetch_sub(1);

            // Drive xlAutoClose exactly as Excel does on add-in unload.
            m_loader.resolve<xlAutoClose>("xlAutoClose")
                | fxt::tee([](const auto& fn) { fn(); })
                | fxt::tap_error([](const fxt::failure& err) { std::cerr << err.message() << std::endl; });
        }

        /// @cond
        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;
        Session(Session&&)                 = delete;
        Session& operator=(Session&&)      = delete;
        /// @endcond

        /**
         * @brief Returns the absolute path to the loaded XLL.
         */
        [[nodiscard]] const fs::path& path() const noexcept { return m_loader.path(); }

        /**
         * @brief Registers a custom handler for an Excel function/command code.
         *
         * Forwards to `impl::Excel12Server::register_handler`.  The handler is
         * invoked by `dispatch()` for any `xlfn` not handled internally
         * (`xlFree`, `xlGetName`, `xlfRegister`, `xlCoerce`).  Registering a
         * handler for a code that already has one silently replaces it.
         *
         * @param xlfn    The Excel function/command code (e.g. `xlcAlert`).
         * @param handler Callable matching `impl::Excel12Server::HandlerFn`.
         */
        void register_handler(int xlfn, impl::Excel12Server::HandlerFn handler) // NOLINT
        {
            impl::Excel12Server::instance().register_handler(xlfn, std::move(handler));
        }

        /**
         * @brief Removes the custom handler for @p xlfn, if one exists.
         *
         * After this call `dispatch()` reverts to the silent-default behaviour
         * for that code (`xlretSuccess` with `xll::Nil`).
         *
         * @param xlfn The Excel function/command code whose handler to remove.
         */
        void unregister_handler(int xlfn) // NOLINT
        {
            impl::Excel12Server::instance().unregister_handler(xlfn);
        }

        /**
         * @brief Calls a registered XLL function by its compile-time Excel name.
         *
         * The Excel name is baked in as a non-type template parameter and
         * resolved at compile time to a `std::string_view`, avoiding a runtime
         * string lookup.  Each argument is wrapped in an `xll::Any` (deep copy)
         * and forwarded to `impl::Excel12Server::call_by_excel_name`.
         *
         * @tparam Name      Compile-time Excel function name, e.g. `"ADD.NUMBERS"`.
         * @tparam Args      Argument types; each must be derived from `XLOPER12`.
         * @param  args      Arguments to pass to the XLL function.
         * @return An owning `xll::Any` containing the return value, or `xll::Nil`
         *         if the function name is not registered or has no resolved symbol.
         *
         * @code
         * xll::Number a{ 3.0 }, b{ 4.0 };
         * xll::Any result = session.call<"ADD.NUMBERS">(a, b);
         * @endcode
         */
        template<fixstr::fixed_string Name, typename... Args>
            requires(std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
        [[nodiscard]] xll::Any call(Args&&... args) const // NOLINT
        {
            return impl::Excel12Server::instance().call_by_excel_name(
                std::string_view(Name.data(), Name.size()),
                std::vector<xll::Any>{ xll::Any(args)... });
        }

        /**
         * @brief Calls a registered XLL function by its runtime Excel name.
         *
         * Identical to the compile-time overload except that the Excel name is
         * supplied as a `std::string_view` at runtime.  Use this overload when
         * the function name is not known at compile time.
         *
         * @tparam Args      Argument types; each must be derived from `XLOPER12`.
         * @param  excelName Excel-visible name under which the function was
         *                   registered (case-sensitive).
         * @param  args      Arguments to pass to the XLL function.
         * @return An owning `xll::Any` containing the return value, or `xll::Nil`
         *         if the function name is not registered or has no resolved symbol.
         *
         * @code
         * xll::Number a{ 3.0 }, b{ 4.0 };
         * xll::Any result = session.call("ADD.NUMBERS", a, b);
         * @endcode
         */
        template<typename... Args>
            requires(std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
        [[nodiscard]] xll::Any call(std::string_view excelName, Args&&... args) const //NOLINT
        {
            return impl::Excel12Server::instance().call_by_excel_name(
                excelName,
                std::vector<xll::Any>{ xll::Any(args)... });
        }
    };

}    // namespace MockXL
