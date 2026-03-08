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
// On Linux the entry points Excel12/Excel12v are declared extern "C" in
// xlcall.h and must be provided as a strong symbol by the executable (the
// unix/xlcall_cpp.h has only stub no-op definitions so they must be
// overridden).
//
// How MockXL provides them
// -------------------------
// 1.  Include this header in your mock-executable translation unit.
// 2.  Call MockXL::Excel12Server::install() before the XLL is loaded.
//     (Session::Session() does this automatically when this header is
//     included before Session.hpp, but you can also call it explicitly.)
// 3.  Place the macro  MOCK_XL_DEFINE_EXCEL12()  in exactly ONE .cpp file
//     of the mock executable.  This defines the exported entry points that
//     the add-in resolves.
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
// All other function codes return xlretSuccess with an xltypeNil result,
// which is the correct "not implemented / don't care" behaviour for a mock.
//
// Registration records
// ---------------------
// After xlAutoOpen has run, call Excel12Server::registrations() to inspect
// what functions the add-in tried to register.  Each Registration holds the
// raw XLOPER12* arguments exactly as the add-in passed them.

#include "Types/Any.hpp"
#include "Types/Nil.hpp"

#include <cstdarg>
#include <iostream>
#include <string>
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

namespace MockXL {

// ============================================================================
// Registration record
// ============================================================================

/**
 * @brief One entry produced by an xlfRegister call from the add-in.
 *
 * The arguments are stored by value as XLOPER12 objects; string payloads are
 * deep-copied so the record remains valid after xlAutoFree12 is called.
 */
struct Registration {
    int                    function_id{};  ///< Numeric ID assigned by the mock
    std::vector<xll::Any>  args;           ///< Raw xlfRegister arguments (deep-copied via xll::Any)
};

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

    // ------------------------------------------------------------------
    // Registration records
    // ------------------------------------------------------------------

    [[nodiscard]] const std::vector<Registration>& registrations() const noexcept
    {
        return m_registrations;
    }

    void clear_registrations() { m_registrations.clear(); }

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
                const int id = m_next_id++;
                Registration reg;
                reg.function_id = id;
                for (int i = 0; i < coper; ++i) {
                    if (rgpxloper12[i])
                        reg.args.emplace_back(*rgpxloper12[i]);
                }

                // Print Excel name (arg[3]) and procedure name (arg[1]) to console.
                {
                    auto str_from_any = [](const std::vector<xll::Any>& v, std::size_t idx) -> std::string {
                        if (idx >= v.size()) return "<missing>";
                        auto s = xll::cast<xll::String>(v[idx]);
                        return s ? static_cast<std::string>(*s) : "<not a string>";
                    };

                    std::cout << "[MockXL] Registered: "
                              << str_from_any(reg.args, 3)   // Excel function name
                              << " -> "
                              << str_from_any(reg.args, 1)   // Procedure (C++) name
                              << '\n';
                }

                m_registrations.push_back(std::move(reg));

                if (xloper12Res) {
                    xloper12Res->xltype     = xltypeNum;
                    xloper12Res->val.num    = static_cast<double>(id);
                }
                return xlretSuccess;
            }

            // ----------------------------------------------------------
            // xlCoerce  — basic type coercion (num <-> bool <-> int)
            // ----------------------------------------------------------
            case xlCoerce:
                return handle_coerce(coper, rgpxloper12, xloper12Res);

            // ----------------------------------------------------------
            // Everything else — succeed silently with nil result
            // ----------------------------------------------------------
            default:
                return xlretSuccess;
        }
    }

private:
    // ------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------

    xll::String            m_xll_name   { "MockXL.xll" };
    std::vector<Registration> m_registrations;
    int                    m_next_id    { 1 };

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------


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


