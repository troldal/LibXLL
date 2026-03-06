/**
 * @file Nil.hpp
 * @brief Excel-compatible null/empty-cell type for the xll library.
 *
 * Defines `xll::Nil`, a concrete Excel primitive type that represents the
 * `xltypeNil` sentinel value used by the Excel C API to signal an empty or
 * null cell.
 *
 * **Memory layout**
 *
 * `xll::Nil` inherits from `impl::Base<Nil, xltypeNil>`, which itself
 * inherits from `XLOPER12` without adding any data members. The object
 * therefore occupies exactly `sizeof(XLOPER12)` bytes and can be passed
 * directly to and from the Excel C API without any conversion.
 *
 * **Semantics**
 *
 * `xltypeNil` is a type tag only — no union member of `XLOPER12` carries a
 * payload for this type. The sole information conveyed is the `xltype` field
 * being equal to `xltypeNil`. As a consequence:
 * - All `Nil` objects are equal to each other.
 * - There is no meaningful value to read from the object.
 * - Copy and assignment simply re-set `xltype = xltypeNil`.
 *
 * **Relationship to `xll::Missing`**
 *
 * Both `xll::Nil` and `xll::Missing` are valueless sentinel types used by
 * Excel to represent the absence of data. `xltypeNil` represents an empty or
 * null cell value, whereas `xltypeMissing` specifically denotes an omitted
 * optional function argument. `xll::Missing` is implicitly convertible to
 * `xll::Nil` (via `Missing::operator Nil()`), but not vice versa — a null
 * cell value is not the same as an omitted argument.
 *
 * **Restricted interface**
 *
 * Like `xll::Missing`, `xll::Nil` does **not** use `using BASE::BASE` or
 * `using BASE::operator=`. This deliberately prevents construction or
 * assignment from arbitrary `XLOPER12` values, numeric types, or other xll
 * types — a `Nil` can only ever represent an empty cell.
 *
 * The following `impl::Base` members are still accessible:
 * - `is_valid()` — returns `true` when `xltype == xltypeNil`
 * - `has_crtp_base` — compile-time CRTP detection flag
 * - `excel_type` — compile-time `xltypeNil` constant
 *
 * @see xll::Missing
 * @see impl::Base
 */

#pragma once

#include "Base.hpp"

namespace xll
{
    /**
     * @brief Excel-compatible null/empty-cell type (`xltypeNil`).
     *
     * Represents the Excel sentinel value that signals an empty or null cell.
     * The `XLOPER12::xltype` field is set to `xltypeNil`; no union member
     * carries a payload.
     *
     * - All instances are semantically identical and compare equal.
     * - The class has no value semantics beyond its type tag.
     * - `xll::Missing` is implicitly convertible to `xll::Nil`, but `xll::Nil`
     *   is not convertible to `xll::Missing`.
     *
     * **Design note — suppressed `BASE::BASE`**
     *
     * `using BASE::BASE` and `using BASE::operator=` are intentionally
     * commented out. Inheriting them would allow construction and assignment
     * from arbitrary `XLOPER12` values or numeric types, which is semantically
     * wrong for a type that can only ever mean "empty cell". Instead, only the
     * default constructor, explicit copy constructor, and copy assignment
     * operator are provided, all of which unconditionally set
     * `xltype = xltypeNil`.
     *
     * @note `sizeof(xll::Nil) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note Move construction and move assignment are not explicitly defined.
     *       The compiler-generated move operations delegate to the copy
     *       operations, which is correct here because there is no resource
     *       to transfer.
     */
    class Nil : public impl::Base<Nil, xltypeNil>
    {
        using BASE = impl::Base<Nil, xltypeNil>;

    public:
        // using BASE::BASE;

        /**
         * @brief Default constructor — constructs a valid null/empty-cell value.
         *
         * `impl::Base`'s default constructor sets `xltype = xltypeNil`.
         * No union member is initialised because `xltypeNil` carries no payload.
         *
         * @post `xltype == xltypeNil`
         * @post `is_valid() == true`
         */
        constexpr Nil() = default;

        /**
         * @brief Copy constructor — produces a valid null/empty-cell value.
         *
         * Ignores the source object's state entirely and unconditionally sets
         * `xltype = xltypeNil`. This is correct because all `Nil` objects are
         * semantically identical — there is no payload to copy.
         *
         * @param Unnamed source `Nil` object (ignored).
         *
         * @post `xltype == xltypeNil`
         * @post `is_valid() == true`
         */
        constexpr Nil(const Nil&) : BASE() {}
        // {
        //     xltype = xltypeNil;
        // }

        /**
         * @brief Copy assignment operator — re-asserts the null/empty-cell type.
         *
         * Ignores the right-hand side entirely and unconditionally sets
         * `xltype = xltypeNil`, then returns `*this`. Self-assignment is safe
         * because no resources are involved.
         *
         * @param Unnamed source `Nil` object (ignored).
         * @return Reference to `*this`.
         *
         * @post `xltype == xltypeNil`
         * @post `is_valid() == true`
         */
        constexpr Nil& operator=(const Nil&)
        {
            xltype = xltypeNil;
            return *this;
        }

        /**
         * @brief Equality comparison — all `Nil` objects are equal.
         *
         * Because `xltypeNil` carries no payload and all instances are
         * semantically identical, this operator always returns `true`.
         * It is defined as a hidden friend to participate in ADL.
         *
         * @param Unnamed left operand (ignored).
         * @param Unnamed right operand (ignored).
         * @return Always `true`.
         */
        constexpr friend bool operator==(const Nil&, const Nil&)
        {
            return true;
        }
    };
}    // namespace xll

