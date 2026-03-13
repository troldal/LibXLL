#pragma once

// ============================================================================
// MockXL::Excel12Server
// ============================================================================
//
// Provides a mock implementation of the Excel callback mechanism used by XLL
// add-ins when they call Excel12() or Excel12v().
//
// How the real Excel mechanism works
// ------------------------------------
// xlcall_cpp.h (both Windows and Linux variants) implements Excel12/Excel12v
// by looking up a single entry-point called "MdCallBack12" and routing all
// calls through it.  On Windows this is resolved at runtime via
//   GetProcAddress(GetModuleHandle(NULL), "MdCallBack12")
// i.e. the host *executable* must export the symbol.
// On Linux, unix/xlcall_cpp.h provides real implementations that store the
// dispatch callback supplied via SetExcel12EntryPt, so no host-side symbols
// are required.
//
// Handled function codes
// -----------------------
//  xlFree      — frees memory allocated by Excel (no-op in mock; the
//                XloperResult RAII wrapper handles xlAutoFree12 instead)
//  xlGetName   — returns the XLL path that was registered via
//                Excel12Server::set_xll_name()
//  xlfRegister — records the registration and returns a numeric function ID
//  xlCoerce    — performs basic scalar type coercion (num<->bool<->int)
//
// All other function codes are looked up in the user-supplied handler map
// (see register_handler / unregister_handler).  If no handler is registered
// for a given code, the call succeeds silently with an xltypeNil result.
//
// Custom handlers
// ----------------
// To mock a function code not handled natively (e.g. xlcAlert, xlcMessage),
// register a handler before xlAutoOpen is called:
//
//   server.register_handler(xlcAlert,
//       [](const std::vector<xll::Any>& args, xll::Any& result) -> int {
//           std::cout << "Alert: " << ... << '\n';
//           return xlretSuccess;
//       });
//
// The handler receives the xlfn arguments as a vector of xll::Any values and
// a reference to the result (pre-initialised to xll::Nil).  It returns an
// xlret code (xlretSuccess, xlretFailed, etc.).  Built-in codes (xlFree,
// xlGetName, xlfRegister, xlCoerce) are always handled internally and cannot
// be overridden via the handler map.
//
// Registration records
// ---------------------
// After xlAutoOpen has run, call Excel12Server::registrations() to inspect
// what functions the add-in tried to register. Each Registration parses the
// xlfRegister operands into named fields (module path, procedure name,
// signature, Excel-visible name, help text, etc.) so callers don't need to
// index into the raw operand array.
//
// Implementation notes
// ---------------------
// - Excel12Server is a singleton; it must outlive all XLL code that uses
//   Excel12/Excel12v.
// - Registration records are stored in a map keyed by the Excel-visible name.
// - Function resolution (finding the C++ function pointer for a registered
//   name) is injected by the Session class after the XLL is loaded.

#include "Function.hpp"
#include "Registration.hpp"
#include "Types/Any.hpp"
#include "Types/Nil.hpp"

#include <array>
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
 * @brief Singleton that implements the MdCallBack12 / Excel12 / Excel12v
 * dispatch for mock-Excel testing.
 */
class Excel12Server
{
public:
    Excel12Server(const Excel12Server&)          = delete;
    Excel12Server& operator=(const Excel12Server&) = delete;

    // ------------------------------------------------------------------
    // Singleton access
    // ------------------------------------------------------------------

    [[nodiscard]] static Excel12Server& instance() noexcept
    {
        static Excel12Server s;
        return s;
    }

    // ------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------

    /**
     * @brief Sets the path string returned to xlGetName callers.
     * Should be set before xlAutoOpen is called (i.e. before Session loads
     * the XLL).
     */
    void set_xll_name(const std::string& name) { m_xll_name = xll::String(name); }

    [[nodiscard]] const xll::String& xll_name() const noexcept { return m_xll_name; }

    /**
     * @brief Supplies the xlAutoFree12 callback so Function::operator() can
     * free DLL-allocated return values after deep-copying them into xll::Any.
     * Session calls this immediately after resolving xlAutoFree12.
     */
    void set_auto_free(FAutoFree autoFree) noexcept
    {
        m_auto_free = std::move(autoFree);
    }

    // ------------------------------------------------------------------
    // Registration records
    // ------------------------------------------------------------------

    [[nodiscard]] const std::unordered_map<std::string, Registration>& registrations() const noexcept
    {
        return m_registrations;
    }

    void clear_registrations() { m_registrations.clear(); }

    // ------------------------------------------------------------------
    // Proc resolver — injected by Session after the XLL is loaded
    // ------------------------------------------------------------------

    /**
     * @brief Supplies a callback that resolves a C++ procedure name to a raw
     * function pointer.  Session calls this immediately after loading the XLL
     * so that xlfRegister can populate Registration::proc_ptr.
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
     * @param args   The `xlfn` arguments converted to `xll::Any` values.
     *               Passed by `const` reference; the handler must not store
     *               pointers into the vector beyond the call.
     * @param result Reference to the result value, pre-initialised to
     *               `xll::Nil`.  The handler writes its return value here.
     * @return An `xlret` code: `xlretSuccess`, `xlretFailed`, etc.
     */
    using HandlerFn = std::function<int(const std::vector<xll::Any>& args,
                                        xll::Any& result)>;

    /**
     * @brief Registers a custom handler for an Excel function/command code.
     *
     * The handler is invoked by `dispatch()` for any `xlfn` not handled
     * internally (`xlFree`, `xlGetName`, `xlfRegister`, `xlCoerce`).
     * Registering a handler for a code that already has one silently
     * replaces the previous handler.
     *
     * @param xlfn    The Excel function/command code (e.g. `xlcAlert`).
     * @param handler The callable to invoke.  See `HandlerFn` for the
     *                required signature.
     */
    void register_handler(int xlfn, HandlerFn handler)
    {
        m_handlers.insert_or_assign(xlfn, std::move(handler));
    }

    /**
     * @brief Removes the custom handler for @p xlfn, if one exists.
     *
     * After this call, `dispatch()` will revert to the silent-default
     * behaviour for that code (`xlretSuccess` with `xll::Nil`).
     *
     * @param xlfn The Excel function/command code whose handler to remove.
     */
    void unregister_handler(int xlfn)
    {
        m_handlers.erase(xlfn);
    }

    // ------------------------------------------------------------------
    // Call a registered function by its Excel name
    // ------------------------------------------------------------------

    /**
     * @brief Invokes a registered XLL function by its Excel-visible name.
     *
     * Each element of @p xlargs is an `xll::Any` value whose underlying `XLOPER12`
     * pointer is forwarded to the XLL function pointer. Up to `MaxXllArity`
     * arguments are supported; any beyond that limit are silently ignored.
     *
     * @return The function's return value as an `xll::Any`, or `xll::Nil` on failure.
     */
    [[nodiscard]] xll::Any call_by_excel_name(std::string_view excel_name,
                                               const std::vector<xll::Any>& xlargs) const
    {
        return call_impl(excel_name, xlargs);
    }

    // ------------------------------------------------------------------
    // Core dispatch — called by MdCallBack12 / Excel12 / Excel12v
    // ------------------------------------------------------------------

    /**
     * @brief Routes one Excel12 call to the appropriate mock handler.
     *
     * @param xlfn      Function / command code (e.g. xlFree, xlfRegister).
     * @param coper     Number of arguments in rgpxloper12.
     * @param rgpxloper12  Array of argument pointers.
     * @param xloper12Res  Result XLOPER12 to populate (may be nullptr).
     * @return xlretSuccess on success, xlretFailed on unrecoverable error.
     */
    int dispatch(int xlfn, int coper, LPXLOPER12* rgpxloper12, LPXLOPER12 xloper12Res)
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
                    std::string sig = static_cast<std::string>(reg.signature());
                    if (!sig.empty()) sig.erase(0, 1);
                    while (!sig.empty() && sig.back() == '$') sig.pop_back();
                    const int arity = static_cast<int>(sig.size());

                    void* fp = m_proc_resolver(static_cast<std::string>(reg.procedure_name()));
                    if (fp) reg.set_function(make_function(fp, arity));
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

    xll::String                                      m_xll_name  { "MockXL.xll" };
    std::unordered_map<std::string, Registration>    m_registrations;
    std::function<void*(const std::string&)>         m_proc_resolver;
    FAutoFree                                        m_auto_free;
    std::unordered_map<int, HandlerFn>               m_handlers; ///< Custom handlers keyed by Excel function/command code.


    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    /**
     * @brief Converts a raw `LPXLOPER12` argument array to `std::vector<xll::Any>`.
     *
     * Null pointers in @p rgpxloper12 are skipped; all valid entries are
     * deep-copied into `xll::Any` values via `xll::Any(const XLOPER12&)`.
     * Shared by the `xlfRegister` case and the custom-handler path so
     * argument construction is not duplicated.
     *
     * @param coper        Number of entries in @p rgpxloper12.
     * @param rgpxloper12  Raw argument pointer array from `Excel12`/`Excel12v`.
     * @return A vector of `xll::Any` values, one per non-null argument.
     */
    [[nodiscard]] static std::vector<xll::Any> make_args(int coper, LPXLOPER12* rgpxloper12)
    {
        std::vector<xll::Any> args;
        args.reserve(static_cast<std::size_t>(coper));
        for (int i = 0; i < coper; ++i)
            if (rgpxloper12[i])
                args.emplace_back(*rgpxloper12[i]);
        return args;
    }

    [[nodiscard]] xll::Any call_impl(std::string_view excel_name,
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
     * @brief Implements xlCoerce: convert src (args[0]) to destType (args[1]).
     */
    static int handle_coerce(int coper, LPXLOPER12* args, LPXLOPER12 result)
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

    Excel12Server()                              = default;

};

} // namespace MockXL


