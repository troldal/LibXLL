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

namespace MockXL {

    namespace impl {class Excel12Server;}

// ============================================================================
// Registration
// ============================================================================

/**
 * @brief Represents one function registration produced by an `xlfRegister` call from an add-in.
 *
 * When an XLL add-in calls `Excel12(xlfRegister, ...)`, MockXL intercepts the call and
 * constructs a `Registration` object that parses and stores all operands passed to
 * `xlfRegister` as named, typed fields. The fields follow the layout defined by the
 * Excel SDK: indices 0–9 are scalar metadata fields, and indices 10–255 are per-argument
 * help strings.
 *
 * Once the add-in's exported function has been resolved by the `AddInLoader`, the
 * `Registration` is equipped with a `Function` object via `set_function()`, after which
 * `invoke()` can be used to call the function.
 *
 * `Registration` objects are created exclusively by `impl::Excel12Server` and are not
 * intended to be constructed directly by client code.
 *
 * @note The `function_id` assigned to each `Registration` is monotonically increasing
 *       and unique for the lifetime of the process. Gaps may appear if `Registration`
 *       objects are destroyed.
 *
 * @see Function
 * @see impl::Excel12Server
 */
class Registration
{
    friend class impl::Excel12Server;

    /// @brief Default constructor; produces an empty, invalid registration.
    Registration() = default;

    /**
     * @brief Constructs a `Registration` from the raw `xlfRegister` argument list.
     *
     * Parses the `args` vector according to the `xlfRegister` operand layout:
     * - Indices 0–4 are required string fields.
     * - Indices 5–9 are optional metadata fields.
     * - Indices 10 and beyond are per-argument help strings.
     *
     * @param args The `xll::Any` operands passed to `xlfRegister`, in order.
     * @throws std::runtime_error if a required field is missing or has the wrong type.
     */
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

    [[nodiscard]] int                                function_id()    const noexcept { return m_function_id; }    ///< Unique numeric ID assigned at construction time.
    [[nodiscard]] const xll::String&                 module_path()    const noexcept { return m_module_path; }    ///< Full path to the .dll/.xll file containing the function (arg 0).
    [[nodiscard]] const xll::String&                 procedure_name() const noexcept { return m_procedure_name; } ///< Exported symbol name of the function (arg 1).
    [[nodiscard]] const xll::String&                 signature()      const noexcept { return m_signature; }      ///< Return/argument type encoding string (arg 2).
    [[nodiscard]] const xll::String&                 excel_name()     const noexcept { return m_excel_name; }     ///< Name under which the function appears in Excel (arg 3).
    [[nodiscard]] const xll::String&                 argument_names() const noexcept { return m_argument_names; } ///< Comma-delimited argument names shown in the Paste Function dialog (arg 4).
    [[nodiscard]] const std::optional<xll::Int>&     function_type()  const noexcept { return m_function_type; }  ///< 1 = worksheet function, 2 = command; absent means function (arg 5).
    [[nodiscard]] const std::optional<xll::Any>&     category()       const noexcept { return m_category; }       ///< Paste Function category name or number (arg 6).
    [[nodiscard]] const std::optional<xll::Any>&     reserved()       const noexcept { return m_reserved; }       ///< Reserved; should be Nil (arg 7).
    [[nodiscard]] const std::optional<xll::String>&  help_topic()     const noexcept { return m_help_topic; }     ///< URL of the function's help topic (arg 8).
    [[nodiscard]] const std::optional<xll::String>&  description()    const noexcept { return m_description; }    ///< Brief description shown in the Paste Function dialog (arg 9).
    [[nodiscard]] const std::vector<xll::String>&    argument_help()  const noexcept { return m_argument_help; }  ///< Per-argument help strings shown in the Paste Function dialog (args 10+).
    [[nodiscard]] const Function&                    function()       const noexcept { return m_function; }       ///< The resolved callable wrapping the exported function pointer.

    /**
     * @brief Attaches a resolved `Function` to this registration.
     *
     * Called by `impl::Excel12Server` after the exported symbol has been looked up
     * in the add-in's shared library. Once set, `invoke()` becomes usable.
     *
     * @param function A `Function` object wrapping the resolved function pointer.
     */
    void set_function(const Function function) noexcept
    {
        m_function = function;
    }

    /**
     * @brief Returns `true` if a `Function` has been attached via `set_function()`.
     *
     * An unresolved `Registration` (i.e. one whose exported symbol could not be found)
     * will return `false` here, and calling `invoke()` on it would be undefined.
     */
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(m_function); }

    // ------------------------------------------------------------------
    // Invocation
    // ------------------------------------------------------------------

    /**
     * @brief Calls the registered function with the given arguments.
     *
     * Forwards @p xlargs directly to `Function::operator()`, which builds the
     * internal `LPXLOPER12` pointer array and dispatches to the correct
     * typed function pointer.
     *
     * @param xlargs   The argument values. At most `MaxXllArity` elements are used;
     *                 any beyond that limit are silently ignored.
     * @param autoFree Optional callback invoked by the add-in to free its result.
     * @return An `xll::Any` holding the XLOPER12 value returned by the function.
     */
    [[nodiscard]] xll::Any invoke(const std::vector<xll::Any>& xlargs,
                                  const FAutoFree& autoFree = {}) const
    {
        return m_function(xlargs, autoFree);
    }

private:

    /**
     * @brief Extracts a required field at index `I` from the `xlfRegister` args.
     *
     * @tparam T         The expected `xll` type of the field.
     * @tparam I         The zero-based index into `args`.
     * @param args       The full `xlfRegister` argument list.
     * @param field_name Human-readable name used in error messages.
     * @return The extracted value of type `T`.
     * @throws std::runtime_error if the field is absent or cannot be cast to `T`.
     */
    template<typename T, std::size_t I>
    static T req(const std::vector<xll::Any>& args, const std::string_view field_name)
    {
        if (I >= args.size())
            throw std::runtime_error("[MockXL] missing required xlfRegister field: " + std::string(field_name));
        if (auto value = xll::cast<T>(args[I]))
            return *value;
        throw std::runtime_error("[MockXL] xlfRegister field is not the expected type: " + std::string(field_name));
    }

    /**
     * @brief Extracts an optional field at index `I` from the `xlfRegister` args.
     *
     * Returns `std::nullopt` if the index is out of range, the value is `xll::Nil`,
     * or the value cannot be cast to `T`.
     *
     * @tparam T   The expected `xll` type of the field.
     * @tparam I   The zero-based index into `args`.
     * @param args The full `xlfRegister` argument list.
     * @return `std::optional<T>` containing the value, or `std::nullopt`.
     */
    template<typename T, std::size_t I>
    static std::optional<T> opt(const std::vector<xll::Any>& args)
    {
        if (I >= args.size() || xll::holds<xll::Nil>(args[I]))
            return std::nullopt;
        if (auto value = xll::cast<T>(args[I]))
            return *value;
        return std::nullopt;
    }

    /**
     * @brief Collects all fields from index `I` onward as a vector of `T`.
     *
     * Used to gather the per-argument help strings (args 10+). Elements that
     * cannot be cast to `T` are silently skipped.
     *
     * @tparam T   The expected `xll` type of each element.
     * @tparam I   The zero-based starting index into `args`.
     * @param args The full `xlfRegister` argument list.
     * @return A `std::vector<T>` of all successfully cast elements from index `I` onward.
     */
    template<typename T, std::size_t I>
    static std::vector<T> vec(const std::vector<xll::Any>& args)
    {
        std::vector<T> result;
        for (std::size_t i = I; i < args.size(); ++i)
            if (auto value = xll::cast<T>(args[i]))
                result.push_back(*value);
        return result;
    }

    inline static std::atomic<int> s_next_id { 1 }; ///< Monotonically increasing counter used to assign unique function IDs.

    int                        m_function_id{};       ///< Unique ID for this registration, assigned at construction.
    xll::String                m_module_path;         ///< Full drive/path/filename of the .dll/.xll (xlfRegister arg 0).
    xll::String                m_procedure_name;      ///< Exported function name in the binary (xlfRegister arg 1).
    xll::String                m_signature;           ///< Return and argument type encoding (xlfRegister arg 2).
    xll::String                m_excel_name;          ///< Name visible to the user in Excel (xlfRegister arg 3).
    xll::String                m_argument_names;      ///< Comma-delimited argument names (xlfRegister arg 4).
    std::optional<xll::Int>    m_function_type;       ///< 1 = function, 2 = command (xlfRegister arg 5).
    std::optional<xll::Any>    m_category;            ///< Paste Function category (xlfRegister arg 6).
    std::optional<xll::Any>    m_reserved;            ///< Reserved; should be Nil (xlfRegister arg 7).
    std::optional<xll::String> m_help_topic;          ///< URL of the online help topic (xlfRegister arg 8).
    std::optional<xll::String> m_description;         ///< Short description for the Paste Function dialog (xlfRegister arg 9).
    std::vector<xll::String>   m_argument_help;       ///< Per-argument help strings for the Paste Function dialog (xlfRegister args 10+).
    Function                   m_function;            ///< The resolved callable; invalid until set_function() is called.
};

} // namespace MockXL

