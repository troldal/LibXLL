/**
 * @file Number.hpp
 * @brief Excel-compatible floating-point type for the xll library.
 *
 * Defines `xll::Number`, a concrete Excel primitive type that wraps the
 * `xltypeNum` value stored in `XLOPER12::val.num` (an IEEE 754 `double`).
 *
 * **Memory layout**
 *
 * `xll::Number` inherits from
 * `impl::Base<Number, xltypeNum, xltypeInt, xltypeBool>`, which itself
 * inherits from `XLOPER12` without adding any data members. The object
 * therefore occupies exactly `sizeof(XLOPER12)` bytes and can be passed
 * directly to and from the Excel C API without any conversion.
 *
 * **Cross-type compatibility**
 *
 * The `OtherTypes` template arguments (`xltypeInt`, `xltypeBool`) declare
 * that `xll::Number` participates in cross-type arithmetic and comparison
 * with `xll::Int` and `xll::Bool`. This mirrors Excel's own implicit
 * coercion rules, where floating-point numbers, integers, and boolean
 * values freely interoperate:
 * ```cpp
 * xll::Number n = 3.5;
 * xll::Int    i = 2;
 * xll::Bool   b = true;
 * xll::Number r = n + i;   // 3.5 + 2.0  → 5.5
 * xll::Number s = n * b;   // 3.5 * 1.0  → 3.5
 * ```
 *
 * **Inherited interface**
 *
 * The entire public interface of `xll::Number` beyond its default constructor
 * is inherited from `impl::Base`:
 * - Constructors from `double`, `int`, and `bool`
 *   (via `using BASE::BASE`)
 * - Copy and move constructors and assignment operators
 *   (via `using BASE::operator=`)
 * - Arithmetic operators (`+`, `-`, `*`, `/`, unary `+`)
 * - Compound arithmetic assignment operators (`+=`, `-=`, `*=`, `/=`)
 * - Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>`)
 * - Conversion operator `operator T()` and `explicit operator bool()`
 * - Stream output `operator<<`
 * - `is_valid()`, `has_crtp_base`, `excel_type`
 *
 * **IEEE 754 considerations**
 *
 * Because `val.num` is a `double`, all standard IEEE 754 special values
 * (`NaN`, `±Inf`) are representable. Excel itself does not support `NaN`
 * or infinity as valid cell values; the caller is responsible for
 * validating results before passing them to Excel.
 *
 * @see impl::Base
 * @see xll::Int
 * @see xll::Bool
 */

#pragma once

#include "Base.hpp"
#include <cmath>

namespace xll
{
    /**
     * @brief Excel-compatible floating-point type (`xltypeNum`).
     *
     * Represents an Excel numeric value stored in `XLOPER12::val.num`
     * (an IEEE 754 `double`). The class:
     *
     * - Inherits from `impl::Base<Number, xltypeNum, xltypeInt, xltypeBool>`,
     *   and therefore from `XLOPER12`, so its address can be reinterpret-cast
     *   to `XLOPER12*` without any offset.
     * - Is default-constructible to `0.0`.
     * - Accepts `double`, `int`, `bool`, `xll::Int`, and `xll::Bool` as
     *   construction and assignment sources via `using BASE::BASE` /
     *   `using BASE::operator=`.
     * - Provides the full arithmetic, comparison, conversion, and streaming
     *   interface inherited from `impl::Base` without adding any members of
     *   its own.
     *
     * **Relationship to `xll::Int`**
     *
     * `xll::Number` (double precision) and `xll::Int` (32-bit integer) are
     * the two numeric primitives in the Excel type system. Cross-type
     * operations between them produce a `Number` result when `Number` is the
     * left-hand operand, and an `Int` result when `Int` is the left-hand
     * operand (truncating to integer). This matches Excel's own coercion
     * behaviour.
     *
     * @note `sizeof(xll::Number) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note `XLOPER12::val.num` is an IEEE 754 `double`. Excel does not
     *       support `NaN` or infinity as cell values; validate results
     *       before passing them to Excel SDK functions.
     */
    class Number : public impl::Base<Number, xltypeNum, xltypeInt, xltypeBool>
    {
        using BASE = impl::Base<Number, xltypeNum, xltypeInt, xltypeBool>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Number";

    public:
        using BASE::BASE;
        using BASE::operator=;

        /**
         * @brief Default constructor — constructs a number with value `0.0`.
         *
         * Delegates to the inherited `Base` constructor that accepts `double`,
         * which sets `xltype = xltypeNum` and `val.num = 0.0`.
         *
         * @post `xltype == xltypeNum`
         * @post `is_valid() == true`
         * @post `static_cast<double>(*this) == 0.0`
         */
        constexpr Number() : Number(0.0) {}
    };
}    // namespace xll

