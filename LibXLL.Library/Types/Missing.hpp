/**
 * @file Missing.hpp
 * @brief Excel-compatible missing-argument type for the xll library.
 *
 * Defines `xll::Missing`, a concrete Excel primitive type that represents the
 * `xltypeMissing` sentinel value used by the Excel C API to signal that an
 * optional argument was not supplied by the caller.
 *
 * **Memory layout**
 *
 * `xll::Missing` inherits from `impl::Base<Missing, xltypeMissing>`, which
 * itself inherits from `XLOPER12` without adding any data members. The object
 * therefore occupies exactly `sizeof(XLOPER12)` bytes and can be passed
 * directly to and from the Excel C API without any conversion.
 *
 * **Semantics**
 *
 * `xltypeMissing` is a type tag only — no union member of `XLOPER12` carries
 * a payload for this type. The sole information conveyed is the `xltype` field
 * being equal to `xltypeMissing`. As a consequence:
 * - All `Missing` objects are equal to each other.
 * - There is no meaningful value to read from the object.
 * - Copy and assignment simply re-set `xltype = xltypeMissing`.
 *
 * **Relationship to `xll::Nil`**
 *
 * Both `xll::Missing` and `xll::Nil` are valueless sentinel types used by
 * Excel to represent the absence of data. `xltypeMissing` specifically denotes
 * an omitted optional function argument, whereas `xltypeNil` represents an
 * empty or null cell value. `xll::Missing` is implicitly convertible to
 * `xll::Nil` to allow functions that accept either form of absence to handle
 * both uniformly.
 *
 * **Restricted interface**
 *
 * Unlike the numeric xll types, `xll::Missing` does **not** use
 * `using BASE::BASE` or `using BASE::operator=`. This deliberately prevents
 * construction or assignment from arbitrary `XLOPER12` values, numeric types,
 * or other xll types — a `Missing` can only ever represent a missing argument.
 *
 * The following `impl::Base` members are still accessible:
 * - `is_valid()` — returns `true` when `xltype == xltypeMissing`
 * - `has_crtp_base` — compile-time CRTP detection flag
 * - `excel_type` — compile-time `xltypeMissing` constant
 *
 * @see xll::Nil
 * @see impl::Base
 */

#pragma once

#include "Base.hpp"
#include "Nil.hpp"

namespace xll
{
    /**
     * @brief Excel-compatible missing-argument type (`xltypeMissing`).
     *
     * Represents the Excel sentinel value that signals an omitted optional
     * argument. The `XLOPER12::xltype` field is set to `xltypeMissing`; no
     * union member carries a payload.
     *
     * - All instances are semantically identical and compare equal.
     * - The class has no value semantics beyond its type tag.
     * - It is implicitly convertible to `xll::Nil`, which represents the
     *   related concept of an empty/null cell value.
     *
     * **Design note — suppressed `BASE::BASE`**
     *
     * `using BASE::BASE` and `using BASE::operator=` are intentionally
     * commented out. Inheriting them would allow construction and assignment
     * from arbitrary `XLOPER12` values or numeric types, which is semantically
     * wrong for a type that can only ever mean "argument not provided".
     * Instead, only the default constructor, explicit copy constructor, and
     * copy assignment operator are provided, all of which unconditionally set
     * `xltype = xltypeMissing`.
     *
     * @note `sizeof(xll::Missing) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note Move construction and move assignment are not explicitly defined.
     *       The compiler-generated move operations delegate to the copy
     *       operations, which is correct here because there is no resource
     *       to transfer.
     */
    class Missing : public impl::Base<Missing, xltypeMissing>
    {
        using BASE = impl::Base<Missing, xltypeMissing>;

    public:
        // using BASE::BASE;

        /**
         * @brief Default constructor — constructs a valid missing-argument value.
         *
         * `impl::Base`'s default constructor sets `xltype = xltypeMissing`.
         * No union member is initialised because `xltypeMissing` carries no
         * payload.
         *
         * @post `xltype == xltypeMissing`
         * @post `is_valid() == true`
         */
        constexpr Missing() = default;

        /**
         * @brief Copy constructor — produces a valid missing-argument value.
         *
         * Ignores the source object's state entirely and unconditionally sets
         * `xltype = xltypeMissing`. This is correct because all `Missing`
         * objects are semantically identical — there is no payload to copy.
         *
         * @param Unnamed source `Missing` object (ignored).
         *
         * @post `xltype == xltypeMissing`
         * @post `is_valid() == true`
         */
        constexpr Missing(const Missing&) : BASE() {}
        // {
        //     xltype = xltypeMissing;
        // }

        /**
         * @brief Copy assignment operator — re-asserts the missing-argument type.
         *
         * Ignores the right-hand side entirely and unconditionally sets
         * `xltype = xltypeMissing`, then returns `*this`. Self-assignment is
         * safe because no resources are involved.
         *
         * @param Unnamed source `Missing` object (ignored).
         * @return Reference to `*this`.
         *
         * @post `xltype == xltypeMissing`
         * @post `is_valid() == true`
         */
        constexpr Missing& operator=(const Missing&)
        {
            xltype = xltypeMissing;
            return *this;
        }

        /**
         * @brief Equality comparison — all `Missing` objects are equal.
         *
         * Because `xltypeMissing` carries no payload and all instances are
         * semantically identical, this operator always returns `true`.
         * It is defined as a hidden friend to participate in ADL.
         *
         * @param Unnamed left operand (ignored).
         * @param Unnamed right operand (ignored).
         * @return Always `true`.
         */
        constexpr friend bool operator==(const Missing&, const Missing&)
        {
            return true;
        }

        /**
         * @brief Implicit conversion to `xll::Nil`.
         *
         * Allows a `Missing` value to be used wherever a `Nil` is expected,
         * enabling functions that accept either form of absence to handle both
         * uniformly:
         * ```cpp
         * void accept(xll::Nil);
         * xll::Missing m;
         * accept(m);   // converts implicitly to xll::Nil
         * ```
         *
         * Returns a default-constructed `xll::Nil`, which sets
         * `xltype = xltypeNil`.
         *
         * @return A default-constructed `xll::Nil` object.
         *
         * @note This is the only direction of conversion. `xll::Nil` is **not**
         *       implicitly convertible back to `xll::Missing`, because a null
         *       cell value is not the same as an omitted argument.
         */
        constexpr operator xll::Nil() const // NOLINT
        {
            return {};
        }
    };
}    // namespace xll

