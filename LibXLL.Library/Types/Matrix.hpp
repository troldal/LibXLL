/**
 * @file Matrix.hpp
 * @brief Excel-compatible owning dense floating-point matrix type.
 *
 * Defines `xll::Matrix`, the owning counterpart to `xll::MatrixBuffer`.
 * Element data is stored inside a heap-allocated `xll::MatrixBuffer`, whose
 * binary layout is identical to the Excel SDK `FP12` struct.  This means a
 * result matrix can be handed back to Excel with zero additional copying:
 * simply call `.get()` on a `thread_local` `Matrix`.
 *
 * @par Quick-start UDF example
 * A UDF that receives and returns a matrix operates entirely in the `FP12`
 * world.  `xll::MatrixBuffer` is binary-compatible with `FP12`, so the
 * function signature uses `xll::MatrixBuffer*` for both the parameter and
 * the return type.  It must never be cast to or from `xll::Any` or any other
 * `XLOPER12`-derived type.
 * @code
 * thread_local xll::Matrix result;
 *
 * xll::MatrixBuffer* __stdcall MultiplyByTwo(xll::MatrixBuffer* in)
 * {
 *     result = xll::Matrix(in->rows(), in->cols());
 *     for (int r = 0; r < in->rows(); ++r)
 *         for (int c = 0; c < in->cols(); ++c)
 *             result[r, c] = (*in)[r, c] * 2.0;
 *     return result.get();   // MatrixBuffer* is reinterpret_cast-safe to FP12*
 * }
 * @endcode
 *
 * @par Implicit conversion from MatrixBuffer
 * `xll::Matrix` provides a non-explicit constructor accepting a
 * `const xll::MatrixBuffer&`, deep-copying all elements into a freshly
 * allocated internal buffer.  Any `MatrixBuffer` (or a dereferenced `FP12*`)
 * can therefore be passed wherever a `Matrix` is expected:
 * @code
 * void process(xll::Matrix m);
 *
 * xll::Any* __stdcall MyFunc(xll::MatrixBuffer* buf)
 * {
 *     process(*buf);   // implicit MatrixBuffer → Matrix deep copy
 * }
 * @endcode
 *
 * @par Value semantics
 * Copy construction/assignment perform a deep copy of the element data.
 * Move construction/assignment transfer the internal buffer in O(1) without
 * allocating or copying any elements.
 *
 * @par Range support
 * `xll::Matrix` satisfies `std::ranges::contiguous_range`.  The flat
 * row-major storage is exposed through raw `double*` iterators, giving the
 * strongest possible iterator category (`std::contiguous_iterator`) and
 * making every `std::ranges` algorithm usable directly on a `Matrix` object.
 * Individual rows (contiguous) and columns (strided) can also be selected as
 * sub-ranges via `row()` and `col()`.
 *
 * @par Thread safety
 * `xll::Matrix` provides no synchronisation.  The idiomatic pattern for
 * returning results to Excel is to give each UDF a dedicated `thread_local`
 * instance, which avoids any sharing between concurrent calls.
 *
 * @see xll::MatrixBuffer
 */

#pragma once

#include "MatrixBuffer.hpp"

#include <algorithm>
#include <cstdint>
#if __has_include(<mdspan>)
#  include <mdspan>
#elif __has_include(<experimental/mdspan>)
#  include <experimental/mdspan>
#endif
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace xll
{
    /**
     * @brief An owning dense floating-point matrix backed by a `MatrixBuffer`.
     *
     * Stores all element data in a single heap-allocated `xll::MatrixBuffer`
     * that is binary-identical to the Excel SDK `FP12` struct.  This design
     * gives full value semantics (copy = deep copy, move = O(1) pointer
     * transfer) while allowing the result to be returned to Excel without any
     * intermediate copy buffer.
     *
     * The matrix is empty (0 × 0) when default-constructed or when constructed
     * with non-positive dimensions.  All accessors return well-defined
     * zero/null/empty results for an empty matrix; they do not throw.
     *
     * @par Element layout
     * Elements are stored in row-major order.  Element `[r, c]` is at flat
     * index `r * cols() + c` in the contiguous array returned by `data()`.
     *
     * @par Iterator / range model
     * `begin()` / `end()` iterate over all elements in row-major order.
     * `row(r)` returns a `std::span<double>` over the contiguous elements of
     * a single row.  `col(c)` returns a lazy strided range over a single
     * column.  Both support `std::ranges` algorithms.
     *
     * @see xll::MatrixBuffer
     */
    class Matrix
    {
        MatrixBuffer::UniquePtr buffer_;

    public:
        /**
         * @brief Unsigned integer type used for sizes and indices.
         *
         * Matches `std::size_t`, consistent with standard library containers
         * and the `size_type` of `xll::MatrixBuffer`.
         */
        using size_type = std::size_t;

        // -----------------------------------------------------------------------
        // Constructors / destructor
        // -----------------------------------------------------------------------

        /**
         * @brief Default constructor — constructs an empty (0 × 0) matrix.
         *
         * No heap allocation is performed.  `empty()` returns `true`,
         * `data()` returns `nullptr`, and all dimension queries return 0.
         */
        Matrix() = default;

        /**
         * @brief Constructs a `rows × cols` matrix with all elements set to
         *        @p fill.
         *
         * Allocates a single contiguous `xll::MatrixBuffer` large enough for
         * `rows * cols` doubles and initialises every element to @p fill.
         * If either dimension is ≤ 0 the matrix is left empty (no allocation).
         *
         * @param rows  Number of rows (ignored if ≤ 0).
         * @param cols  Number of columns (ignored if ≤ 0).
         * @param fill  Value written to every element.  Defaults to `0.0`.
         *
         * @throws std::bad_alloc if the underlying allocation fails.
         */
        Matrix(int32_t rows, int32_t cols, double fill = 0.0)
            : buffer_(MatrixBuffer::create(rows, cols, fill))
        {}

        /**
         * @brief Implicit conversion from `MatrixBuffer` — deep copy.
         *
         * Allocates a new `MatrixBuffer` with the same dimensions as @p buf
         * and copies all `buf.size()` elements into it.  This constructor is
         * intentionally non-`explicit` so that a `MatrixBuffer*` received
         * from Excel can be dereferenced and passed directly wherever a
         * `Matrix` is expected.
         *
         * @param buf  Source buffer to copy from.  Must remain valid for the
         *             duration of the copy (the caller retains ownership).
         *
         * @throws std::bad_alloc if the allocation fails.
         *
         * @see xll::MatrixBuffer
         */
        Matrix(const MatrixBuffer& buf)
            : buffer_(MatrixBuffer::create(buf.rows(), buf.cols()))
        {
            if (buffer_)
                std::copy(buf.data(), buf.data() + buf.size(), buffer_->data());
        }

        /**
         * @brief Copy constructor — deep copy of all elements.
         *
         * Allocates a new `MatrixBuffer` and copies every element from
         * @p other.  If @p other is empty the constructed matrix is also
         * empty and no allocation is performed.
         *
         * @param other  Matrix to copy from.
         * @throws std::bad_alloc if the allocation fails.
         */
        Matrix(const Matrix& other)
        {
            if (!other.empty()) {
                buffer_ = MatrixBuffer::create(other.rows(), other.cols());
                std::copy(other.data(), other.data() + other.size(), buffer_->data());
            }
        }

        /**
         * @brief Copy assignment — deep copy of all elements.
         *
         * Strongly exception-safe: allocates a fresh buffer, copies all
         * elements, and only then replaces the existing buffer.  If the
         * allocation throws, `*this` is left unchanged.
         *
         * Self-assignment is handled safely and is a no-op.
         *
         * @param other  Matrix to copy from.
         * @return       Reference to `*this`.
         * @throws std::bad_alloc if the allocation fails.
         */
        Matrix& operator=(const Matrix& other)
        {
            if (this != &other) {
                if (other.empty()) {
                    buffer_.reset();
                } else {
                    auto new_buf = MatrixBuffer::create(other.rows(), other.cols());
                    std::copy(other.data(), other.data() + other.size(), new_buf->data());
                    buffer_ = std::move(new_buf);
                }
            }
            return *this;
        }

        /**
         * @brief Move constructor — transfers the internal buffer in O(1).
         *
         * After the move, @p other is left in a valid empty state:
         * `other.empty()` returns `true` and `other.data()` returns
         * `nullptr`.  No allocation or element copy is performed.
         *
         * @param other  Matrix to move from.
         */
        Matrix(Matrix&&) noexcept = default;

        /**
         * @brief Move assignment — transfers the internal buffer in O(1).
         *
         * The previously owned buffer of `*this` is released before the
         * transfer.  After the move, @p other is left in a valid empty state.
         * No allocation or element copy is performed.
         *
         * @param other  Matrix to move from.
         * @return       Reference to `*this`.
         */
        Matrix& operator=(Matrix&&) noexcept = default;

        /**
         * @brief Destructor — releases the underlying `MatrixBuffer`.
         *
         * The DLL is solely responsible for the lifetime of any `FP12` /
         * `MatrixBuffer` result handed to Excel.  Unlike `XLOPER12` results,
         * Excel does **not** call `xlAutoFree12` to signal that an `FP12`
         * buffer may be freed.  The recommended pattern is `thread_local`
         * storage: the buffer remains valid for the duration of every Excel
         * call on that thread and is released only when the thread exits.
         */
        ~Matrix() = default;

        // -----------------------------------------------------------------------
        // Dimension queries
        // -----------------------------------------------------------------------

        /**
         * @brief Returns the number of rows in the matrix.
         * @return Number of rows, or `0` if the matrix is empty.
         */
        [[nodiscard]] int32_t rows() const noexcept { return buffer_ ? buffer_->rows() : 0; }

        /**
         * @brief Returns the number of columns in the matrix.
         * @return Number of columns, or `0` if the matrix is empty.
         */
        [[nodiscard]] int32_t cols() const noexcept { return buffer_ ? buffer_->cols() : 0; }

        /**
         * @brief Returns the total number of elements (`rows() * cols()`).
         * @return Total element count, or `0` if the matrix is empty.
         */
        [[nodiscard]] size_type size() const noexcept { return buffer_ ? buffer_->size() : 0u; }

        /**
         * @brief Returns `true` if the matrix contains no elements.
         *
         * A matrix is empty when default-constructed or when constructed
         * with at least one non-positive dimension.
         *
         * @return `true` if the matrix is empty, `false` otherwise.
         */
        [[nodiscard]] bool empty() const noexcept { return !buffer_; }

        // -----------------------------------------------------------------------
        // Raw data access
        // -----------------------------------------------------------------------

        /**
         * @brief Returns a pointer to the first element in row-major flat storage.
         *
         * The pointed-to memory is a single contiguous array of `size()`
         * doubles laid out in row-major order.  The pointer is valid for the
         * lifetime of the `Matrix` (and its internal buffer).
         *
         * @return Mutable pointer to the first element, or `nullptr` if empty.
         */
        [[nodiscard]] double* data() noexcept { return buffer_ ? buffer_->data() : nullptr; }

        /**
         * @brief Returns a const pointer to the first element in row-major flat storage.
         *
         * @return Read-only pointer to the first element, or `nullptr` if empty.
         */
        [[nodiscard]] const double* data() const noexcept { return buffer_ ? buffer_->data() : nullptr; }

        // -----------------------------------------------------------------------
        // Iterators — satisfies std::ranges::contiguous_range
        //
        // Iteration visits all elements in row-major (flat) order.  The
        // iterator type is a raw double*, giving the strongest possible
        // iterator category (std::contiguous_iterator) and making every
        // std::ranges algorithm (fill, sort, transform, …) usable directly
        // on a Matrix object.
        // -----------------------------------------------------------------------

        /**
         * @brief Returns an iterator to the first element.
         * @return Mutable pointer to the first element, or `nullptr` if empty.
         */
        [[nodiscard]] double* begin() noexcept { return data(); }

        /**
         * @brief Returns an iterator one past the last element.
         * @return Mutable pointer one past the last element, or `nullptr` if empty.
         */
        [[nodiscard]] double* end() noexcept { return data() + size(); }

        /**
         * @brief Returns a const iterator to the first element.
         * @return Read-only pointer to the first element, or `nullptr` if empty.
         */
        [[nodiscard]] const double* begin() const noexcept { return data(); }

        /**
         * @brief Returns a const iterator one past the last element.
         * @return Read-only pointer one past the last element, or `nullptr` if empty.
         */
        [[nodiscard]] const double* end() const noexcept { return data() + size(); }

        /**
         * @brief Returns a const iterator to the first element.
         * @return Read-only pointer to the first element, or `nullptr` if empty.
         */
        [[nodiscard]] const double* cbegin() const noexcept { return data(); }

        /**
         * @brief Returns a const iterator one past the last element.
         * @return Read-only pointer one past the last element, or `nullptr` if empty.
         */
        [[nodiscard]] const double* cend() const noexcept { return data() + size(); }

        // -----------------------------------------------------------------------
        // Buffer access
        // -----------------------------------------------------------------------

        /**
         * @brief Returns a raw pointer to the underlying `MatrixBuffer`.
         *
         * The returned pointer is binary-identical to `FP12*` and may be
         * `reinterpret_cast`ed directly to hand the matrix back to Excel
         * without any additional copy:
         * @code
         * thread_local xll::Matrix result;
         * result = xll::Matrix(3, 4, 1.0);
         * return reinterpret_cast<FP12*>(result.get());
         * @endcode
         *
         * @return Raw pointer to the `MatrixBuffer`, or `nullptr` if empty.
         *
         * @warning The pointer is valid only for the lifetime of this `Matrix`
         *          object.  Always use `thread_local` (or `static`) storage
         *          when returning the pointer to Excel, so that the `Matrix`
         *          outlives the call.
         *
         * @see xll::MatrixBuffer
         */
        [[nodiscard]] MatrixBuffer* get() const noexcept { return buffer_.get(); }

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
         * the const-ness of the `Matrix` object.
         *
         * @tparam Self  Deduced type of `*this`; determines whether the
         *               returned reference is `double&` or `const double&`.
         *
         * @param row  Zero-based row index in `[0, rows())`.
         * @param col  Zero-based column index in `[0, cols())`.
         *
         * @return `double&` for a mutable `Matrix`, `const double&` for a
         *         const `Matrix`.
         *
         * @throws std::out_of_range if the matrix is empty or if either
         *         index is out of range.
         *
         * @code
         * xll::Matrix m(3, 4, 0.0);
         * m[1, 2] = 42.0;           // write
         * double v = m[1, 2];       // read  → 42.0
         * @endcode
         */
        template<typename Self>
        constexpr auto& operator[](this Self&& self, size_type row, size_type col)
        {
            if (!self.buffer_)
                throw std::out_of_range("xll::Matrix is empty");

            using QualifiedBuffer = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const MatrixBuffer,
                MatrixBuffer>;

            return static_cast<QualifiedBuffer&>(*self.buffer_)[row, col];
        }

        // -----------------------------------------------------------------------
        // Row / column views
        // -----------------------------------------------------------------------

        /**
         * @brief Returns a contiguous view of row @p r as a `std::span`.
         *
         * Because elements are stored in row-major order, each row occupies a
         * contiguous block of exactly `cols()` doubles.  The returned
         * `std::span` is a `std::contiguous_range`, so every `std::ranges`
         * algorithm that works on the full matrix works equally on a single row:
         * @code
         * for (double& v : m.row(2))       v *= 2.0;
         * std::ranges::fill(m.row(0), 0.0);
         * auto it = std::ranges::find(m.row(1), 42.0);
         * @endcode
         *
         * @tparam Self  Deduced type of `*this`; determines element
         *               mutability.  A const `Matrix` yields
         *               `std::span<const double>`.
         *
         * @param r  Zero-based row index in `[0, rows())`.
         *
         * @return `std::span<double>` for a mutable `Matrix`, or
         *         `std::span<const double>` for a const `Matrix`, over the
         *         `cols()` elements of row @p r.
         *
         * @throws std::out_of_range if the matrix is empty or @p r ≥ rows().
         *
         * @see col()
         */
        template<typename Self>
        [[nodiscard]] auto row(this Self&& self, size_type r)
        {
            if (!self.buffer_ || r >= static_cast<size_type>(self.rows()))
                throw std::out_of_range("xll::Matrix row index out of range");

            using Elem = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>, const double, double>;

            const auto n_cols = static_cast<size_type>(self.cols());
            return std::span<Elem>(self.data() + r * n_cols, n_cols);
        }

        /**
         * @brief Returns a lazy range over the elements of column @p c.
         *
         * Column elements are not contiguous in row-major storage — they are
         * spaced `cols()` elements apart.  The returned range is a lazy
         * `std::ranges::transform_view` over `std::views::iota(0, rows())`
         * that maps each row index to the corresponding element by reference.
         * It yields exactly `rows()` elements and supports range-based `for`
         * and most `std::ranges` algorithms:
         * @code
         * for (double& v : m.col(1))                   v = 0.0;
         * auto s = std::ranges::fold_left(m.col(0), 0.0, std::plus{});
         * @endcode
         *
         * @tparam Self  Deduced type of `*this`; determines element
         *               mutability.  A const `Matrix` yields `const double&`
         *               from the range.
         *
         * @param c  Zero-based column index in `[0, cols())`.
         *
         * @return A `std::ranges::transform_view` whose reference type is
         *         `double&` for a mutable `Matrix`, or `const double&` for
         *         a const `Matrix`.
         *
         * @throws std::out_of_range if the matrix is empty or @p c ≥ cols().
         *
         * @note `std::ranges::count_if` and similar algorithms return
         *       `range_difference_t` for the view.  Because the underlying
         *       `iota_view<size_t, size_t>` uses `__int128` as its
         *       `difference_type` on 64-bit Linux, assigning the result to
         *       `auto` and streaming it directly to `std::cout` produces an
         *       ambiguous-overload error.  Cast to `std::ptrdiff_t` before
         *       printing:
         * @code
         * auto n = std::ranges::count_if(m.col(1), [](double x){ return x > 0; });
         * std::cout << static_cast<std::ptrdiff_t>(n) << '\n';
         * @endcode
         *
         * @see row()
         */
        template<typename Self>
        [[nodiscard]] auto col(this Self&& self, size_type c)
        {
            if (!self.buffer_ || c >= static_cast<size_type>(self.cols()))
                throw std::out_of_range("xll::Matrix column index out of range");

            using Elem = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>, const double, double>;

            const auto    n_rows = static_cast<size_type>(self.rows());
            const auto    n_cols = static_cast<size_type>(self.cols());
            Elem* const   base   = self.data() + c;
            // iota produces row indices 0..n_rows-1; transform maps each to
            // the corresponding column element via pointer arithmetic.
            // decltype(auto) in transform_view preserves the Elem& reference,
            // so mutation through the range works for non-const Matrix.
            return std::views::iota(size_type{0}, n_rows)
                   | std::views::transform([base, n_cols](size_type i) -> Elem& {
                         return *(base + i * n_cols);
                     });
        }

        // -----------------------------------------------------------------------
        // 2-D mdspan view  (requires <mdspan> or <experimental/mdspan>)
        // -----------------------------------------------------------------------

#if __has_include(<mdspan>) || __has_include(<experimental/mdspan>)
        /**
         * @brief Returns a 2-D `std::mdspan` view over the entire matrix.
         *
         * The mdspan wraps the same storage as the `Matrix` — no allocation
         * or element copy is performed.  It can be passed to any algorithm
         * or library that accepts an mdspan, and provides 2-D subscripting
         * via the multi-index `operator[]`:
         * @code
         * auto view = m.as_mdspan();
         * for (size_t r = 0; r < view.extent(0); ++r)
         *     for (size_t c = 0; c < view.extent(1); ++c)
         *         view[r, c] *= 2.0;
         * @endcode
         *
         * @tparam Self  Deduced type of `*this`; determines element
         *               mutability.  A const `Matrix` produces
         *               `std::mdspan<const double, ...>`.
         *
         * @return `std::mdspan<double, std::dextents<size_type, 2>>` for a
         *         mutable `Matrix`, or the `const double` variant for a
         *         const `Matrix`.  When the matrix is empty the returned
         *         mdspan has zero extents and a null data pointer.
         *
         * @note This method is conditionally compiled: it is present only
         *       when `<mdspan>` (C++23, GCC ≥ 14 / Clang ≥ 18 / MSVC 19.38+)
         *       or `<experimental/mdspan>` (Kokkos reference implementation)
         *       is available.  When using the reference implementation the
         *       types live in `std::experimental` rather than `std`.
         *
         * @see get()
         * @see row()
         * @see col()
         */
        template<typename Self>
        [[nodiscard]] auto as_mdspan(this Self&& self) noexcept
        {
            using Elem = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>, const double, double>;

            // Normalise between the C++23 standard header and the reference
            // implementation, which puts its types in std::experimental.
#  if __has_include(<mdspan>)
            namespace mdns = std;
#  else
            namespace mdns = std::experimental;
#  endif
            return mdns::mdspan<Elem, mdns::dextents<size_type, 2>>(
                self.data(),
                static_cast<size_type>(self.rows()),
                static_cast<size_type>(self.cols()));
        }
#endif // __has_include(<mdspan>) || __has_include(<experimental/mdspan>)
    };

}    // namespace xll

