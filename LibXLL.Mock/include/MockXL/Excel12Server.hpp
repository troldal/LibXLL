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
#include "Types/Any.hpp"
#include "Types/Nil.hpp"

#include <array>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
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

namespace MockXL {

// ============================================================================
// Registration
// ============================================================================

/**
 * @brief One entry produced by an xlfRegister call from the add-in.
 *
 * Constructed by Excel12Server when it handles an xlfRegister call.
 * The xlfRegister operands are parsed into named fields for easier access,
 * and invoke() executes the resolved exported function.
 */
class Registration
{
public:
    Registration() = default;

    explicit Registration(int id, const std::vector<xll::Any>& args)
        : m_function_id(id)
        , m_module_path(require_string(args, 0, "module path"))
        , m_procedure_name(require_string(args, 1, "procedure name"))
        , m_signature(require_string(args, 2, "signature"))
        , m_excel_name(require_string(args, 3, "Excel name"))
        , m_argument_names(require_string(args, 4, "argument names"))
        , m_function_type(optional_int(args, 5))
        , m_category(optional_any(args, 6))
        , m_reserved(optional_any(args, 7))
        , m_help_topic(optional_string(args, 8))
        , m_description(optional_string(args, 9))
        , m_argument_help(collect_strings(args, 10))
    {}

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    [[nodiscard]] int                                function_id()    const noexcept { return m_function_id; }
    [[nodiscard]] const xll::String&                 module_path()    const noexcept { return m_module_path; }
    [[nodiscard]] const xll::String&                 procedure_name() const noexcept { return m_procedure_name; }
    [[nodiscard]] const xll::String&                 signature()      const noexcept { return m_signature; }
    [[nodiscard]] const xll::String&                 excel_name()     const noexcept { return m_excel_name; }
    [[nodiscard]] const xll::String&                 argument_names() const noexcept { return m_argument_names; }
    [[nodiscard]] const std::optional<xll::Int>&     function_type()  const noexcept { return m_function_type; }
    [[nodiscard]] const std::optional<xll::Any>&     category()       const noexcept { return m_category; }
    [[nodiscard]] const std::optional<xll::Any>&     reserved()       const noexcept { return m_reserved; }
    [[nodiscard]] const std::optional<xll::String>&  help_topic()     const noexcept { return m_help_topic; }
    [[nodiscard]] const std::optional<xll::String>&  description()    const noexcept { return m_description; }
    [[nodiscard]] const std::vector<xll::String>&    argument_help()  const noexcept { return m_argument_help; }
    [[nodiscard]] const Function&                    function()       const noexcept { return m_function; }

    void set_function(Function function) noexcept
    {
        m_function = function;
    }

    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(m_function); }

    // ------------------------------------------------------------------
    // Invocation
    // ------------------------------------------------------------------

    [[nodiscard]] LPXLOPER12 invoke(const XLOPER12* const* xlargs, int nargs) const
    {
        static XLOPER12 s_nil = xll::Nil{};
        std::array<LPXLOPER12, MaxXllArity + 1> p{};
        p.fill(&s_nil);
        for (int i = 0; i < nargs && i < static_cast<int>(MaxXllArity + 1); ++i)
            p[static_cast<std::size_t>(i)] = const_cast<LPXLOPER12>(xlargs[i]);
        return m_function(p.data());
    }

private:
    static xll::String require_string(const std::vector<xll::Any>& args,
                                      std::size_t index,
                                      std::string_view field_name)
    {
        if (index >= args.size())
            throw std::runtime_error("[MockXL] missing required xlfRegister field: " + std::string(field_name));
        if (auto value = xll::cast<xll::String>(args[index]))
            return *value;
        throw std::runtime_error("[MockXL] xlfRegister field is not a string: " + std::string(field_name));
    }

    static std::optional<xll::String> optional_string(const std::vector<xll::Any>& args,
                                                      std::size_t index)
    {
        if (index >= args.size() || xll::holds<xll::Nil>(args[index]))
            return std::nullopt;
        if (auto value = xll::cast<xll::String>(args[index]))
            return *value;
        return std::nullopt;
    }

    static std::optional<xll::Int> optional_int(const std::vector<xll::Any>& args,
                                                std::size_t index)
    {
        if (index >= args.size() || xll::holds<xll::Nil>(args[index]))
            return std::nullopt;
        if (auto value = xll::cast<xll::Int>(args[index]))
            return *value;
        return std::nullopt;
    }

    static std::optional<xll::Any> optional_any(const std::vector<xll::Any>& args,
                                                std::size_t index)
    {
        if (index >= args.size() || xll::holds<xll::Nil>(args[index]))
            return std::nullopt;
        return args[index];
    }

    static std::vector<xll::String> collect_strings(const std::vector<xll::Any>& args,
                                                    std::size_t first_index)
    {
        std::vector<xll::String> result;
        for (std::size_t i = first_index; i < args.size(); ++i) {
            if (auto value = xll::cast<xll::String>(args[i]))
                result.push_back(*value);
        }
        return result;
    }

    int                        m_function_id{};
    xll::String                m_module_path;
    xll::String                m_procedure_name;
    xll::String                m_signature;
    xll::String                m_excel_name;
    xll::String                m_argument_names;
    std::optional<xll::Int>    m_function_type;
    std::optional<xll::Any>    m_category;
    std::optional<xll::Any>    m_reserved;
    std::optional<xll::String> m_help_topic;
    std::optional<xll::String> m_description;
    std::vector<xll::String>   m_argument_help;
    Function                   m_function;
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
    // Call a registered function by its Excel name
    // ------------------------------------------------------------------

    /**
     * @brief Invokes a registered XLL function by its Excel-visible name.
     *
     * Each element of @p xlargs is passed as a pointer to the XLL function.
     * Up to 30 arguments are supported (the Excel SDK maximum).
     *
     * @return The function's return value as an xll::Any, or xltypeNil on failure.
     */
    [[nodiscard]] xll::Any call_by_excel_name(std::string_view excel_name,
                                               std::initializer_list<const XLOPER12*> xlargs) const
    {
        return call_impl(excel_name, xlargs.begin(), static_cast<int>(xlargs.size()));
    }

    /** @overload Accepts a vector of XLOPER12 pointers. */
    [[nodiscard]] xll::Any call_by_excel_name(std::string_view excel_name,
                                               const std::vector<const XLOPER12*>& xlargs) const
    {
        return call_impl(excel_name, xlargs.data(), static_cast<int>(xlargs.size()));
    }

    /** @overload Accepts a raw pointer array and count. Used by Session::call<Name>. */
    [[nodiscard]] xll::Any call_by_excel_name(std::string_view excel_name,
                                               const XLOPER12* const* xlargs, int nargs) const
    {
        return call_impl(excel_name, xlargs, nargs);
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
                const int id = m_next_id++;

                std::vector<xll::Any> args;
                for (int i = 0; i < coper; ++i)
                    if (rgpxloper12[i])
                        args.emplace_back(*rgpxloper12[i]);

                Registration reg{ id, args };

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

                m_registrations.emplace(static_cast<std::string>(reg.excel_name()), std::move(reg));

                if (xloper12Res) {
                    xloper12Res->xltype  = xltypeNum;
                    xloper12Res->val.num = static_cast<double>(id);
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

    xll::String                                      m_xll_name  { "MockXL.xll" };
    std::unordered_map<std::string, Registration>    m_registrations;
    int                                              m_next_id   { 1 };
    std::function<void*(const std::string&)>         m_proc_resolver;


    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    [[nodiscard]] xll::Any call_impl(std::string_view excel_name,
                                     const XLOPER12* const* xlargs, int nargs) const
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

        LPXLOPER12 ret = reg.invoke(xlargs, nargs);

        if (!ret) return xll::Any{ xll::Nil{} };
        return xll::Any{ *ret };
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


