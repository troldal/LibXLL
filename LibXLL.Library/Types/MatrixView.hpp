/**
 * @file MatrixView.hpp
 * @brief Excel-compatible dense floating-point matrix view type.
 *
 * Defines `xll::MatrixView`, a thin wrapper around the Excel `FP12` struct that
 * adds a C++23 multi-index subscript operator and convenience dimension
 * queries, while preserving the exact binary layout of `FP12`.
 *
 * **Binary layout**
 *
 * `xll::MatrixView` replicates the `FP12` member layout directly and adds no
 *
 * @code
 * static_assert(offsetof(xll::MatrixView, rows)    == offsetof(FP12, rows));
 * static_assert(offsetof(xll::MatrixView, columns) == offsetof(FP12, columns));
 * static_assert(offsetof(xll::MatrixView, array)   == offsetof(FP12, array));
 * @endcode
 *
 * A pointer to a `MatrixView` may therefore be `reinterpret_cast`ed to `FP12*`
 * (and vice-versa) safely.  When Excel calls an XLL function and passes an
 * `FP12*`, the pointer can be cast directly to `MatrixView*`:
 *
 * @code
 * extern "C" __declspec(dllexport)
 * xll::Any* __stdcall MyFunc(xll::MatrixView* m)
 * {
 *     double v = (*m)[0, 1];   // element at row 0, column 1
 *     // ...
 * }
 * @endcode
 *
 * **Element access**
 *
 * Elements are stored in row-major order at `array_[row * cols() + col]`.
 * The declared size of the internal array is 1, but the actual allocation
 * is large enough for all `rows() * cols()` elements — accessing beyond
 * `array_[0]` is standard XLL practice.
 *
 * **Heap allocation and returning to Excel**
 *
 * Because the element array is variable-length, a `MatrixView` cannot be
 * constructed on the stack. Use the static `create()` factory, which
 * allocates a single contiguous buffer of exactly
 * `offsetof(MatrixView, array_) + rows * cols * sizeof(double)` bytes,
 * constructs the object via placement-new, and returns a `UniquePtr`:
 *
 * @code
 * xll::Any* __stdcall MultiplyByTwo(xll::MatrixView* in)
 * {
 *     static xll::MatrixView::UniquePtr result;
 *     result = xll::MatrixView::create(in->rows(), in->cols());
 *     for (int r = 0; r < in->rows(); ++r)
 *         for (int c = 0; c < in->cols(); ++c)
 *             (*result)[r, c] = (*in)[r, c] * 2.0;
 *     return reinterpret_cast<FP12*>(result.get());
 * }
 * @endcode
 *
 * If Excel takes ownership of the returned pointer (via `xlAutoFree12`),
 * call `result.release()` before returning and free with
 * `MatrixView::destroy()` inside `xlAutoFree12`.
 */

#pragma once

#include "../ExcelSDK/xlcall.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace xll
{
    /**
     * @brief A layout-compatible wrapper around `FP12`.
     *
     * Replicates the `FP12` member layout (`int32_t rows_`, `int32_t columns_`,
     * `double array_[1]`) directly, without inheriting from `FP12`. Provides a
     * C++23 multi-index subscript operator, dimension-query helpers, and a
     * heap-allocation factory (`create()`) for constructing matrices that
     * can be returned to Excel.
     *
     * @see FP12
     */
    class MatrixView
    {
        int32_t rows_;
        int32_t columns_;
        double  array_[1];

        /// Private — used only by create(). Sets dimensions; elements are
        /// filled by create() after placement-new.
        MatrixView(int32_t rows, int32_t cols) noexcept
            : rows_(rows), columns_(cols)
        {}

    public:
        using size_type = std::size_t;

        MatrixView(const MatrixView&)            = delete;
        MatrixView(MatrixView&&)                 = delete;
        MatrixView& operator=(const MatrixView&) = delete;
        MatrixView& operator=(MatrixView&&)      = delete;

        // -----------------------------------------------------------------------
        // Heap allocation
        // -----------------------------------------------------------------------

        /**
         * @brief Deleter for use with `UniquePtr`.
         *
         * Calls `::operator delete`. The destructor of `MatrixView` is trivial
         * so no explicit destructor call is needed.
         */
        struct Deleter
        {
            void operator()(MatrixView* ptr) const noexcept { ::operator delete(ptr); }
        };

        /// Owning pointer type returned by `create()`.
        using UniquePtr = std::unique_ptr<MatrixView, Deleter>;

        /**
         * @brief Allocates a heap-resident `rows × cols` matrix.
         *
         * Allocates a single contiguous buffer of size
         * `offsetof(MatrixView, array_) + rows * cols * sizeof(double)`,
         * constructs a `MatrixView` at its base via placement-new, fills every
         * element with @p fill, and returns the result as a `UniquePtr`.
         *
         * The buffer layout is binary-identical to an `FP12` of the same
         * dimensions, so `reinterpret_cast<FP12*>(ptr.get())` is safe.
         *
         * To hand ownership to Excel (e.g. for `xlAutoFree12`), call
         * `.release()` on the `UniquePtr` and later free with `destroy()`.
         *
         * Returns `nullptr` if either dimension is ≤ 0.
         *
         * @param rows  Number of rows.
         * @param cols  Number of columns.
         * @param fill  Initial value for every element (default `0.0`).
         * @throws std::bad_alloc if the allocation fails.
         */
        [[nodiscard]] static UniquePtr create(int32_t rows, int32_t cols, double fill = 0.0)
        {
            if (rows <= 0 || cols <= 0) return nullptr;

            const size_type n     = static_cast<size_type>(rows) * static_cast<size_type>(cols);
            const size_type bytes = offsetof(MatrixView, array_) + n * sizeof(double);

            void* buf  = ::operator new(bytes);
            auto* view = ::new(buf) MatrixView(rows, cols);
            std::fill_n(view->array_, n, fill);
            return UniquePtr(view);
        }

        /**
         * @brief Frees a `MatrixView` previously released from a `UniquePtr`.
         *
         * Use this inside `xlAutoFree12` when Excel owns the pointer:
         * @code
         * extern "C" void xlAutoFree12(xloper12* p)
         * {
         *     xll::MatrixView::destroy(reinterpret_cast<xll::MatrixView*>(p));
         * }
         * @endcode
         */
        static void destroy(MatrixView* ptr) noexcept { ::operator delete(ptr); }

        // -----------------------------------------------------------------------
        // Dimension queries
        // -----------------------------------------------------------------------

        [[nodiscard]] int32_t   rows()  const noexcept { return rows_; }
        [[nodiscard]] int32_t   cols()  const noexcept { return columns_; }
        [[nodiscard]] size_type size()  const noexcept { return static_cast<size_type>(rows_) * static_cast<size_type>(columns_); }
        [[nodiscard]] bool      empty() const noexcept { return size() == 0; }

        [[nodiscard]] double*       data() noexcept       { return array_; }
        [[nodiscard]] const double* data() const noexcept { return array_; }

        // -----------------------------------------------------------------------
        // Element access
        // -----------------------------------------------------------------------

        /**
         * @brief Accesses an element by (row, col) using the C++23
         *        multi-index subscript operator.
         *
         * Elements are stored in row-major order: element `[r, c]` is at
         * flat index `r * cols() + c`.
         *
         * Deducing-this provides a single implementation for both the
         * const and non-const cases.
         *
         * @tparam Self  Deduced type of `*this`; determines const-ness of
         *               the returned reference.
         *
         * @param row  Zero-based row index in `[0, rows())`.
         * @param col  Zero-based column index in `[0, cols())`.
         * @return     Reference to the element (`const` when `*this` is const).
         *
         * @throws std::out_of_range if @p row or @p col is out of range.
         *
         * @code
         * xll::MatrixView* m = reinterpret_cast<xll::MatrixView*>(fp12_ptr);
         * double v   = (*m)[0, 2];   // read
         * (*m)[1, 0] = 99.0;         // write
         * @endcode
         */
        template<typename Self>
        constexpr auto& operator[](this Self&& self, size_type row, size_type col)
        {
            if (row >= static_cast<size_type>(self.rows_) ||
                col >= static_cast<size_type>(self.columns_))
                throw std::out_of_range("xll::MatrixView index out of range");
            return self.array_[row * static_cast<size_type>(self.columns_) + col];
        }

    private:
        static void verify_layout_() noexcept
        {
            static_assert(offsetof(MatrixView, rows_)    == offsetof(FP12, rows));
            static_assert(offsetof(MatrixView, columns_) == offsetof(FP12, columns));
            static_assert(offsetof(MatrixView, array_)   == offsetof(FP12, array));
        }
    };

}    // namespace xll

