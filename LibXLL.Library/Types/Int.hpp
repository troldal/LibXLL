/**
 * @file Int.hpp
 * @brief Excel-compatible integer type for the xll library.
 *
 * Defines `xll::Int`, a concrete Excel primitive type that wraps the
 * `xltypeInt` value stored in `XLOPER12::val.w` (a 32-bit `int`).
 *
 * **Memory layout**
 *
 * `xll::Int` inherits from `impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>`,
 * which itself inherits from `XLOPER12` without adding any data members.
 * The object therefore occupies exactly `sizeof(XLOPER12)` bytes and can be
 * passed directly to and from the Excel C API without any conversion.
 *
 * **Cross-type compatibility**
 *
 * The `OtherTypes` template arguments (`xltypeNum`, `xltypeBool`) declare
 * that `xll::Int` participates in cross-type arithmetic and comparison with
 * `xll::Number` and `xll::Bool`. This mirrors Excel's own implicit coercion
 * rules, where integers, floating-point numbers, and boolean values freely
 * interoperate:
 * ```cpp
 * xll::Int    i = 3;
 * xll::Number n = 1.5;
 * xll::Bool   b = true;
 * xll::Int    r = i + b;   // 3 + 1 → 4
 * xll::Int    s = i + n;   // 3 + 1.5 → truncated to Int
 * ```
 *
 * **Inherited interface**
 *
 * The bulk of `xll::Int`'s interface is inherited from `impl::Base`:
 * - Constructors from `int`, `double`, and `bool` (via `using BASE::BASE`)
 * - Copy and move constructors and assignment operators
 *   (via `using BASE::operator=`)
 * - Arithmetic operators (`+`, `-`, `*`, `/`, unary `+`)
 * - Compound arithmetic assignment operators (`+=`, `-=`, `*=`, `/=`)
 * - Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>`)
 * - Conversion operator `operator T()` and `explicit operator bool()`
 * - Stream output `operator<<`
 * - `is_valid()`, `has_crtp_base`, `excel_type`
 *
 * **Integer-specific extensions**
 *
 * `xll::Int` adds the following beyond the `impl::Base` interface:
 * - Pre- and post-increment operators (`++`)
 * - Pre- and post-decrement operators (`--`)
 * - Modulo operator (`%`) and compound modulo assignment (`%=`)
 *
 * @see impl::Base
 * @see xll::Number
 * @see xll::Bool
 */

#pragma once
#include "Base.hpp"
#include "Bool.hpp"
#include "Number.hpp"

#include <iostream>
#include <optional>

namespace xll
{
    /**
     * @brief Excel-compatible integer type (`xltypeInt`).
     *
     * Represents an Excel integer value stored in `XLOPER12::val.w`
     * (a 32-bit `int`). The class:
     *
     * - Inherits from `impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>`,
     *   and therefore from `XLOPER12`, so its address can be reinterpret-cast
     *   to `XLOPER12*` without any offset.
     * - Is default-constructible to `0`.
     * - Accepts `int`, `double`, `bool`, `xll::Number`, and `xll::Bool` as
     *   construction and assignment sources via `using BASE::BASE` /
     *   `using BASE::operator=`.
     * - Extends the inherited arithmetic interface with increment, decrement,
     *   and modulo operations that are meaningful for integers but not for
     *   `xll::Number` or `xll::Bool`.
     *
     * @note `sizeof(xll::Int) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note The underlying `XLOPER12::val.w` member is a 32-bit `int`.
     *       Overflow behaviour follows standard C++ signed integer rules.
     */
    class Int : public impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>
    {
        using BASE = impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Int";

    public:
        using BASE::BASE;
        using BASE::operator=;

        /**
         * @brief Default constructor — constructs an integer with value `0`.
         *
         * Delegates to the inherited `Base` constructor that accepts `int`,
         * which sets `xltype = xltypeInt` and `val.w = 0`.
         *
         * @post `xltype == xltypeInt`
         * @post `is_valid() == true`
         * @post `static_cast<int>(*this) == 0`
         */
        constexpr Int() : Int(0) {}

        // =====================================================================
        // Increment / decrement operators
        // =====================================================================

        /**
         * @brief Pre-increment operator — increments the stored value by 1.
         *
         * Directly increments `val.w` and returns a reference to `*this`.
         *
         * @return Reference to `*this` after incrementing.
         *
         * @note Overflow is undefined behaviour for signed integers per the
         *       C++ standard.
         */
        constexpr Int& operator++()
        {
            ++value();
            return *this;
        }

        /**
         * @brief Post-increment operator — returns the old value, then increments.
         *
         * Copies `*this`, increments `val.w`, and returns the copy.
         *
         * @param Unnamed `int` dummy parameter (standard post-increment signature).
         * @return Copy of `*this` holding the value before incrementing.
         *
         * @note Overflow is undefined behaviour for signed integers per the
         *       C++ standard.
         */
        constexpr Int operator++(int)
        {
            Int old = *this;
            ++value();
            return old;
        }

        /**
         * @brief Pre-decrement operator — decrements the stored value by 1.
         *
         * Directly decrements `val.w` and returns a reference to `*this`.
         *
         * @return Reference to `*this` after decrementing.
         *
         * @note Overflow is undefined behaviour for signed integers per the
         *       C++ standard.
         */
        constexpr Int& operator--()
        {
            --value();
            return *this;
        }

        /**
         * @brief Post-decrement operator — returns the old value, then decrements.
         *
         * Copies `*this`, decrements `val.w`, and returns the copy.
         *
         * @param Unnamed `int` dummy parameter (standard post-decrement signature).
         * @return Copy of `*this` holding the value before decrementing.
         *
         * @note Overflow is undefined behaviour for signed integers per the
         *       C++ standard.
         */
        constexpr Int operator--(int)
        {
            Int old = *this;
            --value();
            return old;
        }

        // =====================================================================
        // Modulo operators
        // =====================================================================

        /**
         * @brief Compound modulo assignment — computes `*this %= rhs`.
         *
         * Converts `rhs` to `int` via `static_cast`, then performs
         * `val.w %= rhsValue`. `TValue` must be implicitly convertible to `int`.
         *
         * @tparam TValue Type of the right-hand operand. Must satisfy
         *                `std::convertible_to<TValue, int>`. Typical types:
         *                `int`, `xll::Bool`, raw `bool`.
         * @param rhs Right-hand operand.
         * @return Reference to `*this`.
         *
         * @warning Division (modulo) by zero is undefined behaviour. The caller
         *          is responsible for ensuring `rhs != 0`.
         */
        template<typename TValue>
        constexpr Int& operator%=(TValue rhs)
            requires std::convertible_to<TValue, int>
        {
            // ensure(is_valid());
            auto rhsValue = static_cast<int>(rhs);
            // ensure(rhsValue != 0 && "Modulo by zero");
            value() %= rhsValue;
            return *this;
        }

        /**
         * @brief Modulo operator — computes `lhs % rhs` where `rhs` is an
         *        `xll::Int` or `xll::Bool`.
         *
         * Creates a copy of `lhs`, applies `operator%=`, and returns the result.
         * Accepts `xll::Int` or `xll::Bool` as the right-hand operand; other
         * xll types (e.g. `xll::Number`) are excluded by the concept constraint,
         * matching the semantics of C++ built-in integer modulo.
         *
         * @tparam TOther Type of the right-hand operand. Must be (after removing
         *                cv/ref qualifiers) exactly `xll::Int` or `xll::Bool`.
         * @param lhs Left-hand operand.
         * @param rhs Right-hand operand.
         * @return New `Int` holding `lhs % rhs`.
         *
         * @warning Modulo by zero is undefined behaviour.
         */
        template<typename TOther>
            requires (std::same_as<std::remove_cvref_t<TOther>, Int> || std::same_as<std::remove_cvref_t<TOther>, xll::Bool>)
        constexpr friend Int operator%(const Int& lhs, TOther&& rhs)
        {
            Int result = lhs;
            result %= std::forward<TOther>(rhs);
            return result;
        }

        /**
         * @brief Modulo operator — computes `lhs % rhs` where `rhs` is a
         *        primitive integral type.
         *
         * Creates a copy of `lhs`, applies `operator%=`, and returns the result.
         * Accepts any type satisfying `std::integral` (e.g. `int`, `long`,
         * `unsigned int`) as the right-hand operand.
         *
         * @tparam TValue Type of the right-hand operand. Must satisfy
         *                `std::integral<TValue>`.
         * @param lhs Left-hand operand.
         * @param rhs Right-hand operand.
         * @return New `Int` holding `lhs % rhs`.
         *
         * @warning Modulo by zero is undefined behaviour.
         */
        template<typename TValue>
            requires std::integral<TValue>
        constexpr friend Int operator%(const Int& lhs, TValue rhs)
        {
            Int result = lhs;
            result %= rhs;
            return result;
        }
    };

}    // namespace xll

