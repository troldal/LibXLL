#pragma once

#include "Function.hpp"
#include "Types/Any.hpp"
#include "Types/Nil.hpp"

#include <atomic>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <xlcall.hpp>

namespace MockXL {

    namespace impl {class Excel12Server;}

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
    friend class impl::Excel12Server;

    Registration() = default;

    explicit Registration(const std::vector<xll::Any>& args)
        : m_function_id(s_next_id.fetch_add(1))
        , m_module_path(req<xll::String, 0>(args, "module path"))
        , m_procedure_name(req<xll::String, 1>(args, "procedure name"))
        , m_signature(req<xll::String, 2>(args, "signature"))
        , m_excel_name(req<xll::String, 3>(args, "Excel name"))
        , m_argument_names(req<xll::String, 4>(args, "argument names"))
        , m_function_type(opt<xll::Int, 5>(args))
        , m_category(opt<xll::Any, 6>(args))
        , m_reserved(opt<xll::Any, 7>(args))
        , m_help_topic(opt<xll::String, 8>(args))
        , m_description(opt<xll::String, 9>(args))
        , m_argument_help(vec<xll::String, 10>(args))
    {}

public:

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

    template<typename T, std::size_t I>
    static T req(const std::vector<xll::Any>& args, std::string_view field_name)
    {
        if (I >= args.size())
            throw std::runtime_error("[MockXL] missing required xlfRegister field: " + std::string(field_name));
        if (auto value = xll::cast<T>(args[I]))
            return *value;
        throw std::runtime_error("[MockXL] xlfRegister field is not the expected type: " + std::string(field_name));
    }

    template<typename T, std::size_t I>
    static std::optional<T> opt(const std::vector<xll::Any>& args)
    {
        if (I >= args.size() || xll::holds<xll::Nil>(args[I]))
            return std::nullopt;
        if (auto value = xll::cast<T>(args[I]))
            return *value;
        return std::nullopt;
    }


    template<typename T, std::size_t I>
    static std::vector<T> vec(const std::vector<xll::Any>& args)
    {
        std::vector<T> result;
        for (std::size_t i = I; i < args.size(); ++i)
            if (auto value = xll::cast<T>(args[i]))
                result.push_back(*value);
        return result;
    }

    inline static std::atomic<int> s_next_id { 1 };

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

} // namespace MockXL

