//
// Created by kenne on 25/04/2025.
//

/**
 * @file MultiRef.hpp
 * @brief Excel-compatible multi-block external range reference for the xll library.
 *
 * Defines `xll::MultiRef`, a concrete type wrapping the `xltypeRef` variant
 * of `XLOPER12`.  It can reference **one or more** rectangular blocks of cells
 * on a **specified sheet** (identified by an `IDSHEET`), which may or may not
 * be the current sheet.  For this reason an `xltypeRef` is also known as an
 * *external reference*.
 *
 * **Memory layout**
 *
 * `MultiRef` inherits from `impl::RefBase<MultiRef, xltypeRef>`, which itself
 * inherits from `XLOPER12` without adding any data members, so
 * `sizeof(MultiRef) == sizeof(XLOPER12)`.
 *
 * **Heap allocation**
 *
 * `XLOPER12::val.mref.lpmref` points to a heap-allocated `XLMREF12` that
 * holds the variable-length array of `XLREF12` blocks.  `MultiRef` owns this
 * memory and frees it in the destructor.  Copy construction/assignment
 * performs a deep copy; move construction/assignment transfers ownership.
 *
 * **Interface**
 *
 * The common interface is inherited from `impl::RefBase`:
 * `count()`, `ref(i)`, `row_first(i)`, …, `cell_count(i)`, `is_valid()`.
 * `MultiRef` additionally exposes:
 *
 * - `sheet_id()` / `set_sheet_id(id)` — get/set the internal sheet identifier.
 *
 * @code
 * // Single-block external reference
 * xll::MultiRef r(sheetId, 0, 9, 0, 3);   // rows 0-9, cols 0-3
 *
 * // Multi-block external reference
 * XLREF12 blocks[] = { {0,4,0,1}, {6,9,0,1} };
 * xll::MultiRef r2(sheetId, blocks);
 * r2.row_first(0);   // 0
 * r2.row_first(1);   // 6
 * @endcode
 *
 * @see xll::SingleRef
 * @see impl::RefBase
 */

#pragma once

#include "RefBase.hpp"
#include "SheetId.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Excel multi-block external range reference (`xltypeRef`).
     *
     * Wraps `XLOPER12::val.mref`, which holds a pointer to a heap-allocated
     * `XLMREF12` (containing a variable-length array of `XLREF12` blocks) and
     * an `IDSHEET` identifying the target sheet.
     *
     * @note `sizeof(xll::MultiRef) == sizeof(XLOPER12)` is a hard invariant.
     *
     * @note The `XLMREF12*` stored in `val.mref.lpmref` is allocated with
     *       `std::malloc` / freed with `std::free` for compatibility with the
     *       Excel SDK memory conventions.
     */
    class MultiRef : public impl::RefBase<MultiRef, xltypeRef>
    {
        using BASE = impl::RefBase<MultiRef, xltypeRef>;
        friend BASE;

        // ---- Private helpers ----

        /// Allocates an XLMREF12 with room for @p n XLREF12 entries and sets count.
        /// Returns nullptr when n == 0.
        static XLMREF12* allocate(WORD n)
        {
            if (n == 0) return nullptr;
            // XLMREF12 already contains one XLREF12 slot (reftbl[1]).
            auto* p = static_cast<XLMREF12*>(
                std::malloc(sizeof(XLMREF12) + static_cast<std::size_t>(n - 1) * sizeof(XLREF12)));
            if (!p) throw std::bad_alloc();
            p->count = n;
            return p;
        }

        /// Frees the XLMREF12 and nulls the pointer.
        void free_mref() noexcept
        {
            std::free(val.mref.lpmref);
            val.mref.lpmref = nullptr;
        }

        // ---- CRTP primitives (private, accessible by BASE via friend) ----

        [[nodiscard]] WORD count_impl() const noexcept
        {
            return val.mref.lpmref ? val.mref.lpmref->count : WORD{0};
        }

        [[nodiscard]] const XLREF12& ref_impl(WORD i) const noexcept
        {
            return val.mref.lpmref->reftbl[i];
        }

        [[nodiscard]] XLREF12& ref_impl(WORD i) noexcept
        {
            return val.mref.lpmref->reftbl[i];
        }

    public:
        /**
         * @brief Default constructor — no sheet, no reference blocks.
         *
         * `val.mref.lpmref` is `nullptr` and `val.mref.idSheet` is 0.
         * `count()` returns 0.
         *
         * @post `is_valid() == true`
         * @post `count() == 0`
         */
        MultiRef() : BASE()
        {
            val.mref.lpmref  = nullptr;
            val.mref.idSheet = 0;
        }

        /**
         * @brief Constructs an empty reference associated with a given sheet.
         *
         * The resulting object has `count() == 0`.  Useful as a starting point
         * when the sheet is known but no specific range is needed yet.
         *
         * @param sheetId  Sheet identifier — accepts `xll::SheetId` or a raw `IDSHEET`.
         */
        explicit MultiRef(SheetId sheetId) : MultiRef()
        {
            val.mref.idSheet = sheetId;
        }

        /**
         * @brief Constructs a single-block reference on the given sheet.
         *
         * @param sheetId  Sheet identifier — accepts `xll::SheetId` or a raw `IDSHEET`.
         * @param rwFirst  0-based index of the first row.
         * @param rwLast   0-based index of the last row  (must be >= rwFirst).
         * @param colFirst 0-based index of the first column.
         * @param colLast  0-based index of the last column (must be >= colFirst).
         *
         * @throws std::bad_alloc if heap allocation fails.
         */
        MultiRef(SheetId sheetId, RW rwFirst, RW rwLast, COL colFirst, COL colLast) : BASE()
        {
            val.mref.lpmref          = allocate(1);
            val.mref.idSheet         = sheetId;
            XLREF12& r               = val.mref.lpmref->reftbl[0];
            r.rwFirst  = rwFirst;
            r.rwLast   = rwLast;
            r.colFirst = colFirst;
            r.colLast  = colLast;
        }

        /**
         * @brief Constructs a multi-block reference from a span of XLREF12 blocks.
         *
         * @param sheetId  Sheet identifier — accepts `xll::SheetId` or a raw `IDSHEET`.
         * @param refs     Contiguous range of XLREF12 blocks to copy.
         *
         * @throws std::out_of_range if `refs.size()` exceeds `WORD` maximum.
         * @throws std::bad_alloc    if heap allocation fails.
         */
        MultiRef(SheetId sheetId, std::span<const XLREF12> refs) : BASE()
        {
            if (refs.size() > static_cast<std::size_t>(std::numeric_limits<WORD>::max()))
                throw std::out_of_range("MultiRef: too many ref blocks");
            const WORD n     = static_cast<WORD>(refs.size());
            val.mref.lpmref  = allocate(n);
            val.mref.idSheet = sheetId;
            if (n > 0)
                std::memcpy(val.mref.lpmref->reftbl, refs.data(), n * sizeof(XLREF12));
        }

        /// Deep-copy constructor — allocates a fresh XLMREF12.
        MultiRef(const MultiRef& other) : BASE()  // NOLINT(bugprone-copy-constructor-init)
        {
            val.mref.idSheet = other.val.mref.idSheet;
            const WORD n     = other.count_impl();
            val.mref.lpmref  = allocate(n);
            if (n > 0)
                std::memcpy(val.mref.lpmref->reftbl,
                            other.val.mref.lpmref->reftbl,
                            n * sizeof(XLREF12));
        }

        /// Move constructor — transfers XLMREF12 ownership without allocation.
        MultiRef(MultiRef&& other) noexcept : BASE()
        {
            val.mref.lpmref       = other.val.mref.lpmref;
            val.mref.idSheet      = other.val.mref.idSheet;
            other.val.mref.lpmref  = nullptr;
            other.val.mref.idSheet = 0;
        }

        /// Destructor — frees the heap-allocated XLMREF12.
        ~MultiRef() { free_mref(); }

        /// Deep-copy assignment (copy-and-swap).
        MultiRef& operator=(const MultiRef& other)
        {
            if (this != &other) {
                MultiRef tmp(other);
                swap(*this, tmp);
            }
            return *this;
        }

        /// Move assignment — transfers ownership without allocation.
        MultiRef& operator=(MultiRef&& other) noexcept
        {
            if (this != &other) {
                free_mref();
                val.mref.lpmref        = other.val.mref.lpmref;
                val.mref.idSheet       = other.val.mref.idSheet;
                other.val.mref.lpmref  = nullptr;
                other.val.mref.idSheet = 0;
            }
            return *this;
        }

        // ---- Sheet identity ----

        /// Returns the sheet identifier for this reference as an `xll::SheetId`.
        [[nodiscard]] SheetId sheet_id() const noexcept
        {
            return SheetId(val.mref.idSheet);
        }

        /// Sets the sheet identifier.  Accepts `xll::SheetId` or a raw `IDSHEET`.
        void set_sheet_id(SheetId id) noexcept { val.mref.idSheet = id; }

        // ---- Equality and swap ----

        /**
         * @brief Two `MultiRef` objects are equal when their sheet IDs are
         *        the same and all XLREF12 blocks match in order.
         */
        friend bool operator==(const MultiRef& lhs, const MultiRef& rhs) noexcept
        {
            return lhs.sheet_id() == rhs.sheet_id() && lhs.refs_equal(rhs);
        }

        /// ADL-enabled swap — exchanges sheet IDs and XLMREF12 pointers directly.
        friend void swap(MultiRef& lhs, MultiRef& rhs) noexcept
        {
            using std::swap;
            swap(lhs.xltype,          rhs.xltype);
            swap(lhs.val.mref.lpmref,  rhs.val.mref.lpmref);
            swap(lhs.val.mref.idSheet, rhs.val.mref.idSheet);
        }
    };

}    // namespace xll

