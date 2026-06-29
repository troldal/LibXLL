/**
 * @file Error.hpp
 * @brief Excel-compatible error type for the xll library.
 *
 * Defines `xll::Error`, a concrete Excel primitive type that wraps the
 * `xltypeErr` value stored in `XLOPER12::val.err` (a 32-bit `int` holding
 * one of the `xlerr*` constants from the Excel SDK).
 *
 * **Memory layout**
 *
 * `xll::Error` inherits from `impl::Base<Error, xltypeErr>`, which itself
 * inherits from `XLOPER12` without adding any data members. The object
 * therefore occupies exactly `sizeof(XLOPER12)` bytes and can be passed
 * directly to and from the Excel C API without any conversion.
 *
 * **Excel error codes**
 *
 * The seven standard Excel error values and their SDK constants are:
 *
 * | Excel display | SDK constant  | `error_index()` | `error_id()` |
 * |---------------|---------------|-----------------|--------------|
 * | `#NULL!`      | `xlerrNull`   | 0               | 0            |
 * | `#DIV/0!`     | `xlerrDiv0`   | 1               | 7            |
 * | `#VALUE!`     | `xlerrValue`  | 2               | 15           |
 * | `#REF!`       | `xlerrRef`    | 3               | 23           |
 * | `#NAME?`      | `xlerrName`   | 4               | 29           |
 * | `#NUM!`       | `xlerrNum`    | 5               | 36           |
 * | `#N/A`        | `xlerrNA`     | 6               | 42           |
 *
 * **Predefined constants**
 *
 * Seven `inline static const` objects (`xll::ErrNull`, `xll::ErrDiv0`, etc.)
 * are provided for convenient use at call sites without constructing temporaries.
 *
 * **No arithmetic, no cross-type operations**
 *
 * Unlike `xll::Bool`, `xll::Int`, and `xll::Number`, `xll::Error` declares no
 * `OtherTypes` in its `impl::Base` instantiation. This intentionally disables
 * cross-type arithmetic and conversion inherited from `impl::Base`, reflecting
 * the fact that Excel errors are not numeric values.
 *
 * **Inherited interface**
 *
 * The following are inherited from `impl::Base`:
 * - Construction from raw `XLOPER12` (explicit)
 * - Copy and move constructors and assignment operators
 * - `is_valid()`, `has_crtp_base`, `excel_type`
 * - `explicit operator bool()` (returns `true` for any valid error)
 * - Stream output `operator<<` (delegates to `to_string()`)
 *
 * @see impl::Base
 * @see xll::ErrNull, xll::ErrDiv0, xll::ErrValue, xll::ErrRef,
 *      xll::ErrName, xll::ErrNum, xll::ErrNA
 */

#pragma once

#include "Base.hpp"
#include "String.hpp"

#include <format>
#include <string>

namespace xll
{
    /**
     * @brief Excel-compatible error type (`xltypeErr`).
     *
     * Represents one of Excel's seven standard error values, stored as a
     * 32-bit integer (`XLOPER12::val.err`) whose value is one of the `xlerr*`
     * SDK constants. The class:
     *
     * - Inherits from `impl::Base<Error, xltypeErr>`, and therefore from
     *   `XLOPER12`, so its address can be reinterpret-cast to `XLOPER12*`
     *   without any offset.
     * - Has no default constructor — an `xll::Error` must always be
     *   constructed from a specific error code.
     * - Is not constructible from, or comparable with, numeric types
     *   (`xll::Int`, `xll::Number`, `xll::Bool`), since no `OtherTypes` are
     *   declared in the `impl::Base` instantiation.
     * - Provides human-readable string representations via `to_string()` and
     *   `operator<<`.
     *
     * **Typical usage**
     * ```cpp
     * // Use the predefined constants:
     * return xll::ErrNA;
     *
     * // Or construct from an SDK constant:
     * XLOPER12 raw{}; raw.xltype = xltypeErr; raw.val.err = xlerrDiv0;
     * xll::Error e(raw);
     *
     * // Inspect and format:
     * std::cout << e;                    // prints "#DIV/0!"
     * std::string s = e.to_string();     // returns xll::String("#DIV/0!")
     * int idx = e.error_index();         // returns 1
     * ```
     *
     * @note `sizeof(xll::Error) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note `XLOPER12::val.err` values (`xlerrNull`, `xlerrDiv0`, etc.) are
     *       small non-contiguous integers defined by the Excel SDK. Use
     *       `error_index()` for a zero-based contiguous index, or `error_id()`
     *       for the raw SDK value.
     */
    class Error : public impl::Base<Error, xltypeErr>
    {
        using BASE = impl::Base<Error, xltypeErr>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Error";

    public:
        using BASE::BASE;

        // =====================================================================
        // String representation
        // =====================================================================

        /**
         * @brief Returns the Excel display string for this error value.
         *
         * Maps `val.err` to the conventional Excel error string:
         *
         * | `val.err`     | Return value |
         * |---------------|--------------|
         * | `xlerrNull`   | `"#NULL!"`   |
         * | `xlerrDiv0`   | `"#DIV/0!"`  |
         * | `xlerrValue`  | `"#VALUE!"`  |
         * | `xlerrRef`    | `"#REF!"`    |
         * | `xlerrName`   | `"#NAME?"`   |
         * | `xlerrNum`    | `"#NUM!"`    |
         * | `xlerrNA`     | `"#N/A"`     |
         *
         * @return `xll::String` containing the error display text.
         *
         * @throws std::runtime_error if `val.err` is not one of the seven
         *         standard `xlerr*` constants.
         */
        [[nodiscard]]
        constexpr xll::String to_string() const
        {
            switch (val.err) {
                case xlerrNull:
                    return "#NULL!";
                case xlerrDiv0:
                    return "#DIV/0!";
                case xlerrValue:
                    return "#VALUE!";
                case xlerrRef:
                    return "#REF!";
                case xlerrName:
                    return "#NAME?";
                case xlerrNum:
                    return "#NUM!";
                case xlerrNA:
                    return "#N/A";
                default:
                    throw std::runtime_error("Unknown error");
            }
        }

        // =====================================================================
        // Conversion
        // =====================================================================

        /**
         * @brief Explicit conversion to any integral type.
         *
         * Returns `error_index()` cast to `T`. Allows compact use in
         * switch statements or arithmetic contexts where a zero-based index
         * is more convenient than the non-contiguous raw SDK value:
         * ```cpp
         * int idx = static_cast<int>(xll::ErrNA);   // 6
         * ```
         *
         * @tparam T Target integral type. Defaults to `int`.
         *
         * @return Zero-based index of the error in the range [0, 6].
         *
         * @throws std::runtime_error if `val.err` is not a recognised error code.
         *
         * @see error_index()
         */
        template <std::integral T = int>
        constexpr explicit operator T() const
        {
            return error_index();
        }

        // =====================================================================
        // Error code accessors
        // =====================================================================

        /**
         * @brief Returns a zero-based contiguous index for this error.
         *
         * Maps the non-contiguous `xlerr*` SDK constants to a compact
         * zero-based index suitable for array indexing or switch dispatch:
         *
         * | Error      | Index |
         * |------------|-------|
         * | `#NULL!`   | 0     |
         * | `#DIV/0!`  | 1     |
         * | `#VALUE!`  | 2     |
         * | `#REF!`    | 3     |
         * | `#NAME?`   | 4     |
         * | `#NUM!`    | 5     |
         * | `#N/A`     | 6     |
         *
         * @return Integer in the range [0, 6].
         *
         * @throws std::runtime_error (via `ensure`) if `xltype != xltypeErr`.
         * @throws std::runtime_error if `val.err` is not a recognised code.
         */
        [[nodiscard]]
        constexpr int error_index() const
        {
            XLL_ENSURE(xltype == xltypeErr);
            switch (val.err) {
                case xlerrNull:
                    return 0;
                case xlerrDiv0:
                    return 1;
                case xlerrValue:
                    return 2;
                case xlerrRef:
                    return 3;
                case xlerrName:
                    return 4;
                case xlerrNum:
                    return 5;
                case xlerrNA:
                    return 6;
                default:
                    throw std::runtime_error("Unknown error");
            }
        }

        /**
         * @brief Returns the raw Excel SDK error code.
         *
         * Returns the value of `XLOPER12::val.err` directly — one of the
         * `xlerr*` constants defined by the Excel SDK (`xlerrNull`,
         * `xlerrDiv0`, `xlerrValue`, `xlerrRef`, `xlerrName`, `xlerrNum`,
         * `xlerrNA`).
         *
         * Use this when interfacing directly with Excel SDK functions that
         * expect the raw error code, rather than the zero-based index.
         *
         * @return Raw `xlerr*` SDK constant value.
         *
         * @throws std::runtime_error (via `ensure`) if `xltype != xltypeErr`.
         *
         * @see error_index() for the zero-based contiguous index.
         */
        [[nodiscard]]
        constexpr int error_id() const
        {
            XLL_ENSURE(xltype == xltypeErr);
            return val.err;
        }

        // =====================================================================
        // Comparison
        // =====================================================================

        /**
         * @brief Equality comparison between two `Error` values.
         *
         * Compares the underlying `val.err` integer values directly. Two
         * `Error` objects are equal if and only if they hold the same
         * `xlerr*` constant.
         *
         * Defined as a hidden friend to participate in ADL without being
         * visible to unqualified lookup (unlike `xll::String`'s operators,
         * which must be at namespace scope for Catch2 compatibility —
         * `xll::Error` is not compared against string literals in tests).
         *
         * @param lhs Left operand.
         * @param rhs Right operand.
         * @return `true` if both errors represent the same Excel error code.
         */
        constexpr friend bool operator==(const xll::Error& lhs, const xll::Error& rhs)
        {
            XLL_ENSURE(lhs.is_valid());
            XLL_ENSURE(rhs.is_valid());
            return lhs.value() == rhs.value();
        }

        // =====================================================================
        // Stream output
        // =====================================================================

        /**
         * @brief Writes the Excel error display string to an output stream.
         *
         * Delegates to `to_string()` and then to `operator<<(ostream, String)`.
         * Supports chaining (`os << e1 << e2`).
         *
         * @param os Destination output stream.
         * @param error Source `Error` value.
         * @return Reference to `os`.
         *
         * @throws std::runtime_error if `val.err` is not a recognised code.
         */
        friend std::ostream& operator<<(std::ostream& os, const Error& error) { return os << error.to_string(); }
    };

    // =========================================================================
    // Predefined error constants
    // =========================================================================
    //
    // Inline static const objects for all seven standard Excel error values.
    // Using these avoids constructing temporaries at call sites and makes
    // error-returning code self-documenting:
    //
    //   return xll::ErrNA;   // clearly communicates intent
    //
    // Each constant is initialised via an immediately-invoked lambda that
    // constructs a raw XLOPER12 with the appropriate xltype and val.err,
    // then passes it to the explicit Error(const XLOPER12&) constructor.

    /** @brief Represents the Excel `#NULL!` error (`xlerrNull`). */
    inline static const Error ErrNull  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrNull; return err; }());

    /** @brief Represents the Excel `#DIV/0!` error (`xlerrDiv0`). */
    inline static const Error ErrDiv0  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrDiv0; return err; }());

    /** @brief Represents the Excel `#VALUE!` error (`xlerrValue`). */
    inline static const Error ErrValue = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrValue; return err; }());

    /** @brief Represents the Excel `#REF!` error (`xlerrRef`). */
    inline static const Error ErrRef   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrRef; return err; }());

    /** @brief Represents the Excel `#NAME?` error (`xlerrName`). */
    inline static const Error ErrName  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrName; return err; }());

    /** @brief Represents the Excel `#NUM!` error (`xlerrNum`). */
    inline static const Error ErrNum   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrNum; return err; }());

    /** @brief Represents the Excel `#N/A` error (`xlerrNA`). */
    inline static const Error ErrNA    = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrNA; return err; }());

}    // namespace xll

/**
 * @brief `std::formatter` specialisation for `xll::Error`.
 *
 * Enables `xll::Error` to be used with `std::format` and `std::print`:
 * ```cpp
 * xll::Error e = xll::ErrNA;
 * std::string s = std::format("Result: {}", e);  // "Result: #N/A"
 * ```
 *
 * Delegates to `std::formatter<xll::String>` after converting via
 * `to_string()`, inheriting all string format specifiers (width, fill,
 * alignment) supported by `xll::String`'s own formatter.
 */
template<>
struct std::formatter<xll::Error> : std::formatter<xll::String> {
    auto format(const xll::Error& str, std::format_context& ctx) const {
        return std::formatter<xll::String>::format(str.to_string(), ctx);
    }
};
