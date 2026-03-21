/**
 * @file RefBase.hpp
 * @brief CRTP base class for Excel range reference types (xltypeSRef and xltypeRef).
 *
 * Provides a uniform read/write interface for XLREF12 blocks shared by both
 * `xll::SingleRef` (xltypeSRef) and `xll::MultiRef` (xltypeRef).
 *
 * **Memory layout**
 *
 * `RefBase` inherits from `XLOPER12` and adds no data members, so
 * `sizeof(RefBase<D,T>) == sizeof(XLOPER12)`.  Derived types may be passed
 * directly to Excel C API functions that expect `XLOPER12*`.
 *
 * **CRTP contract**
 *
 * Derived classes must be friended (`friend BASE`) and must provide two
 * private primitives accessed by the base via CRTP:
 *
 * | Primitive                         | Semantics                              |
 * |-----------------------------------|----------------------------------------|
 * | `WORD count_impl() const`         | Number of XLREF12 blocks               |
 * | `const XLREF12& ref_impl(WORD) const` | Read access to the i-th block      |
 * | `XLREF12& ref_impl(WORD)`         | Write access to the i-th block         |
 *
 * @tparam TDerived The concrete derived class (CRTP).
 * @tparam XLType   Excel type tag — must be `xltypeSRef` or `xltypeRef`.
 *
 * @see xll::SingleRef
 * @see xll::MultiRef
 */

#pragma once

#include "../Utils/Ensure.hpp"
#include <cstddef>
#include <xlcall.hpp>

namespace xll::impl
{
    template<typename TDerived, size_t XLType>
    class RefBase : public XLOPER12
    {
        TDerived&       derived()       noexcept { return static_cast<TDerived&>(*this); }
        const TDerived& derived() const noexcept { return static_cast<const TDerived&>(*this); }

    protected:
        ~RefBase() = default;

        constexpr RefBase() : XLOPER12() { xltype = static_cast<decltype(xltype)>(XLType); }

    public:
        /// Compile-time CRTP detection flag (mirrors impl::Base convention).
        static constexpr bool has_crtp_base = true;

        /// The Excel type tag this specialisation represents.
        static constexpr size_t excel_type = XLType;

        /// Returns `true` when `xltype` (ignoring memory-management bits) equals `XLType`.
        [[nodiscard]] bool is_valid() const noexcept
        {
            constexpr auto TYPE_MASK = static_cast<decltype(xltype)>(~(xlbitDLLFree | xlbitXLFree));
            return (xltype & TYPE_MASK) == static_cast<decltype(xltype)>(XLType);
        }

        // ---- CRTP-dispatched primitives ----

        /// Number of XLREF12 blocks contained in this reference.
        [[nodiscard]] WORD count() const noexcept { return derived().count_impl(); }

        /// Read-only access to the i-th XLREF12 block (0-based).
        [[nodiscard]] const XLREF12& ref(WORD i = 0) const
        {
            ensure(i < count(), "ref index out of range");
            return derived().ref_impl(i);
        }

        /// Read-write access to the i-th XLREF12 block (0-based).
        XLREF12& ref(WORD i = 0)
        {
            ensure(i < count(), "ref index out of range");
            return derived().ref_impl(i);
        }

        // ---- Convenience row/col accessors (0-based block index) ----

        /// First row of the i-th block (0-based row index).
        [[nodiscard]] RW  row_first(WORD i = 0) const { return ref(i).rwFirst; }
        /// Last row of the i-th block (0-based row index, inclusive).
        [[nodiscard]] RW  row_last (WORD i = 0) const { return ref(i).rwLast;  }
        /// First column of the i-th block (0-based column index).
        [[nodiscard]] COL col_first(WORD i = 0) const { return ref(i).colFirst; }
        /// Last column of the i-th block (0-based column index, inclusive).
        [[nodiscard]] COL col_last (WORD i = 0) const { return ref(i).colLast;  }

        /// Number of rows in the i-th block (at least 1 for a valid ref).
        [[nodiscard]] RW  row_count(WORD i = 0) const { return row_last(i) - row_first(i) + 1; }
        /// Number of columns in the i-th block (at least 1 for a valid ref).
        [[nodiscard]] COL col_count(WORD i = 0) const { return col_last(i) - col_first(i) + 1; }
        /// Total number of cells in the i-th block.
        [[nodiscard]] std::size_t cell_count(WORD i = 0) const
        {
            return static_cast<std::size_t>(row_count(i)) * static_cast<std::size_t>(col_count(i));
        }

    protected:
        /// Compares the XLREF12 blocks of `*this` against `other` (sheet ID excluded).
        /// Intended for use inside derived-class `operator==` implementations.
        [[nodiscard]] bool refs_equal(const TDerived& other) const noexcept
        {
            const WORD n = derived().count_impl();
            if (n != other.count_impl()) return false;
            for (WORD i = 0; i < n; ++i) {
                const XLREF12& a = derived().ref_impl(i);
                const XLREF12& b = other.ref_impl(i);
                if (a.rwFirst  != b.rwFirst  || a.rwLast  != b.rwLast ||
                    a.colFirst != b.colFirst || a.colLast != b.colLast)
                    return false;
            }
            return true;
        }
    };

}    // namespace xll::impl

