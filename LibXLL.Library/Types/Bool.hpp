/**
 * @file Bool.hpp
 * @brief Excel-compatible boolean type for the xll library.
 *
 * Defines `xll::Bool`, a concrete Excel primitive type that wraps the
 * `xltypeBool` value stored in `XLOPER12::val.xbool` (a 32-bit `BOOL`).
 *
 * **Memory layout**
 *
 * `xll::Bool` inherits from `impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>`,
 * which itself inherits from `XLOPER12` without adding any data members.
 * The object therefore occupies exactly `sizeof(XLOPER12)` bytes and can be
 * passed directly to and from the Excel C API without any conversion.
 *
 * **Cross-type compatibility**
 *
 * The `OtherTypes` template arguments (`xltypeInt`, `xltypeNum`) declare that
 * `xll::Bool` participates in cross-type arithmetic and comparison with
 * `xll::Int` and `xll::Number`. This is consistent with Excel's own implicit
 * coercion rules, where `TRUE` equals `1` and `FALSE` equals `0`.
 *
 * **Inherited interface**
 *
 * The bulk of `xll::Bool`'s interface is inherited from `impl::Base`:
 * - Constructors from `bool`, `int`, and `double` (via `using BASE::BASE`)
 * - Copy and move constructors and assignment operators
 * - Arithmetic operators (`+`, `-`, `*`, `/`)
 * - Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>`)
 * - Conversion operator `operator T()` and `explicit operator bool()`
 * - Stream output `operator<<`
 * - `is_valid()`, `has_crtp_base`, `excel_type`
 *
 * @see impl::Base
 * @see xll::Int
 * @see xll::Number
 */

#pragma once

#include "Base.hpp"
#include <cmath>

namespace xll
{
    /**
     * @brief Excel-compatible boolean type (`xltypeBool`).
     *
     * Represents an Excel boolean value (`TRUE` / `FALSE`) stored in
     * `XLOPER12::val.xbool` (a 32-bit `BOOL`). The class:
     *
     * - Inherits from `impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>`,
     *   and therefore from `XLOPER12`, so its address can be reinterpret-cast
     *   to `XLOPER12*` without any offset.
     * - Is default-constructible to `false`.
     * - Accepts `bool`, `xll::Int`, and `xll::Number` as construction and
     *   assignment sources via the inherited `BASE::BASE` / `BASE::operator=`.
     * - Provides a hidden-friend `operator==(const Bool&, bool)` that compares
     *   the stored value against a raw C++ `bool` without requiring a
     *   temporary `Bool` object.
     *
     * **Cross-type operations**
     *
     * Because `xltypeInt` and `xltypeNum` are listed as `OtherTypes`, the
     * inherited `impl::Base` operators allow:
     * ```cpp
     * xll::Bool b  = true;
     * xll::Int  i  = 1;
     * xll::Bool r1 = b + i;    // Bool + Int  → Bool
     * bool      r2 = (b == i); // Bool == Int  (compares as int)
     * ```
     *
     * @note `sizeof(xll::Bool) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note The underlying `XLOPER12::val.xbool` member is a 32-bit `BOOL`
     *       (Win32 `int`), not a C++ `bool`. The conversion operators in
     *       `impl::Base` handle this correctly.
     */
    class Bool : public impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>
    {
        using BASE = impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Bool";

    public:
        using BASE::BASE;
        using BASE::operator=;

        /**
         * @brief Default constructor — constructs a `false` boolean value.
         *
         * Delegates to the inherited `Base` constructor that accepts `bool`,
         * which sets `xltype = xltypeBool` and `val.xbool = 0`.
         *
         * @post `xltype == xltypeBool`
         * @post `is_valid() == true`
         * @post `static_cast<bool>(*this) == false`
         */
        constexpr Bool() : Bool(false) {}

        /**
         * @brief Equality comparison with a raw C++ `bool`.
         *
         * Converts the stored `XLOPER12::val.xbool` to `bool` via
         * `static_cast<bool>(lhs.value())` and compares against `rhs`.
         *
         * This overload is provided as a hidden friend rather than relying
         * solely on the inherited `impl::Base` comparisons, ensuring that
         * `xll::Bool == bool` resolves unambiguously without constructing
         * a temporary `Bool` object.
         *
         * @param lhs Left operand (`xll::Bool`).
         * @param rhs Right operand (raw C++ `bool`).
         * @return `true` if the stored value equals `rhs`, `false` otherwise.
         */
        constexpr friend bool operator==(const Bool& lhs, bool rhs)
        {
            XLL_ENSURE(lhs.is_valid());
            return static_cast<bool>(lhs.value()) == rhs;
        }
    };
}    // namespace xll
