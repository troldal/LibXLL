/**
 * @file MatrixBuffer.hpp
 * @brief Excel-compatible dense floating-point matrix buffer type.
 *
 * Defines `xll::MatrixBuffer`, a non-constructible wrapper around the Excel
 * SDK `FP12` struct that adds a C++23 multi-index subscript operator and
 * convenience dimension queries, while preserving the exact binary layout of
 * `FP12`.
 *
 * @par Ownership and construction
 * `xll::MatrixBuffer` objects cannot be constructed directly by client code.
 * Only `xll::Matrix` (declared as a friend) may allocate and destroy them.
 * Client code encounters `MatrixBuffer` in two ways:
 *
 * 1. **As a UDF parameter** — Excel passes its internal `FP12*` data; the
 *    pointer is cast to `MatrixBuffer*` in the function signature.  Because
 *    `FP12` and `XLOPER12` are unrelated types, the return type must also be
 *    `xll::MatrixBuffer*` (or `FP12*`) — never `xll::Any*` or any other
 *    `XLOPER12`-derived type:
 *    @code
 *    XLL_FUNCTION xll::MatrixBuffer* XLLAPI Scale(xll::MatrixBuffer* in, double factor)
 *    {
 *        thread_local xll::Matrix result;
 *        result = xll::Matrix(*in);          // deep copy via implicit conversion
 *        for (int r = 0; r < result.rows(); ++r)
 *            for (int c = 0; c < result.cols(); ++c)
 *                result[r, c] *= factor;
 *        return result.get();
 *    }
 *    @endcode
 *
 * 2. **Via `xll::Matrix::get()`** — returns a pointer to the matrix's
 *    internally-owned `MatrixBuffer`, ready to be `reinterpret_cast`ed to
 *    `FP12*` and handed back to Excel (no copy required):
 *    @code
 *    thread_local xll::Matrix result;
 *    result = xll::Matrix(3, 4, 1.0);
 *    return reinterpret_cast<FP12*>(result.get());
 *    @endcode
 *
 * @par Binary layout
 * `xll::MatrixBuffer` replicates the `FP12` member layout directly:
 * @code
 * static_assert(offsetof(xll::MatrixBuffer, rows_)    == offsetof(FP12, rows));
 * static_assert(offsetof(xll::MatrixBuffer, columns_) == offsetof(FP12, columns));
 * static_assert(offsetof(xll::MatrixBuffer, array_)   == offsetof(FP12, array));
 * @endcode
 * A pointer to a `MatrixBuffer` may therefore be `reinterpret_cast`ed to
 * `FP12*` (and vice-versa) without undefined behaviour.
 *
 * @par Element access
 * Elements are stored in row-major order at `array_[row * cols() + col]`.
 * The declared size of the internal array is 1, but the actual allocation
 * is large enough to hold all `rows() * cols()` elements.
 *
 * @see xll::Matrix
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
     * @brief A layout-compatible, non-constructible wrapper around `FP12`.
     *
     * Replicates the `FP12` member layout (`int32_t rows_`, `int32_t columns_`,
     * `double array_[1]`) without inheriting from `FP12`.  Provides a C++23
     * multi-index subscript operator and dimension-query helpers.
     *
     * Construction is restricted to `xll::Matrix` (declared as a friend).
     * Client code uses `MatrixBuffer` only by pointer — either received from
     * Excel as a UDF parameter, or obtained from `xll::Matrix::get()`.
     *
     * @par Non-copyable, non-movable
     * Copy and move operations are deleted because `MatrixBuffer` uses a
     * flexible-array-member trick (`double array_[1]` backed by a larger
     * allocation) that makes value-semantic copying unsafe without knowing
     * the true allocation size.  Use `xll::Matrix` for owned, copyable
     * matrices.
     *
     * @see xll::Matrix
     * @see FP12
     */
    class MatrixBuffer
    {
        // xll::Matrix is the sole class permitted to allocate and destroy
        // MatrixBuffer objects.
        friend class Matrix;

        // -----------------------------------------------------------------------
        // Private data
        // -----------------------------------------------------------------------

        int32_t rows_;
        int32_t columns_;
        double  array_[1]{};

        /// Private constructor — dimensions only; elements are filled by create().
        MatrixBuffer(int32_t rows, int32_t cols) noexcept
            : rows_(rows), columns_(cols)
        {}

        // -----------------------------------------------------------------------
        // Private ownership types
        // -----------------------------------------------------------------------

        struct Deleter
        {
            void operator()(MatrixBuffer* ptr) const noexcept { ::operator delete(ptr); }
        };

        using UniquePtr = std::unique_ptr<MatrixBuffer, Deleter>;

        // -----------------------------------------------------------------------
        // Private factory and destructor
        // -----------------------------------------------------------------------

        /**
         * @brief Allocates a heap-resident `rows × cols` matrix buffer.
         *
         * Allocates a single contiguous buffer of size
         * `offsetof(MatrixBuffer, array_) + rows * cols * sizeof(double)`,
         * constructs a `MatrixBuffer` at its base via placement-new, fills
         * every element with @p fill, and returns the result as a `UniquePtr`.
         *
         * The buffer layout is binary-identical to an `FP12` of the same
         * dimensions, so `reinterpret_cast<FP12*>(ptr.get())` is safe.
         *
         * Returns `nullptr` if either dimension is ≤ 0.
         *
         * @throws std::bad_alloc if the allocation fails.
         */
        [[nodiscard]] static UniquePtr create(int32_t rows, int32_t cols, double fill = 0.0)
        {
            if (rows <= 0 || cols <= 0) return nullptr;

            const size_type n     = static_cast<size_type>(rows) * static_cast<size_type>(cols);
            const size_type bytes = offsetof(MatrixBuffer, array_) + n * sizeof(double);

            // Wrap the raw allocation immediately. Deleter only calls ::operator delete,
            // so it is safe to hold unconstructed memory.
            UniquePtr result(static_cast<MatrixBuffer*>(::operator new(bytes)));
            ::new(result.get()) MatrixBuffer(rows, cols);
            std::fill_n(result->data(), n, fill);
            return result;
        }

        /**
         * @brief Frees a `MatrixBuffer` that was previously released from a
         *        `UniquePtr` via `.release()`.
         *
         * Use this when ownership of the raw pointer has been transferred
         * outside the `UniquePtr` and must be returned to the heap:
         * @code
         * MatrixBuffer::UniquePtr buf = MatrixBuffer::create(3, 4);
         * MatrixBuffer* raw = buf.release();   // UniquePtr no longer owns it
         * // ... hand raw to something else ...
         * MatrixBuffer::destroy(raw);          // clean up when done
         * @endcode
         *
         * @note `xlAutoFree12` is the Excel callback for freeing `XLOPER12`
         *       (`xll::Any`) results — it is unrelated to `MatrixBuffer` /
         *       `FP12`.  Do not cast between these two unrelated pointer types.
         */
        static void destroy(MatrixBuffer* ptr) noexcept { ::operator delete(ptr); }

        // -----------------------------------------------------------------------
        // Layout verification (compile-time, private)
        // -----------------------------------------------------------------------

        static void verify_layout_() noexcept
        {
            static_assert(offsetof(MatrixBuffer, rows_)    == offsetof(FP12, rows));
            static_assert(offsetof(MatrixBuffer, columns_) == offsetof(FP12, columns));
            static_assert(offsetof(MatrixBuffer, array_)   == offsetof(FP12, array));
        }

    public:
        /**
         * @brief Unsigned integer type used for sizes and indices.
         *
         * Matches `std::size_t`, consistent with standard library containers
         * and the `size_type` of `xll::Matrix`.
         */
        using size_type = std::size_t;

        /// @name Deleted special members
        /// @{

        /**
         * @brief Copy constructor — deleted.
         *
         * `MatrixBuffer` uses a flexible-array-member layout whose true size
         * is known only at allocation time.  Copying by value is therefore
         * not safe.  Use `xll::Matrix` (which performs a correct deep copy
         * via `MatrixBuffer::create`) for owned, copyable matrices.
         */
        MatrixBuffer(const MatrixBuffer&)            = delete;

        /**
         * @brief Move constructor — deleted.
         *
         * Ownership of a `MatrixBuffer` is managed exclusively by
         * `xll::Matrix` through its internal `UniquePtr`.  Direct moves
         * are not supported.
         */
        MatrixBuffer(MatrixBuffer&&)                 = delete;

        /**
         * @brief Copy assignment — deleted.
         * @see MatrixBuffer(const MatrixBuffer&)
         */
        MatrixBuffer& operator=(const MatrixBuffer&) = delete;

        /**
         * @brief Move assignment — deleted.
         * @see MatrixBuffer(MatrixBuffer&&)
         */
        MatrixBuffer& operator=(MatrixBuffer&&)      = delete;

        /**
         * @brief Scalar `delete` — deleted.
         *
         * Prevents accidental `delete ptr` on a `MatrixBuffer*`.  The DLL
         * is solely responsible for buffer lifetime: buffers must be released
         * through `xll::Matrix` (which uses a custom `Deleter` that calls
         * `::operator delete` on the raw allocation).  Excel does **not**
         * call `xlAutoFree12` to free `FP12` / `MatrixBuffer` results.
         */
        void operator delete(void*) = delete;

        /**
         * @brief Sized `delete` — deleted.
         * @see operator delete(void*)
         */
        void operator delete(void*, std::size_t) = delete;

        /// @}

        // -----------------------------------------------------------------------
        // Dimension queries
        // -----------------------------------------------------------------------

        /**
         * @brief Returns the number of rows in the buffer.
         * @return Number of rows as set at allocation time.
         */
        [[nodiscard]] int32_t rows() const noexcept { return rows_; }

        /**
         * @brief Returns the number of columns in the buffer.
         * @return Number of columns as set at allocation time.
         */
        [[nodiscard]] int32_t cols() const noexcept { return columns_; }

        /**
         * @brief Returns the total number of elements (`rows() * cols()`).
         * @return Total element count.
         */
        [[nodiscard]] size_type size() const noexcept { return static_cast<size_type>(rows_) * static_cast<size_type>(columns_); }

        /**
         * @brief Returns `true` if the buffer contains no elements.
         *
         * A `MatrixBuffer` obtained from `MatrixBuffer::create()` is never
         * empty (the factory returns `nullptr` for non-positive dimensions).
         * This accessor exists for symmetry with `xll::Matrix` and for buffers
         * received from Excel that may have zero-sized dimensions.
         *
         * @return `true` if `size() == 0`, `false` otherwise.
         */
        [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        // -----------------------------------------------------------------------
        // Raw data access
        // -----------------------------------------------------------------------

        /**
         * @brief Returns a pointer to the first element in row-major flat storage.
         *
         * The memory is a single contiguous allocation of `size()` doubles.
         * Element `[r, c]` is at offset `r * cols() + c`.
         *
         * @return Mutable pointer to the first element of the flat array.
         */
        [[nodiscard]] double* data() noexcept { return array_; }

        /**
         * @brief Returns a const pointer to the first element in row-major flat storage.
         *
         * @return Read-only pointer to the first element of the flat array.
         */
        [[nodiscard]] const double* data() const noexcept { return array_; }

        // -----------------------------------------------------------------------
        // Element access
        // -----------------------------------------------------------------------

        /**
         * @brief Accesses a single element via the C++23 multi-index subscript
         *        operator.
         *
         * Elements are stored in row-major order: `[r, c]` maps to flat index
         * `r * cols() + c`.  Deducing-this provides a single implementation
         * for both mutable and const access — the returned reference inherits
         * the const-ness of the `MatrixBuffer`.
         *
         * @tparam Self  Deduced type of `*this`; determines whether the
         *               returned reference is `double&` or `const double&`.
         *
         * @param row  Zero-based row index in `[0, rows())`.
         * @param col  Zero-based column index in `[0, cols())`.
         *
         * @return `double&` when called on a mutable `MatrixBuffer`,
         *         `const double&` when called on a const `MatrixBuffer`.
         *
         * @throws std::out_of_range if @p row ≥ rows() or @p col ≥ cols().
         *
         * @code
         * xll::MatrixBuffer* m = reinterpret_cast<xll::MatrixBuffer*>(fp12_ptr);
         * double v   = (*m)[0, 2];   // read
         * (*m)[1, 0] = 99.0;         // write
         * @endcode
         */
        template<typename Self>
        constexpr auto& operator[](this Self&& self, size_type row, size_type col)
        {
            if (row >= static_cast<size_type>(self.rows_) ||
                col >= static_cast<size_type>(self.columns_))
                throw std::out_of_range("xll::MatrixBuffer index out of range");
            return self.array_[row * static_cast<size_type>(self.columns_) + col];
        }
    };

}    // namespace xll

