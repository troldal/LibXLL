/**
 * @file Excel12Server.hpp
 * @brief Singleton dispatch router for `Excel12` / `Excel12v` callbacks in mock-Excel testing.
 *
 * When an XLL add-in calls `Excel12()` or `Excel12v()`, the call is routed
 * through `xlcall_cpp.h` to a stored callback pointer installed by
 * `SetExcel12EntryPt`.  `Excel12Server::dispatch()` is that callback.
 *
 * ### Dispatch order
 * 1. **Built-in codes** — `xlFree`, `xlGetName`, `xlfRegister`, `xlCoerce` are
 *    handled directly and cannot be overridden.
 * 2. **Custom handlers** — any other `xlfn` registered via `register_handler()`.
 * 3. **Silent default** — unrecognised codes succeed with an `xll::Nil` result.
 *
 * ### Typical usage
 * `Excel12Server` is accessed exclusively through `MockXL::Session`, which
 * configures it (name, free callback, proc resolver, entry-point injection) and
 * drives `xlAutoOpen` / `xlAutoClose` at the right moments.  Direct use is
 * only needed when registering custom handlers before the add-in is loaded:
 *
 * @code
 * MockXL::Session session{ "addin.xll" };
 * session.register_handler(xlcAlert,
 *     [](const std::vector<xll::Any>& args, xll::Any&) -> int {
 *         std::cout << "Alert!\n";
 *         return xlretSuccess;
 *     });
 * @endcode
 *
 * @see MockXL::Session
 * @see Registration
 */
#pragma once

#include "Function.hpp"
#include "Registration.hpp"
#include "Types/Any.hpp"
#include "Types/Nil.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <xlcall.hpp>

// Bring in the PASCAL / pascal calling-convention macro where needed.
#ifdef _WIN32
#  ifndef PASCAL
#    define PASCAL __stdcall
#  endif
#else
#  ifndef PASCAL
#    define PASCAL
#  endif
#endif

namespace MockXL::impl {


// ============================================================================
// Excel12Server
// ============================================================================

/**
 * @brief Singleton that implements the `MdCallBack12` / `Excel12` / `Excel12v`
 *        dispatch for mock-Excel testing.
 *
 * `Excel12Server` receives every `Excel12` / `Excel12v` call made from inside
 * a loaded XLL and routes it to one of three tiers:
 *
 * 1. **Built-in handlers** (`xlFree`, `xlGetName`, `xlfRegister`, `xlCoerce`) —
 *    handled directly; cannot be overridden.
 * 2. **Custom handlers** — any `xlfn` registered via `register_handler()`.
 *    The handler receives arguments as `const std::vector<xll::Any>&` and a
 *    writable `xll::Any&` result, and returns an `xlret` code.
 * 3. **Silent default** — if no handler is found, the call succeeds with
 *    `xll::Nil`.
 *
 * The singleton is accessed via `instance()`.  Only `MockXL::Session`
 * constructs and configures it; client code should prefer the forwarding
 * methods on `Session` over calling `instance()` directly.
 *
 * @note `Excel12Server` is not thread-safe.  All calls must be serialised by
 *       the caller (which `Session` guarantees through its single-instance
 *       invariant).
 *
 * @see MockXL::Session
 * @see Registration
 */
class Excel12Server
{
public:
    /// @cond — non-copyable, non-movable singleton.
    Excel12Server(const Excel12Server&)            = delete;
    Excel12Server& operator=(const Excel12Server&) = delete;
    /// @endcond

    // ------------------------------------------------------------------
    // Singleton access
    // ------------------------------------------------------------------

    /**
     * @brief Returns the process-wide singleton instance.
     *
     * The instance is constructed on first call (Meyers singleton) and
     * destroyed at program exit.  It must outlive all XLL code that uses
     * `Excel12` / `Excel12v`.
     */
    [[nodiscard]] static Excel12Server& instance() noexcept
    {
        static Excel12Server s;
        return s;
    }

    // ------------------------------------------------------------------
    // Configuration — called by Session during add-in load
    // ------------------------------------------------------------------

    /**
     * @brief Sets the path string returned to `xlGetName` callers.
     *
     * `Session` calls this immediately after loading the XLL, passing the
     * XLL's stem name with `.xll` appended.  Must be called before
     * `xlAutoOpen` is driven.
     *
     * @param name The filename (or full path) to report as the add-in name.
     */
    void set_xll_name(const std::string& name) { m_xll_name = xll::String(name); }

    /**
     * @brief Returns the currently configured XLL name.
     */
    [[nodiscard]] const xll::String& xll_name() const noexcept { return m_xll_name; }

    /**
     * @brief Supplies the `xlAutoFree12` callback.
     *
     * `Function::operator()` calls this after deep-copying a DLL-allocated
     * return value into an owning `xll::Any`, so the add-in can release its
     * memory.  `Session` calls this immediately after resolving
     * `xlAutoFree12`; if the symbol is not exported, an empty `FAutoFree` is
     * stored and no freeing takes place.
     *
     * @param autoFree The `xlAutoFree12` callback, or an empty `std::function`
     *                 if the XLL does not export it.
     */
    void set_auto_free(FAutoFree autoFree) noexcept
    {
        m_auto_free = std::move(autoFree);
    }

    // ------------------------------------------------------------------
    // Registration records
    // ------------------------------------------------------------------

    /**
     * @brief Returns a read-only view of all function registrations.
     *
     * The map is populated during `xlAutoOpen` as the add-in calls
     * `xlfRegister` for each of its functions.  The key is the Excel-visible
     * function name (e.g. `"ADD.NUMBERS"`).
     *
     * @return Const reference to the internal registration map.
     */
    [[nodiscard]] const std::unordered_map<std::string, Registration>&
    registrations() const noexcept
    {
        return m_registrations;
    }

    /**
     * @brief Removes all registration records.
     *
     * Useful when reloading an add-in within the same process (e.g. in a
     * test fixture that creates multiple `Session` objects sequentially).
     */
    void clear_registrations() { m_registrations.clear(); }

    // ------------------------------------------------------------------
    // Proc resolver — injected by Session after the XLL is loaded
    // ------------------------------------------------------------------

    /**
     * @brief Supplies a callback that maps a C++ procedure name to a raw
     *        function pointer.
     *
     * `Session` calls this immediately after loading the XLL so that
     * `xlfRegister` handling can resolve each exported symbol and cache a
     * typed `Function` in the corresponding `Registration` object.
     *
     * @param resolver Callable that accepts a symbol name and returns the
     *                 corresponding raw function pointer, or `nullptr` if the
     *                 symbol is not found.
     */
    void set_proc_resolver(std::function<void*(const std::string&)> resolver) noexcept
    {
        m_proc_resolver = std::move(resolver);
    }

    // ------------------------------------------------------------------
    // Custom handler registration
    // ------------------------------------------------------------------

    /**
     * @brief Callable type for a custom `Excel12` handler.
     *
     * A `HandlerFn` is invoked by `dispatch()` whenever the add-in calls
     * `Excel12(xlfn, ...)` and `xlfn` is not one of the built-in codes.
     *
     * @param args   Arguments passed by the add-in, converted to `xll::Any`
     *               values.  Passed by `const` reference; the handler must
     *               not store pointers into the vector beyond the call.
     * @param result Reference to the result, pre-initialised to `xll::Nil`.
     *               Write the return value here.
     * @return An `xlret` code: `xlretSuccess`, `xlretFailed`,
     *         `xlretAbort`, etc.
     */
    using HandlerFn = std::function<int(const std::vector<xll::Any>& args,
                                        xll::Any& result)>;

    /**
     * @brief Registers a custom handler for an Excel function/command code.
     *
     * The handler is invoked by `dispatch()` for any `xlfn` not handled
     * internally.  Registering a handler for a code that already has one
     * silently replaces it.
     *
     * Built-in codes (`xlFree`, `xlGetName`, `xlfRegister`, `xlCoerce`) are
     * always handled internally and **cannot** be overridden via this map.
     *
     * @param xlfn    The Excel function/command code to handle
     *                (e.g. `xlcAlert`, `xlcMessage`).
     * @param handler Callable matching `HandlerFn`.
     *
     * @code
     * server.register_handler(xlcAlert,
     *     [](const std::vector<xll::Any>& args, xll::Any&) -> int {
     *         std::cout << "Alert!\n";
     *         return xlretSuccess;
     *     });
     * @endcode
     */
    void register_handler(const int xlfn, HandlerFn handler)
    {
        m_handlers.insert_or_assign(xlfn, std::move(handler));
    }

    /**
     * @brief Removes the custom handler for @p xlfn, if one exists.
     *
     * After this call, `dispatch()` reverts to the silent-default behaviour
     * for that code (`xlretSuccess` with `xll::Nil`).  If no handler was
     * registered for @p xlfn, this is a no-op.
     *
     * @param xlfn The Excel function/command code whose handler to remove.
     */
    void unregister_handler(const int xlfn)
    {
        m_handlers.erase(xlfn);
    }

    // ------------------------------------------------------------------
    // Call a registered function by its Excel name
    // ------------------------------------------------------------------

    /**
     * @brief Invokes a registered XLL function by its Excel-visible name.
     *
     * Looks up @p excel_name in the registration map and calls the resolved
     * function with @p xlargs.  Each element is forwarded as an `XLOPER12`
     * pointer to the underlying typed function pointer (safe because
     * `xll::Any` inherits from `XLOPER12` with no added members).  Up to
     * `MaxXllArity` arguments are used; any beyond that limit are silently
     * ignored.
     *
     * @param excel_name  The Excel-visible name the function was registered
     *                    under (case-sensitive, e.g. `"ADD.NUMBERS"`).
     * @param xlargs      Arguments to pass to the function.
     * @return An owning `xll::Any` containing the function's return value, or
     *         `xll::Nil` if the name is not registered or has no resolved
     *         function pointer.
     */
    [[nodiscard]] xll::Any call_by_excel_name(const std::string_view excel_name,
                                               const std::vector<xll::Any>& xlargs) const
    {
        return call_impl(excel_name, xlargs);
    }

    // ------------------------------------------------------------------
    // Core dispatch — called by MdCallBack12 / Excel12 / Excel12v
    // ------------------------------------------------------------------

    /**
     * @brief Routes one `Excel12` call to the appropriate handler.
     *
     * This is the function installed as the `EXCEL12PROC` callback via
     * `SetExcel12EntryPt`.  Every `Excel12` / `Excel12v` call made from
     * inside the loaded XLL reaches this method.
     *
     * Dispatch order:
     * 1. The result is pre-initialised to `xll::Nil`.
     * 2. Built-in codes are handled in a `switch` and return immediately.
     * 3. The custom handler map is checked; if a handler is found it is
     *    called with the arguments converted to `std::vector<xll::Any>`.
     * 4. If no handler matches, `xlretSuccess` is returned with the result
     *    left as `xll::Nil`.
     *
     * @param xlfn         Excel function/command code
     *                     (e.g. `xlFree`, `xlfRegister`, `xlcAlert`).
     * @param coper        Number of valid pointers in @p rgpxloper12.
     * @param rgpxloper12  Array of `LPXLOPER12` argument pointers.
     * @param xloper12Res  Output `XLOPER12` to populate with the result.
     *                     May be `nullptr`; the function is a no-op on it in
     *                     that case.
     * @return `xlretSuccess` on success; `xlretFailed` if a built-in handler
     *         detects a fatal error (e.g. malformed `xlCoerce` operands).
     *         Custom handlers may return any `xlret` code.
     */
    int dispatch(const int xlfn, const int coper, const LPXLOPER12* rgpxloper12, const LPXLOPER12 xloper12Res)
    {
        // Initialise the result to nil so callers get a well-defined value
        // even for unhandled function codes.
        if (xloper12Res)
            *xloper12Res = xll::Nil{}; // make_nil();

        switch (xlfn) {

            // ----------------------------------------------------------
            // xlFree  — release memory allocated by Excel
            // In mock mode xlbitXLFree is never set (the add-in only uses
            // xlbitDLLFree managed by XloperResult), so this is a no-op.
            // ----------------------------------------------------------
            case xlFree:
                return xlretSuccess;

            // ----------------------------------------------------------
            // xlGetName  — return the XLL module path
            // ----------------------------------------------------------
            case xlGetName:
                if (xloper12Res)
                    *xloper12Res = m_xll_name;
                return xlretSuccess;

            // ----------------------------------------------------------
            // xlfRegister  — record function registration, return ID
            // ----------------------------------------------------------
            case xlfRegister: {
                const auto args = make_args(coper, rgpxloper12);

                Registration reg{ args };

                if (m_proc_resolver) {
                    auto sig = static_cast<std::string>(reg.signature());
                    if (!sig.empty()) sig.erase(0, 1);
                    while (!sig.empty() && sig.back() == '$') sig.pop_back();
                    const int arity = static_cast<int>(sig.size());

                    if (void* fp = m_proc_resolver(static_cast<std::string>(reg.procedure_name()))) reg.set_function(make_function(fp, arity));
                }

                std::cout << "[MockXL] Registered: "
                          << static_cast<std::string>(reg.excel_name())
                          << " -> "
                          << static_cast<std::string>(reg.procedure_name())
                          << '\n';

                const int function_id = reg.function_id();
                m_registrations.emplace(static_cast<std::string>(reg.excel_name()), std::move(reg));

                if (xloper12Res) {
                    xloper12Res->xltype  = xltypeNum;
                    xloper12Res->val.num = static_cast<double>(function_id);
                }
                return xlretSuccess;
            }

            // ----------------------------------------------------------
            // xlCoerce  — basic type coercion (num <-> bool <-> int)
            // ----------------------------------------------------------
            case xlCoerce:
                return handle_coerce(coper, rgpxloper12, xloper12Res);

            // ----------------------------------------------------------
            // Everything else — consult the custom handler map, then
            // succeed silently if no handler is registered.
            // ----------------------------------------------------------
            default: {
                const auto it = m_handlers.find(xlfn);
                if (it == m_handlers.end())
                    return xlretSuccess;

                xll::Any result{ xll::Nil{} };
                const int ret = it->second(make_args(coper, rgpxloper12), result);
                if (xloper12Res)
                    *xloper12Res = static_cast<XLOPER12>(result);
                return ret;
            }
        }
    }

private:
    // ------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------

    xll::String                                   m_xll_name{ "MockXL.xll" };       ///< Reported by `xlGetName`; set by `set_xll_name()`.
    std::unordered_map<std::string, Registration> m_registrations;                   ///< Functions registered via `xlfRegister`, keyed by Excel-visible name.
    std::function<void*(const std::string&)>      m_proc_resolver;                   ///< Resolves a C++ procedure name to a raw function pointer; injected by `Session`.
    FAutoFree                                     m_auto_free;                       ///< `xlAutoFree12` callback; empty if the XLL does not export it.
    std::unordered_map<int, HandlerFn>            m_handlers;                        ///< Custom handlers keyed by Excel function/command code.

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    /**
     * @brief Converts a raw `LPXLOPER12` argument array to
     *        `std::vector<xll::Any>`.
     *
     * Null pointers are skipped; non-null entries are deep-copied into
     * `xll::Any` via `xll::Any(const XLOPER12&)`.  Called by both the
     * `xlfRegister` case and the custom-handler path so the conversion
     * logic is not duplicated.
     *
     * @param coper        Number of entries in @p rgpxloper12.
     * @param rgpxloper12  Raw argument pointer array.
     * @return Vector of `xll::Any` values, one per non-null argument.
     */
    [[nodiscard]] static std::vector<xll::Any> make_args(const int coper, const LPXLOPER12* rgpxloper12)
    {
        std::vector<xll::Any> args;
        args.reserve(static_cast<std::size_t>(coper));
        for (int i = 0; i < coper; ++i)
            if (rgpxloper12[i])
                args.emplace_back(*rgpxloper12[i]);
        return args;
    }

    /**
     * @brief Looks up @p excel_name in the registration map and invokes it.
     *
     * Prints a diagnostic to `std::cerr` and returns `xll::Nil` if the name
     * is not found or the `Registration` has no resolved `Function`.
     *
     * @param excel_name Excel-visible function name.
     * @param xlargs     Arguments forwarded to `Registration::invoke`.
     * @return Owning `xll::Any` result, or `xll::Nil` on lookup failure.
     */
    [[nodiscard]] xll::Any call_impl(const std::string_view excel_name,
                                     const std::vector<xll::Any>& xlargs) const
    {
        const auto it = m_registrations.find(std::string(excel_name));
        if (it == m_registrations.end()) {
            std::cerr << "[MockXL] call: no registration found for '" << excel_name << "'\n";
            return xll::Any{ xll::Nil{} };
        }
        const Registration& reg = it->second;
        if (!reg) {
            std::cerr << "[MockXL] call: no resolved entry for '" << excel_name << "'\n";
            return xll::Any{ xll::Nil{} };
        }

        return reg.invoke(xlargs, m_auto_free);
    }

    /**
     * @brief Implements `xlCoerce`: converts `args[0]` to the type
     *        specified in `args[1]`.
     *
     * Supports identity coercion and the three numeric scalar conversions:
     *
     * | Source       | Destination  |
     * |--------------|--------------|
     * | `xltypeBool` | `xltypeNum`  |
     * | `xltypeInt`  | `xltypeNum`  |
     * | `xltypeNum`  | `xltypeBool` |
     * | `xltypeInt`  | `xltypeBool` |
     * | `xltypeNum`  | `xltypeInt`  |
     * | `xltypeBool` | `xltypeInt`  |
     *
     * All other type combinations return `xlretFailed`.
     *
     * @param coper  Number of argument pointers (must be ≥ 2).
     * @param args   `args[0]` = source value; `args[1]` = desired type.
     * @param result Output `XLOPER12` to populate.
     * @return `xlretSuccess` on success; `xlretFailed` on invalid operands
     *         or unsupported type conversion.
     */
    static int handle_coerce(const int coper, const LPXLOPER12* args, const LPXLOPER12 result)
    {
        if (coper < 2 || !args[0] || !args[1] || !result)
            return xlretFailed;

        const XLOPER12& src      = *args[0];
        const int       destType = (args[1]->xltype == xltypeInt)
                                       ? static_cast<int>(args[1]->val.w)
                                       : static_cast<int>(args[1]->val.num);

        constexpr auto mask = ~static_cast<decltype(src.xltype)>(xlbitDLLFree | xlbitXLFree);
        const int srcType = static_cast<int>(src.xltype & mask);

        *result = {};

        // Identity coercion
        if (srcType == destType) { *result = src; return xlretSuccess; }

        // Numeric <-> bool <-> int conversions
        if (destType == xltypeNum) {
            if (srcType == xltypeBool) {
                result->xltype  = xltypeNum;
                result->val.num = src.val.xbool ? 1.0 : 0.0;
                return xlretSuccess;
            }
            if (srcType == xltypeInt) {
                result->xltype  = xltypeNum;
                result->val.num = static_cast<double>(src.val.w);
                return xlretSuccess;
            }
        }
        if (destType == xltypeBool) {
            if (srcType == xltypeNum) {
                result->xltype    = xltypeBool;
                result->val.xbool = (src.val.num != 0.0) ? 1 : 0;
                return xlretSuccess;
            }
            if (srcType == xltypeInt) {
                result->xltype    = xltypeBool;
                result->val.xbool = (src.val.w != 0) ? 1 : 0;
                return xlretSuccess;
            }
        }
        if (destType == xltypeInt) {
            if (srcType == xltypeNum) {
                result->xltype = xltypeInt;
                result->val.w  = static_cast<int>(src.val.num);
                return xlretSuccess;
            }
            if (srcType == xltypeBool) {
                result->xltype = xltypeInt;
                result->val.w  = src.val.xbool ? 1 : 0;
                return xlretSuccess;
            }
        }

        return xlretFailed;
    }

    /// @brief Default constructor — private; use `instance()`.
    Excel12Server() = default;
};

} // namespace MockXL


