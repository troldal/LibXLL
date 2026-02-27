/**
 * @file StringEnum.hpp
 * @brief Excel-compatible string-based enum with compile-time type safety.
 *
 * `xll::StringEnum<"A","B","C">` is modelled after `fxt::string_enum` but
 * lives inside an `XLOPER12` layout — it inherits from `xll::String` and
 * therefore from `XLOPER12` directly, adding no data members.
 *
 * Unlike `fxt::string_enum`, `xll::StringEnum` stores the value in an
 * `XLOPER12` payload rather than a plain index.  Like `fxt::string_enum`,
 * construction or assignment from an unrecognised string throws
 * `std::invalid_argument`.  The only way to obtain an object holding an
 * unknown string is via a raw `XLOPER12` that bypasses the xll type system,
 * which is detectable via `valid()`.
 *
 * All query operations (`index()`, `is<>()`, `visit()`) reflect this:
 *
 *  - `valid()` — always `true` for normally constructed objects; `false` only
 *    for raw-XLOPER12-derived objects with an unrecognised string.
 *  - `index()` — returns `xll::Optional<xll::Int>`: always engaged for
 *    normally constructed objects, `xll::None` only for raw-XLOPER12 bypass.
 *  - `is<"X">()` — returns `true` iff the stored string equals "X".
 *  - `visit(visitor)` — calls `visitor(Type<Str>{})` on a match, or calls
 *    `visitor(Unknown{})` for raw-XLOPER12-derived unknown values.
 *  - `value()` — returns the stored string as `xll::String`.
 *
 * @code
 * using Direction = xll::StringEnum<"North", "South", "East", "West">;
 *
 * Direction d { xll::String("North") };  // OK
 * Direction u { xll::String("Up") };     // throws std::invalid_argument
 *
 * d.visit(fxt::overload{
 *     [](Direction::Type<"North">) { ... },
 *     [](Direction::Type<"South">) { ... },
 *     [](Direction::Type<"East">)  { ... },
 *     [](Direction::Type<"West">)  { ... },
 *     [](Direction::Unknown)       { ... },   // only for raw-XLOPER12 bypass
 * });
 * @endcode
 */

#pragma once

#include "Optional.hpp"
#include "String.hpp"
#include <fxt/enums/StringEnum.hpp>

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>

namespace xll
{
    /**
     * @brief Excel-compatible string-based enum.
     *
     * Stores the raw Excel string (`xltypeStr`) in the XLOPER12 payload and
     * provides compile-time enum semantics on top.  An incoming string that
     * does not match any allowed element is still accepted — use `valid()` to
     * test before querying the current element.
     *
     * @tparam Strings  Compile-time string literals defining the allowed elements.
     *                  At least one must be provided.
     */
    template<fixstr::basic_fixed_string... Strings>
    class StringEnum : public String
    {
        static_assert(sizeof...(Strings) > 0,
            "xll::StringEnum must have at least one string");

    public:
        // ------------------------------------------------------------------
        // Sentinel type for unrecognised values
        // ------------------------------------------------------------------

        /**
         * @brief Tag type passed to a visitor when the stored string does not
         *        match any compile-time element.
         */
        struct Unknown {};

        // ------------------------------------------------------------------
        // Type alias — mirrors fxt::string_enum::Type<Str>
        // ------------------------------------------------------------------

        /**
         * @brief The `fixstr::typed_string` type corresponding to `Str`.
         *
         * @tparam Str  A compile-time string that must be in the allowed set.
         */
        template<fixstr::basic_fixed_string Str>
        using Type = fixstr::typed_string<Str>;

        // ------------------------------------------------------------------
        // Construction
        // ------------------------------------------------------------------

        /**
         * @brief Default-constructs to the first allowed element.
         */
        constexpr StringEnum()
            : String(std::string_view(std::get<0>(std::tuple{Strings...})))
        {}
        // Note: default-construction is always valid (first element),
        // so no call to validate() is needed.

        /**
         * @brief Constructs from an `xll::String`.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum(const String& str)    // NOLINT(google-explicit-constructor)
            : String(str)
        {
            validate(str);
        }

        /**
         * @brief Constructs from an `xll::String` rvalue.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum(String&& str)    // NOLINT(google-explicit-constructor)
            : String(std::move(str))
        {
            validate(static_cast<const xll::String&>(*this));
        }

        /**
         * @brief Constructs from a `std::string_view`.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum(std::string_view sv)    // NOLINT(google-explicit-constructor)
            : String(sv)
        {
            validate(static_cast<const xll::String&>(*this));
        }

        /**
         * @brief Constructs from a C-string literal.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum(const char* str)    // NOLINT(google-explicit-constructor)
            : String(str)
        {
            validate(static_cast<const xll::String&>(*this));
        }

        StringEnum(const StringEnum&)            = default;
        StringEnum(StringEnum&&)                 = default;
        StringEnum& operator=(const StringEnum&) = default;
        StringEnum& operator=(StringEnum&&)      = default;

        // ------------------------------------------------------------------
        // Assignment
        // ------------------------------------------------------------------

        /**
         * @brief Assigns from an `xll::String`.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum& operator=(const String& str)
        {
            validate(str);
            String::operator=(str);
            return *this;
        }

        /**
         * @brief Assigns from a `std::string_view`.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum& operator=(std::string_view sv)
        {
            const xll::String s(sv);
            validate(s);
            String::operator=(s);
            return *this;
        }

        /**
         * @brief Assigns from a C-string literal.
         *
         * @throws std::invalid_argument if the string does not match any allowed element.
         */
        StringEnum& operator=(const char* str)
        {
            return operator=(std::string_view(str));
        }

        // ------------------------------------------------------------------
        // Validity
        // ------------------------------------------------------------------

        /**
         * @brief Returns `true` iff the stored string matches one of the
         *        compile-time elements.
         *
         * Always returns `true` for objects constructed via the normal
         * constructors (which validate on construction).  May return `false`
         * only if the object was populated from a raw `XLOPER12` that
         * bypasses the xll type system.
         */
        [[nodiscard]] bool valid() const noexcept
        {
            return index() != npos;
        }

        // ------------------------------------------------------------------
        // Sentinel
        // ------------------------------------------------------------------

        /// Returned by `index()` when the stored string is not a recognised element.
        /// Mirrors the `std::string::npos` convention.
        static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

        // ------------------------------------------------------------------
        // Index query
        // ------------------------------------------------------------------

        /**
         * @brief Returns the zero-based index of the current string.
         *
         * Returns `npos` (`std::numeric_limits<std::size_t>::max()`) when the
         * object was populated from a raw `XLOPER12` holding an unrecognised
         * string (e.g. an Excel UDF argument that did not match any element).
         *
         * For objects constructed via the normal constructors the return value
         * is always in `[0, size())`.
         */
        [[nodiscard]] std::size_t index() const noexcept
        {
            try {
                const auto raw = find_index_raw(raw_value().to_string());
                return raw.has_value() ? *raw : npos;
            }
            catch (...) { return npos; }
        }

        // ------------------------------------------------------------------
        // Element check
        // ------------------------------------------------------------------

        /**
         * @brief Returns `true` iff the stored string equals `Str`.
         *
         * Returns `false` for unrecognised strings (raw-XLOPER12 bypass).
         *
         * @tparam Str  Must be one of the compile-time allowed strings.
         */
        template<fixstr::basic_fixed_string Str>
        [[nodiscard]] bool is() const noexcept
        {
            static_assert(fxt::StringInPack_v<Str, Strings...>,
                "String is not a member of this StringEnum");
            return raw_value() == std::string_view(Str);
        }

        // ------------------------------------------------------------------
        // Current string value
        // ------------------------------------------------------------------

        /**
         * @brief Returns the stored string as an `xll::Optional<xll::String>`.
         *
         * Returns an engaged Optional when the stored string matches one of the
         * compile-time elements (i.e. `valid() == true`), or `xll::None` when
         * the object was populated from a raw `XLOPER12` holding an unrecognised
         * string.
         */
        [[nodiscard]] xll::Optional<xll::String> value() const
        {
            const xll::String& s = raw_value();
            if (!is_known(s.to_string())) return xll::None;
            return xll::Optional<xll::String>(s);
        }

        // ------------------------------------------------------------------
        // Static queries
        // ------------------------------------------------------------------

        /**
         * @brief Returns the number of allowed elements.
         */
        static constexpr std::size_t size() noexcept { return sizeof...(Strings); }

        /**
         * @brief Returns all allowed element strings as a fixed-size array of
         *        `std::string_view`.
         */
        static constexpr std::array<std::string_view, sizeof...(Strings)> values() noexcept
        {
            return { std::string_view(Strings)... };
        }

        /**
         * @brief Returns the compile-time index of `Str` in the allowed set.
         *
         * Fails to compile if `Str` is not a member of the allowed set.
         */
        template<fixstr::basic_fixed_string Str>
        static constexpr std::size_t IndexOf() noexcept
        {
            static_assert(fxt::StringInPack_v<Str, Strings...>,
                "String is not a member of this StringEnum");
            return fxt::StringIndex_v<Str, Strings...>;
        }

        /**
         * @brief Constructs a `StringEnum` set to the element at `idx`.
         *
         * @throws std::out_of_range if `idx >= size()`.
         */
        static StringEnum from_index(std::size_t idx)
        {
            if (idx >= sizeof...(Strings))
                throw std::out_of_range(
                    "xll::StringEnum::from_index: index " +
                    std::to_string(idx) + " out of range (size " +
                    std::to_string(sizeof...(Strings)) + ")");

            return index_to_sv(idx, std::index_sequence_for<decltype(Strings)...>{});
        }

        // ------------------------------------------------------------------
        // Visitation
        // ------------------------------------------------------------------

        /**
         * @brief Visits the current element.
         *
         * If the stored string matches element at index `I`, calls
         * `visitor(Type<Strings[I]>{})`.  If no element matches, calls
         * `visitor(Unknown{})`.
         *
         * The visitor must handle all `sizeof...(Strings)` typed_string types
         * **and** `Unknown`.
         */
        template<typename Visitor>
        constexpr auto visit(Visitor&& visitor) const
        {
            const std::size_t idx = index();
            if (idx == npos)
                return visitor(Unknown{});
            return visit_at_index(idx, visitor,
                                  std::index_sequence_for<decltype(Strings)...>{});
        }

        // ------------------------------------------------------------------
        // Comparison
        // ------------------------------------------------------------------

        [[nodiscard]] bool operator==(const StringEnum& other) const
        { return raw_value() == other.raw_value(); }

        [[nodiscard]] auto operator<=>(const StringEnum& other) const
        { return raw_value() <=> other.raw_value(); }

        [[nodiscard]] bool operator==(std::string_view sv) const
        { return raw_value() == sv; }

        [[nodiscard]] auto operator<=>(std::string_view sv) const
        { return raw_value().to_string() <=> std::string(sv); }

        [[nodiscard]] bool operator==(const char* str) const
        { return raw_value() == std::string_view(str); }

        [[nodiscard]] bool operator==(const xll::String& str) const
        { return raw_value() == str; }

        // ------------------------------------------------------------------
        // Stream / ADL helpers
        // ------------------------------------------------------------------

        /**
         * @brief Returns the stored string as an `xll::Optional<xll::String>` (ADL).
         *
         * Matches the `value()` semantics: engaged for valid strings, `xll::None`
         * for unrecognised raw-XLOPER12 strings.
         */
        [[nodiscard]] friend xll::Optional<xll::String> to_string(const StringEnum& e)
        { return e.value(); }

        /**
         * @brief Streams the stored raw string regardless of validity.
         *
         * Prints the raw stored content so that debugging unknown values is possible.
         */
        friend std::ostream& operator<<(std::ostream& os, const StringEnum& e)
        { return os << e.raw_value().to_string(); }

        [[nodiscard]] friend std::size_t hash_value(const StringEnum& e) noexcept
        { return std::hash<std::string>{}(e.raw_value().to_string()); }

    private:
        // ------------------------------------------------------------------
        // Internal helpers
        // ------------------------------------------------------------------

        /// Direct access to the underlying xll::String without validity check.
        /// Used internally wherever the raw stored string is needed regardless
        /// of whether it is a recognised enum element.
        [[nodiscard]] const xll::String& raw_value() const noexcept
        {
            return static_cast<const xll::String&>(*this);
        }

        /// Returns true iff @p s matches one of the compile-time elements.
        static bool is_known(const std::string& s) noexcept
        {
            bool found = false;
            auto check = [&](auto str) {
                if (!found && std::string_view(s) == std::string_view(str))
                    found = true;
            };
            (check(Strings), ...);
            return found;
        }

        /// Returns the zero-based index of @p s, or std::nullopt.
        static std::optional<std::size_t> find_index_raw(const std::string& s) noexcept
        {
            std::size_t idx       = 0;
            bool        found     = false;
            std::size_t found_idx = 0;
            auto check = [&](auto str) {
                if (!found && std::string_view(s) == std::string_view(str)) {
                    found_idx = idx;
                    found     = true;
                }
                ++idx;
            };
            (check(Strings), ...);
            return found ? std::optional<std::size_t>(found_idx) : std::nullopt;
        }

        /// Throws std::invalid_argument if @p sv is not a recognised element.
        static void validate(const xll::String& sv)
        {
            if (!is_known(sv.to_string()))
                throw std::invalid_argument(
                    "Invalid string for xll::StringEnum: \"" +
                    sv.to_string() + "\"");
        }


        /// Tag used by the private no-validate constructor.
        struct NoValidate {};

        /// Constructs without validation — only called from index_to_sv where
        /// the string is guaranteed to be a compile-time element.
        StringEnum(std::string_view sv, NoValidate) : String(sv) {}

        /// Converts runtime index to the corresponding StringEnum via fold.
        template<std::size_t... Is>
        static StringEnum index_to_sv(std::size_t idx, std::index_sequence<Is...>)
        {
            // Default to first element; will be overwritten for the matching index.
            StringEnum result(std::string_view(std::get<0>(std::tuple{ Strings... })),
                              NoValidate{});
            auto pick = [&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
                if (idx == I)
                    result = StringEnum(std::string_view(
                        std::get<I>(std::tuple{ Strings... })), NoValidate{});
            };
            (pick(std::integral_constant<std::size_t, Is>{}), ...);
            return result;
        }

        /// Dispatches to visitor with the typed_string at compile-time index I.
        template<typename Visitor, std::size_t... Is>
        constexpr auto visit_at_index(
            std::size_t runtime_idx,
            Visitor&&   visitor,
            std::index_sequence<Is...>) const
        {
            using Ret = decltype(visitor(Unknown{}));

            if constexpr (std::is_void_v<Ret>) {
                auto try_one = [&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
                    if (runtime_idx == I) {
                        constexpr auto str = std::get<I>(std::tuple{ Strings... });
                        visitor(Type<str>{});
                    }
                };
                (try_one(std::integral_constant<std::size_t, Is>{}), ...);
            }
            else {
                auto try_one = [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
                    -> std::optional<Ret>
                {
                    if (runtime_idx != I) return std::nullopt;
                    constexpr auto str = std::get<I>(std::tuple{ Strings... });
                    return std::optional<Ret>(visitor(Type<str>{}));
                };

                std::optional<Ret> result;
                auto assign_if = [&](std::optional<Ret> r) {
                    if (r.has_value()) result = std::move(r);
                };
                (assign_if(try_one(std::integral_constant<std::size_t, Is>{})), ...);

                return *result;
            }
        }
    };

    // Layout check — forced via a concrete instantiation.
    static_assert(sizeof(StringEnum<"A">) == sizeof(XLOPER12),
        "xll::StringEnum must not add data members beyond XLOPER12");

}    // namespace xll

// =============================================================================
// std::hash specialisation
// =============================================================================

template<fixstr::basic_fixed_string... Strings>
struct std::hash<xll::StringEnum<Strings...>>
{
    std::size_t operator()(const xll::StringEnum<Strings...>& e) const noexcept
    {
        // hash_value() is an ADL friend with access to raw_value()
        return hash_value(e);
    }
};

// =============================================================================
// std::formatter specialisation (C++20 and later)
// =============================================================================

#if __cpp_lib_format >= 201907L
#include <format>

template<fixstr::basic_fixed_string... Strings>
struct std::formatter<xll::StringEnum<Strings...>> : std::formatter<std::string_view>
{
    auto format(const xll::StringEnum<Strings...>& e, std::format_context& ctx) const
    {
        // Stream the raw stored string (valid or not) via the friend operator<<.
        std::ostringstream oss;
        oss << e;
        return std::formatter<std::string_view>::format(oss.str(), ctx);
    }
};

#endif    // __cpp_lib_format

