/**
 * @file SingleRef.hpp
 * @brief Excel-compatible single-block range reference for the xll library.
 *
 * Defines `xll::SingleRef`, a concrete type wrapping the `xltypeSRef` variant
 * of `XLOPER12`.  It references exactly **one** rectangular block of cells on
 * the **current** sheet; no sheet identifier is stored.
 *
 * **Memory layout**
 *
 * `SingleRef` inherits from `impl::RefBase<SingleRef, xltypeSRef>`, which
 * itself inherits from `XLOPER12` without adding any data members.  The
 * object therefore occupies exactly `sizeof(XLOPER12)` bytes and can be
 * passed directly to and from the Excel C API without conversion.
 *
 * **No heap allocation**
 *
 * All data (`val.sref.count` and `val.sref.ref`) live inside the `XLOPER12`
 * union; no dynamic memory is used.  Copy, move, and destruction all follow
 * the compiler-generated defaults.
 *
 * **Interface**
 *
 * In addition to the common interface inherited from `impl::RefBase`
 * (`count()`, `ref()`, `row_first()`, `row_last()`, `col_first()`,
 * `col_last()`, `row_count()`, `col_count()`, `cell_count()`, `is_valid()`),
 * `SingleRef` provides compile-time first/last selectors via the `Cell` enum:
 *
 * @code
 * xll::SingleRef r(0, 4, 0, 2);  // rows 0-4, cols 0-2 on the current sheet
 * auto rf = r.row<xll::SingleRef::Cell::First>();  // 0
 * auto rl = r.row<xll::SingleRef::Cell::Last>();   // 4
 * // Or via the base-class helpers:
 * r.row_first();   // 0
 * r.row_last();    // 4
 * @endcode
 *
 * @see xll::MultiRef
 * @see impl::RefBase
 */

#pragma once

#include "RefBase.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Excel single-block range reference on the current sheet (`xltypeSRef`).
     *
     * Wraps `XLOPER12::val.sref`, which holds a fixed `WORD count` (always 1)
     * and a single `XLREF12` block.  Can only reference ranges on the sheet
     * from which the DLL function is currently being called.
     *
     * @note `sizeof(xll::SingleRef) == sizeof(XLOPER12)` is a hard invariant.
     */
    class SingleRef : public impl::RefBase<SingleRef, xltypeSRef>
    {
        using BASE = impl::RefBase<SingleRef, xltypeSRef>;
        friend BASE;

        // ---- CRTP primitives (private, accessible by BASE via friend) ----

        [[nodiscard]] WORD count_impl() const noexcept { return val.sref.count; }

        [[nodiscard]] const XLREF12& ref_impl(WORD /*i*/) const noexcept { return val.sref.ref; }
        [[nodiscard]]       XLREF12& ref_impl(WORD /*i*/)       noexcept { return val.sref.ref; }

    public:
        /// Compile-time selector for the first or last row/column of the block.
        enum class Cell { First, Last };

        /**
         * @brief Default constructor — an empty reference at (0,0)–(0,0).
         *
         * Sets `xltype = xltypeSRef`, `val.sref.count = 1`, and zeroes all
         * row/column fields.
         *
         * @post `is_valid() == true`
         * @post `count() == 1`
         */
        SingleRef() : BASE()
        {
            val.sref.count       = 1;
            val.sref.ref.rwFirst  = 0;
            val.sref.ref.rwLast   = 0;
            val.sref.ref.colFirst = 0;
            val.sref.ref.colLast  = 0;
        }

        /**
         * @brief Constructs a reference covering the block
         *        [rwFirst..rwLast] × [colFirst..colLast] on the current sheet.
         *
         * @param rwFirst  0-based index of the first row.
         * @param rwLast   0-based index of the last row  (must be >= rwFirst).
         * @param colFirst 0-based index of the first column.
         * @param colLast  0-based index of the last column (must be >= colFirst).
         */
        SingleRef(RW rwFirst, RW rwLast, COL colFirst, COL colLast) : SingleRef()
        {
            val.sref.ref.rwFirst  = rwFirst;
            val.sref.ref.rwLast   = rwLast;
            val.sref.ref.colFirst = colFirst;
            val.sref.ref.colLast  = colLast;
        }

        // Compiler-generated copy/move/destructor are correct (no heap).

        // ---- Compile-time first/last accessors (Cell enum) ----

        /**
         * @brief Returns the first or last row index of the block.
         * @tparam C `Cell::First` for the first row, `Cell::Last` for the last.
         */
        template<Cell C>
        [[nodiscard]] RW row() const noexcept
        {
            if constexpr (C == Cell::First) return val.sref.ref.rwFirst;
            else                            return val.sref.ref.rwLast;
        }

        /**
         * @brief Returns the first or last column index of the block.
         * @tparam C `Cell::First` for the first column, `Cell::Last` for the last.
         */
        template<Cell C>
        [[nodiscard]] COL col() const noexcept
        {
            if constexpr (C == Cell::First) return val.sref.ref.colFirst;
            else                            return val.sref.ref.colLast;
        }

        // ---- Equality and swap ----

        /// Two `SingleRef` objects are equal when their XLREF12 blocks are identical.
        friend bool operator==(const SingleRef& lhs, const SingleRef& rhs) noexcept
        {
            return lhs.refs_equal(rhs);
        }

        /// ADL-enabled swap (no heap allocation: direct XLOPER12 member swap).
        friend void swap(SingleRef& lhs, SingleRef& rhs) noexcept
        {
            using std::swap;
            swap(lhs.xltype, rhs.xltype);
            swap(lhs.val,    rhs.val);
        }
    };

}    // namespace xll

