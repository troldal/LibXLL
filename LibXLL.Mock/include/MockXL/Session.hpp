#pragma once

#include "Excel12Server.hpp"
#include "XloperResult.hpp"
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

    class Session
    {
        // ----------------------------------------------------------------
        // Private nested loader — implementation detail, not part of the
        // public API.
        // ----------------------------------------------------------------
        class AddInLoader
        {
            boost::dll::shared_library m_lib;
            fs::path                   m_path;

        public:
            explicit AddInLoader(const fs::path& path) : m_path(fs::absolute(path))
            {
#ifdef _WIN32
                SetDllDirectoryA(m_path.parent_path().string().c_str());
#endif
                m_lib.load(boost::dll::fs::path{ m_path.string() },
                           boost::dll::load_mode::rtld_now |
                           boost::dll::load_mode::rtld_global);
            }

            ~AddInLoader() = default;

            AddInLoader(const AddInLoader&)            = delete;
            AddInLoader& operator=(const AddInLoader&) = delete;
            AddInLoader(AddInLoader&&)                 = delete;
            AddInLoader& operator=(AddInLoader&&)      = delete;

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

            [[nodiscard]] void* resolve_raw(const std::string& name) const noexcept
            {
                if (!m_lib.has(name))
                    return nullptr;
#ifdef _WIN32
                return reinterpret_cast<void*>(
                    ::GetProcAddress(static_cast<HMODULE>(m_lib.native()), name.c_str()));
#else
                return ::dlsym(m_lib.native(), name.c_str());
#endif
            }

            [[nodiscard]] const fs::path& path() const noexcept { return m_path; }
        };

        // ----------------------------------------------------------------
        // Session state
        // ----------------------------------------------------------------

        using xlAutoOpen  = decltype(+[]{ return 0; });
        using xlAutoClose = decltype(+[]{ return 0; });
        using xlAutoFree  = decltype(+[](const XLOPER12*){ });

        inline static std::atomic<int> s_instance_count { 0 };

        AddInLoader m_loader;
        FAutoFree   m_xlAutoFree {};

    public:
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

            // Tell the mock server the XLL's path so xlGetName returns the right value.
            impl::Excel12Server::instance().set_xll_name(xllPath.stem().string() + ".xll");

            // Supply a proc resolver so xlfRegister can cache raw function pointers.
            // This enables call<"EXCEL.NAME">(args...) after xlAutoOpen.
            impl::Excel12Server::instance().set_proc_resolver(
                [this](const std::string& name) { return m_loader.resolve_raw(name); });

#ifdef _WIN32
            // On Windows xlcall_cpp.h uses GetProcAddress(GetModuleHandle(NULL),
            // "MdCallBack12") to find the callback.  The add-in also exports
            // SetExcel12EntryPt which lets a non-Excel host inject the pointer
            // directly — use it here as a belt-and-suspenders measure.
            using SetEntryPt = void (*)(EXCEL12PROC);
            if (auto setter = m_loader.resolve<SetEntryPt>("SetExcel12EntryPt"); setter.has_value()) {
                (*setter)([](int xlfn, int coper, LPXLOPER12* rgpxloper12, LPXLOPER12 res) -> int {
                    return MockXL::impl::Excel12Server::instance().dispatch(xlfn, coper, rgpxloper12, res);
                });
            }
#endif

            // Drive xlAutoOpen exactly as Excel does on add-in load.
            m_loader.resolve<xlAutoOpen>("xlAutoOpen")
                | fxt::tee([](const auto& fn) { fn(); })
                | fxt::tap_error([](const fxt::failure& err) { throw std::runtime_error{ err.message() }; });
        }

        /**
         * @brief Calls xlAutoClose and unloads the XLL.
         */
        ~Session()
        {
            s_instance_count.fetch_sub(1);

            // Drive xlAutoClose exactly as Excel does on add-in unload.
            m_loader.resolve<xlAutoClose>("xlAutoClose")
                | fxt::tee([](const auto& fn) { fn(); })
                | fxt::tap_error([](const fxt::failure& err) { std::cerr << err.message() << std::endl; });
        }

        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;
        Session(Session&&)                 = delete;
        Session& operator=(Session&&)      = delete;

        [[nodiscard]] const fs::path& path() const noexcept { return m_loader.path(); }

        /**
         * @brief Resolves exportName, calls it with args, and returns the result.
         *
         * Returns an fxt::expected containing the XloperResult on success, or an
         * fxt::failure if the symbol could not be resolved.  The caller decides
         * whether to treat a missing export as fatal (.value()) or optional
         * (.value_or(XloperResult{})).
         *
         * Each argument must be of a type derived from XLOPER12 (e.g. xll::Number,
         * xll::String).  The cast to const XLOPER12* is performed internally.
         */
        template<typename FnPtr, typename... Args>
            requires(std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
        auto call(const char* exportName, Args&&... args) const -> fxt::expected<XloperResult, fxt::failure>
        {
            auto fn = m_loader.resolve<FnPtr>(exportName);
            if (!fn.has_value())
                return fxt::unexpected(fn.error());
            auto* raw = (*fn)(static_cast<const XLOPER12*>(&args)...);
            return XloperResult { raw, m_xlAutoFree };
        }

        /**
         * @brief Calls a registered XLL function by its compile-time Excel name.
         *
         * The name is baked in as a non-type template parameter, e.g.:
         * @code
         * xll::Number a{3.0}, b{4.0};
         * auto result = session.call<"ADD.NUMBERS">(a, b);
         * @endcode
         *
         * Each argument must be derived from XLOPER12.  Returns xll::Any.
         */
        template<fixstr::fixed_string Name, typename... Args>
            requires(std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
        [[nodiscard]] xll::Any call(Args&&... args) const
        {
            // Build a stack-local array of const XLOPER12* from the argument pack.
            std::array<const XLOPER12*, sizeof...(Args)> ptrs{
                static_cast<const XLOPER12*>(&args)...
            };
            return impl::Excel12Server::instance().call_by_excel_name(
                std::string_view(Name.data(), Name.size()),
                ptrs.data(),
                static_cast<int>(ptrs.size()));
        }

        /**
         * @brief Calls a registered XLL function by its runtime Excel name.
         *
         * @code
         * auto result = session.call("ADD.NUMBERS", a, b);
         * @endcode
         */
        template<typename... Args>
            requires(std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
        [[nodiscard]] xll::Any call(std::string_view excelName, Args&&... args) const
        {
            std::array<const XLOPER12*, sizeof...(Args)> ptrs{
                static_cast<const XLOPER12*>(&args)...
            };
            return impl::Excel12Server::instance().call_by_excel_name(
                excelName,
                ptrs.data(),
                static_cast<int>(ptrs.size()));
        }
    };

}    // namespace MockXL
