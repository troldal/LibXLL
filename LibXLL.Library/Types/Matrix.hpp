/**
 * @file Matrix.hpp
 * @brief Excel-compatible owning dense floating-point matrix type.
 *
 * Defines `xll::Matrix`, the owning counterpart to `xll::MatrixView`.
 * Element data is held in a `std::vector<double>`, giving the class full
 * value semantics with no ABI constraints.
 *
 * **Implicit conversion from MatrixView**
 *
 * `xll::Matrix` provides a non-explicit constructor that accepts a
 * `const xll::MatrixView&`, deep-copying all elements into its vector.
 * This means any `MatrixView` (or a dereferenced `FP12*`) can be passed
 * wherever a `Matrix` is expected:
 *
 * @code
 * void process(xll::Matrix m);
 *
 * xll::Any* __stdcall MyFunc(xll::MatrixView* view)
 * {
 *     process(*view);   // implicit MatrixView → Matrix deep copy
 * }
 * @endcode
 *
 * **Value semantics**
 *
 * Copy construction/assignment perform a deep copy of the element vector.
 * Move construction/assignment transfer the vector in O(1).
 */

#pragma once

#include "MatrixView.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace xll
{
    /**
     * @brief An owning dense floating-point matrix.
     *
     * Stores dimensions and a flat row-major `std::vector<double>` of
     * `rows() * cols()` elements.  Implicitly constructible from
     * `xll::MatrixView`, which performs a deep copy of the view's data.
     *
     * @see xll::MatrixView
     */
    class Matrix
    {
        int32_t             rows_    {0};
        int32_t             columns_ {0};
        std::vector<double> data_;

    public:
        /// Unsigned type used for indexing.
        using size_type = std::size_t;

        // -----------------------------------------------------------------------
        // Constructors / destructor
        // -----------------------------------------------------------------------

        /// Default constructor — constructs an empty (0×0) matrix.
        Matrix() = default;

        /**
         * @brief Constructs a `rows × cols` matrix with all elements set to
         *        @p fill.
         *
         * If either dimension is ≤ 0 the matrix is left empty.
         *
         * @param rows  Number of rows.
         * @param cols  Number of columns.
         * @param fill  Initial value for every element (default `0.0`).
         *
         * @throws std::bad_alloc if the vector allocation fails.
         */
        Matrix(int32_t rows, int32_t cols, double fill = 0.0)
            : rows_(rows), columns_(cols),
              data_(rows > 0 && cols > 0
                        ? static_cast<size_type>(rows) * static_cast<size_type>(cols)
                        : 0u,
                    fill)
        {}

        /**
         * @brief Implicit conversion from `MatrixView` — deep copy.
         *
         * Copies all `view.size()` elements from the contiguous `FP12`
         * buffer into the internal vector.
         *
         * @param view  The source view to copy from.
         * @throws std::bad_alloc if the vector allocation fails.
         */
        Matrix(const MatrixView& view)
            : rows_(view.rows()), columns_(view.cols()),
              data_(view.data(), view.data() + view.size())
        {}

        Matrix(const Matrix&)            = default;
        Matrix(Matrix&&) noexcept        = default;
        Matrix& operator=(const Matrix&) = default;
        Matrix& operator=(Matrix&&) noexcept = default;
        ~Matrix()                        = default;

        // -----------------------------------------------------------------------
        // Dimension queries
        // -----------------------------------------------------------------------

        /// Number of rows.
        [[nodiscard]] int32_t rows() const noexcept { return rows_; }

        /// Number of columns.
        [[nodiscard]] int32_t cols() const noexcept { return columns_; }

        /**
         * @brief Total number of elements (`rows() * cols()`).
         */
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }

        /**
         * @brief Returns `true` if the matrix contains no elements.
         */
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

        // -----------------------------------------------------------------------
        // Raw data access
        // -----------------------------------------------------------------------

        /// Returns a pointer to the first element of the flat row-major storage.
        [[nodiscard]] double*       data() noexcept       { return data_.data(); }

        /// Returns a const pointer to the first element.
        [[nodiscard]] const double* data() const noexcept { return data_.data(); }

        // -----------------------------------------------------------------------
        // View conversion
        // -----------------------------------------------------------------------

        /**
         * @brief Creates a heap-allocated `MatrixView` whose contents are a
         *        copy of this matrix.
         *
         * Allocates a contiguous FP12-compatible buffer via
         * `MatrixView::create()` and copies all elements from the internal
         * vector into it. The returned `UniquePtr` owns the allocation; call
         * `.release()` to hand the raw `FP12*` to Excel.
         *
         * Returns `nullptr` if the matrix is empty.
         *
         * @throws std::bad_alloc if the allocation fails.
         *
         * @code
         * xll::Matrix m(2, 3, 1.0);
         * auto view = m.make_view();
         * return reinterpret_cast<FP12*>(view.release()); // return to Excel
         * @endcode
         */
        [[nodiscard]] MatrixView::UniquePtr make_view() const
        {
            if (empty()) return nullptr;
            auto view = MatrixView::create(rows_, columns_);
            std::copy(data_.begin(), data_.end(), view->data());
            return view;
        }

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
         * xll::Matrix m(3, 4, 0.0);
         * m[1, 2] = 42.0;           // write
         * double v = m[1, 2];       // read
         * @endcode
         */
        template<typename Self>
        constexpr auto& operator[](this Self&& self, size_type row, size_type col)
        {
            if (row >= static_cast<size_type>(self.rows_) ||
                col >= static_cast<size_type>(self.columns_))
                throw std::out_of_range("xll::Matrix index out of range");
            return self.data_[row * static_cast<size_type>(self.columns_) + col];
        }
    };

}    // namespace xll

