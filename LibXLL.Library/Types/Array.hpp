//
// Created by kenne on 24/03/2025.
//

#pragma once

#include <fxt.hpp>
#include "Expected.hpp"
#include "Variant.hpp"
#include <expected>
#include <span>

namespace xll
{

/**
 * @brief A type-safe, Excel-compatible two-dimensional array.
 *
 * `xll::Array<TValue>` represents the `xltypeMulti` variant of `XLOPER12` —
 * Excel's native two-dimensional array type. It inherits directly from
 * `XLOPER12` so that its address can be passed to and from the Excel C API
 * without conversion or copying.
 *
 * **Memory layout**
 *
 * The class adds no data members beyond `XLOPER12`. Element storage is a
 * heap-allocated array of `XLOPER12` objects, each of which is constructed
 * in-place as a `TValue` via `std::construct_at`. The invariant
 * `sizeof(TValue) == sizeof(XLOPER12)` is enforced by `TValue`'s own
 * static assertions.
 *
 * **Invariants**
 *
 * - `xltype` is *always* `xltypeMulti` for the lifetime of the object.
 * - `val.array.lparray` is `nullptr` if and only if `rows * cols == 0`.
 * - All elements in `[lparray, lparray + rows * cols)` are live `TValue`
 *   objects whose lifetime is managed by this class.
 *
 * **Shape**
 *
 * The logical shape of the array is described by the nested `Shape`
 * type-enum (`fxt::type_enum<Empty, Singular, Horizontal, Vertical,
 * TwoDimensional>`). Shape tags are also used as constructor arguments to
 * select the desired layout when constructing from a range or initializer
 * list:
 *
 * @code
 * // 1-row array sized to fit the initializer list
 * Array<Number> h { {1.0, 2.0, 3.0}, Array<Number>::Horizontal{} };
 *
 * // 1-column array with explicit size and fill padding
 * Array<Number> v { {1.0, 2.0}, Array<Number>::Vertical(5), Number{-1.0} };
 *
 * // 2×3 matrix from a std::vector<double>
 * std::vector<double> src { 1, 2, 3, 4, 5, 6 };
 * Array<Number> m { src, Array<Number>::TwoDimensional{2, 3} };
 * @endcode
 *
 * **Element type requirements**
 *
 * `TValue` must:
 * - Inherit from `XLOPER12` without adding data members
 *   (`sizeof(TValue) == sizeof(XLOPER12)`).
 * - Be default-constructible, copy-constructible, and destructible.
 *
 * `xll::Number`, `xll::String`, `xll::Bool`, `xll::Int`, and
 * `xll::Error` all satisfy these requirements.
 *
 * @tparam TValue The element type stored in the array. Must be an
 *                `xll::impl::Base`-derived type with the same size and
 *                alignment as `XLOPER12`.
 *
 * @see xll::Number
 * @see xll::String
 */
    template<typename TValue>
    class Array : public XLOPER12
    {
        struct ShapeBase{};
    public:
        /// The element type of this array.
        using value_type      = TValue;
        /// Unsigned size type.
        using size_type       = size_t;
        /// Signed difference type for iterator arithmetic.
        using difference_type = std::ptrdiff_t;
        /// Pointer to element.
        using pointer         = TValue*;
        /// Pointer to const element.
        using const_pointer   = const TValue*;
        /// Reference to element.
        using reference       = TValue&;
        /// Reference to const element.
        using const_reference = const TValue&;
        /// Iterator type (contiguous, raw pointer).
        using iterator        = TValue*;
        /// Const iterator type.
        using const_iterator  = const TValue*;

        /// The XLOPER12 type tag for all Array specialisations.
        static constexpr int excel_type = xltypeMulti;

        // -----------------------------------------------------------------------
        // Shape tags
        // -----------------------------------------------------------------------

        /**
         * @brief Shape tag for a single-row array.
         *
         * When used as a constructor argument, produces an array with `rows == 1`
         * and `cols == N`.
         *
         * @code
         * // Auto-fit: cols == values.size()
         * Array<Number> h { {1.0, 2.0, 3.0}, Array<Number>::Horizontal{} };
         *
         * // Explicit size with fill padding
         * Array<Number> h2 { {1.0}, Array<Number>::Horizontal(5), Number{0.0} };
         * @endcode
         */
        struct Horizontal : ShapeBase {
            /// Optional explicit column count. If absent, the size is inferred
            /// from the source range.
            std::optional<size_t> size;
            /// Constructs a Horizontal tag with auto-fit size.
            constexpr Horizontal() = default;
            /// Constructs a Horizontal tag requesting exactly @p n columns.
            constexpr explicit Horizontal(size_t n) : size(n) {}
        };

        /**
         * @brief Shape tag for a single-column array.
         *
         * When used as a constructor argument, produces an array with `cols == 1`
         * and `rows == N`.
         *
         * @code
         * // Auto-fit: rows == values.size()
         * Array<Number> v { {1.0, 2.0, 3.0}, Array<Number>::Vertical{} };
         *
         * // Explicit size with fill padding
         * Array<Number> v2 { {1.0}, Array<Number>::Vertical(5), Number{-1.0} };
         * @endcode
         */
        struct Vertical : ShapeBase {
            /// Optional explicit row count. If absent, the size is inferred
            /// from the source range.
            std::optional<size_t> size;
            /// Constructs a Vertical tag with auto-fit size.
            constexpr Vertical() = default;
            /// Constructs a Vertical tag requesting exactly @p n rows.
            constexpr explicit Vertical(size_t n) : size(n) {}
        };

        /**
         * @brief Shape tag for a matrix with an explicit row and column count.
         *
         * When used as a constructor argument, produces a `rows × cols` matrix.
         * There is no default constructor — both dimensions must be supplied.
         *
         * @code
         * // Exact fit: 2 rows × 3 cols from 6 values
         * Array<Number> m { {1.0,2.0,3.0,4.0,5.0,6.0},
         *                   Array<Number>::TwoDimensional{2, 3} };
         *
         * // Padded: 2 rows × 3 cols, 2 values supplied, rest filled with -1
         * Array<Number> m2 { {1.0, 2.0},
         *                    Array<Number>::TwoDimensional{2, 3},
         *                    Number{-1.0} };
         * @endcode
         */
        struct TwoDimensional : ShapeBase {
            size_t rows; ///< Number of rows.
            size_t cols; ///< Number of columns.
            /// Constructs a TwoDimensional tag with the given dimensions.
            constexpr TwoDimensional(size_t rows, size_t cols) : rows(rows), cols(cols) {}
        };

        /**
         * @brief Shape tag returned by `shape()` when the array has no elements.
         *
         * An array is empty when `rows() * cols() == 0`, which is the state
         * immediately after default construction or after assignment from
         * `xll::Missing`.
         */
        struct Empty : ShapeBase {};

        /**
         * @brief Shape tag returned by `shape()` when the array has exactly one element.
         *
         * A 1×1 array is considered singular regardless of whether it was
         * constructed as Horizontal, Vertical, or TwoDimensional.
         */
        struct Singular : ShapeBase {};

        /**
         * @brief Type-safe enumeration of all possible array shapes.
         *
         * `Shape` is a `fxt::type_enum` whose variants are `Empty`, `Singular`,
         * `Horizontal`, `Vertical`, and `TwoDimensional`. It is returned by
         * `shape()` and can be inspected with `.is<T>()` or dispatched with
         * `.visit(visitor)`:
         *
         * @code
         * arr.shape().visit(fxt::overload{
         *     [](const Array<Number>::Horizontal& s) { ... },
         *     [](const Array<Number>::Vertical&   s) { ... },
         *     [](const auto&)                        { ... }  // catch-all
         * });
         * @endcode
         */
        using Shape = fxt::type_enum<Empty, Singular, Horizontal, Vertical, TwoDimensional>;

        // -----------------------------------------------------------------------
        // Constructors
        // -----------------------------------------------------------------------

        /**
         * @brief Default constructor — constructs an empty array.
         *
         * Sets `xltype = xltypeMulti`, `lparray = nullptr`, `rows = 0`,
         * `cols = 0`. This is the canonical empty state; `shape()` returns
         * `Empty{}`.
         *
         * @post `xltype == xltypeMulti`
         * @post `empty() == true`
         * @post `val.array.lparray == nullptr`
         */
        constexpr Array() : XLOPER12()
        {
            xltype            = xltypeMulti;
            val.array.lparray = nullptr;
            val.array.rows    = 0;
            val.array.columns = 0;
        }

        /**
         * @brief Constructs a `rows × cols` array, each element initialised to @p v.
         *
         * @param rows Number of rows. Must not exceed the Excel row limit (`RW` max).
         * @param cols Number of columns. Must not exceed the Excel column limit (`COL` max).
         * @param v    Fill value used to initialise every element. Defaults to
         *             a value-initialised `TValue`.
         *
         * @post `this->rows() == rows` (if `rows * cols > 0`)
         * @post `this->cols() == cols` (if `rows * cols > 0`)
         * @post `empty() == (rows * cols == 0)`
         *
         * @throws std::overflow_error  if `rows * cols` overflows `size_t`.
         * @throws std::out_of_range    if @p rows or @p cols exceeds the
         *                              corresponding Excel limit.
         * @throws std::bad_alloc       if heap allocation fails.
         */
        constexpr Array(size_t rows, size_t cols, TValue v = {}) : Array()
        {
            // same validation as Array(rows, cols)
            if (rows != 0 && cols > std::numeric_limits<size_t>::max() / rows)
                throw std::overflow_error("Array dimensions overflow");
            if (rows > static_cast<size_t>(std::numeric_limits<RW>::max()))
                throw std::out_of_range("Row count exceeds Excel limit");
            if (cols > static_cast<size_t>(std::numeric_limits<COL>::max()))
                throw std::out_of_range("Column count exceeds Excel limit");
            if (rows * cols == 0) return;

            val.array.lparray = make_array(rows * cols, v).release();
            val.array.rows    = static_cast<RW>(rows);
            val.array.columns = static_cast<COL>(cols);
        }

        /**
         * @brief Constructs an array from a forward range of `TValue` elements
         *        with an optional shape tag and fill value.
         *
         * The shape tag controls the resulting array geometry:
         * - `Horizontal{}` — 1 row, N columns (N = range size or explicit size).
         * - `Vertical{}`   — N rows, 1 column.
         * - `TwoDimensional{r, c}` — r rows, c columns; total must be ≥ range size.
         *
         * If the explicit size (from the tag) is larger than the range, the
         * remaining elements are filled with @p fill. If it is smaller, an
         * exception is thrown.
         *
         * @tparam TRange     A `std::ranges::forward_range` whose `value_type`
         *                    is exactly `TValue`.
         * @tparam TShape     One of `Horizontal`, `Vertical`, or `TwoDimensional`.
         *
         * @param values      The source range. Must be a forward range (multi-pass).
         * @param arrayShape  Shape tag. Defaults to `Horizontal{}`.
         * @param fill        Value used to pad slots beyond the range size.
         *                    Defaults to a value-initialised `TValue`.
         *
         * @throws std::out_of_range   if the explicit size is smaller than
         *                             the range, or if a dimension exceeds
         *                             the Excel limit.
         * @throws std::overflow_error if `TwoDimensional` dimensions overflow.
         * @throws std::bad_alloc      if heap allocation fails.
         */
        template<std::ranges::forward_range TRange, typename TShape = Horizontal>
            requires (std::same_as<TShape, Horizontal> || std::same_as<TShape, Vertical> || std::same_as<TShape, TwoDimensional>)
                  && std::same_as<std::ranges::range_value_t<TRange>, TValue>
        constexpr Array(TRange&& values, TShape arrayShape = {}, TValue fill = {}) : Array()
        {
            const size_t values_size = [&]() -> size_t {
                if constexpr (std::ranges::sized_range<TRange>)
                    return std::ranges::size(values);
                else
                    return static_cast<size_t>(std::ranges::distance(values));
            }();

            auto populate = [&](size_t count) {
                auto it = std::ranges::begin(values);
                for (size_t i = 0; i < count; ++i, ++it)
                    static_cast<TValue&>(val.array.lparray[i]) = *it;
            };

            Shape(arrayShape).visit(fxt::overload {
                [&](const Horizontal& s) {
                    const size_t explicit_size = s.size.value_or(values_size);
                    if (explicit_size < values_size)
                        throw std::out_of_range("Specified array size is smaller than the range");
                    if (explicit_size == 0) return;
                    if (explicit_size > static_cast<size_t>(std::numeric_limits<COL>::max()))
                        throw std::out_of_range("Column count exceeds Excel limit");
                    val.array.lparray = make_array(explicit_size, fill).release();
                    val.array.rows    = 1;
                    val.array.columns = static_cast<COL>(explicit_size);
                    populate(values_size);
                },
                [&](const Vertical& s) {
                    const size_t explicit_size = s.size.value_or(values_size);
                    if (explicit_size < values_size)
                        throw std::out_of_range("Specified array size is smaller than the range");
                    if (explicit_size == 0) return;
                    if (explicit_size > static_cast<size_t>(std::numeric_limits<RW>::max()))
                        throw std::out_of_range("Row count exceeds Excel limit");
                    val.array.lparray = make_array(explicit_size, fill).release();
                    val.array.rows    = static_cast<RW>(explicit_size);
                    val.array.columns = 1;
                    populate(values_size);
                },
                [&](const TwoDimensional& s) {
                    const size_t total = s.rows * s.cols;
                    if (total < values_size)
                        throw std::out_of_range("Specified array size is smaller than the range");
                    if (s.rows > static_cast<size_t>(std::numeric_limits<RW>::max()))
                        throw std::out_of_range("Row count exceeds Excel limit");
                    if (s.cols > static_cast<size_t>(std::numeric_limits<COL>::max()))
                        throw std::out_of_range("Column count exceeds Excel limit");
                    if (s.rows != 0 && s.cols > std::numeric_limits<size_t>::max() / s.rows)
                        throw std::overflow_error("Array dimensions overflow");
                    if (total == 0) return;
                    val.array.lparray = make_array(total, fill).release();
                    val.array.rows    = static_cast<RW>(s.rows);
                    val.array.columns = static_cast<COL>(s.cols);
                    populate(values_size);
                },
                [](const auto&) { throw std::invalid_argument("Unsupported shape type"); }    // Empty, Singular – unreachable given the requires clause
            });
        }

        /**
         * @brief Constructs an array from a `std::initializer_list<TValue>`
         *        with an optional shape tag and fill value.
         *
         * Delegates to the range constructor via `std::ranges::subrange`.
         * See the range constructor for full semantics.
         *
         * @tparam TShape  One of `Horizontal`, `Vertical`, or `TwoDimensional`.
         *
         * @param values  Brace-enclosed list of `TValue` elements.
         * @param shape   Shape tag. Defaults to `Horizontal{}`.
         * @param fill    Fill value for padding. Defaults to `TValue{}`.
         *
         * @throws std::out_of_range   if the explicit size is smaller than the list.
         * @throws std::bad_alloc      if heap allocation fails.
         *
         * @code
         * Array<Number> h { {Number(1), Number(2), Number(3)},
         *                   Array<Number>::Horizontal{} };
         * @endcode
         */
        template<typename TShape = Horizontal>
            requires (std::same_as<TShape, Horizontal> || std::same_as<TShape, Vertical> || std::same_as<TShape, TwoDimensional>)
        constexpr Array(std::initializer_list<TValue> values, TShape shape = {}, TValue fill = {}) : Array()
        {
            *this = Array(std::ranges::subrange(values.begin(), values.end()), shape, fill);
        }

        /**
         * @brief Constructs an array from a forward range of elements
         *        convertible to `TValue`, with an optional shape tag and fill value.
         *
         * Each element `u` in the range is converted via `TValue(u)`. The
         * resulting `TValue` range is then passed to the primary range
         * constructor.
         *
         * This overload is only selected when `range_value_t<TRange>` differs
         * from `TValue`, preventing ambiguity with the primary range constructor.
         *
         * @tparam TRange  A `std::ranges::forward_range` whose `value_type` is
         *                 constructible to `TValue` but is not `TValue` itself.
         * @tparam TShape  One of `Horizontal`, `Vertical`, or `TwoDimensional`.
         *
         * @param values      Source range.
         * @param shape       Shape tag. Defaults to `Horizontal{}`.
         * @param fill        Fill value for padding. Defaults to `TValue{}`.
         *
         * @throws std::out_of_range   if the explicit size is smaller than the range.
         * @throws std::bad_alloc      if heap allocation fails.
         *
         * @code
         * std::vector<double> src { 1.0, 2.0, 3.0 };
         * Array<Number> h(src, Array<Number>::Horizontal{});
         * @endcode
         */
        template<std::ranges::forward_range TRange, typename TShape = Horizontal>
            requires (std::constructible_from<TValue, std::ranges::range_value_t<TRange>> || std::convertible_to<std::ranges::range_value_t<TRange>, TValue>) &&
                     (!std::same_as<TValue, std::ranges::range_value_t<TRange>>) &&
                     (!std::same_as<Array, std::ranges::range_value_t<TRange>>) &&
                     (std::same_as<TShape, Horizontal> || std::same_as<TShape, Vertical> || std::same_as<TShape, TwoDimensional>)
        constexpr Array(TRange&& values, TShape shape = {}, TValue fill = {}) : Array()
        {
            auto converted = values | std::views::transform([](const std::ranges::range_value_t<TRange>& u) { return TValue(u); });
            *this          = Array(converted, shape, fill);
        }

        /**
         * @brief Constructs an array from a `std::initializer_list<U>` where
         *        `U` is convertible to `TValue`, with an optional shape tag and fill value.
         *
         * Each `U` element is converted to `TValue` via `TValue(u)`. The
         * converted range is then passed to the primary range constructor.
         *
         * This overload is only selected when `U` differs from `TValue`,
         * preventing ambiguity with the `initializer_list<TValue>` overload.
         *
         * @tparam U       Element type of the initializer list. Must be
         *                 constructible to `TValue` but not `TValue` itself.
         * @tparam TShape  One of `Horizontal`, `Vertical`, or `TwoDimensional`.
         *
         * @param values  Brace-enclosed list of `U` elements.
         * @param shape   Shape tag. Defaults to `Horizontal{}`.
         * @param fill    Fill value for padding. Defaults to `TValue{}`.
         *
         * @throws std::out_of_range   if the explicit size is smaller than the list.
         * @throws std::bad_alloc      if heap allocation fails.
         *
         * @code
         * // Constructs a horizontal Array<Number> from doubles
         * Array<Number> h { 1.0, 2.0, 3.0 };
         *
         * // Constructs a vertical Array<String> from string literals
         * Array<String> v { {"foo", "bar"}, Array<String>::Vertical{} };
         * @endcode
         */
        template<typename U, typename TShape = Horizontal>
            requires (std::constructible_from<TValue, U> || std::convertible_to<U, TValue>)
                  && (!std::same_as<TValue, std::remove_cvref_t<U>>)
                  && (!std::same_as<Array, std::remove_cvref_t<U>>)
                  && (std::same_as<TShape, Horizontal> || std::same_as<TShape, Vertical> || std::same_as<TShape, TwoDimensional>)
        constexpr Array(std::initializer_list<U> values, TShape shape = {}, TValue fill = {}) : Array()
        {
            auto converted = values | std::views::transform([](const U& u) { return TValue(u); });
            *this = Array(converted, shape, fill);
        }



        /**
         * @brief Constructs an Array from a Missing value.
         *
         * This constructor creates an empty Array when given a Missing value.
         * It delegates to the default constructor, resulting in a multi-cell array
         * with zero dimensions (rows=0, columns=0).
         *
         * @param _ The Missing value (unused parameter).
         * @note This allows Arrays to be implicitly constructed from Missing values,
         *       which is useful in Excel contexts where missing arguments need to be handled.
         */
        // Array(const Missing& _) : Array() {}



        /**
         * @brief Copy constructor for the Array class.
         *
         * This constructor creates a new Array by copying the contents of another Array instance.
         * It handles three cases:
         * 1. If the source is a multi-cell array (xltypeMulti), it allocates a new buffer of the same size,
         *    copies the dimensions, and performs element-by-element copying of the array contents.
         * 2. If the source is a single value (matching TValue::excel_type), it copies the value and type
         *    using TValue's assignment operator.
         * 3. For any other type, it sets this Array to an empty state (xltypeNil).
         *
         * @param other The source Array to copy from.
         * @throws std::bad_alloc if memory allocation fails.
         */
        constexpr Array(const Array& other) : Array()
        {
            ensure(other.xltype == xltypeMulti);
            if (other.size() == 0) return;
            auto buffer = std::make_unique_for_overwrite<XLOPER12[]>(other.size());
            for (size_t i = 0; i < other.size(); ++i)
                std::construct_at(
                    static_cast<TValue*>(&buffer[i]),
                    static_cast<const TValue&>(other.val.array.lparray[i]));
            val.array.lparray = buffer.release();
            val.array.rows    = other.rows();
            val.array.columns = other.cols();
        }

        /**
         * @brief Move constructor for the Array class.
         *
         * This constructor efficiently transfers ownership of resources from another Array instance
         * without copying data. It handles three cases:
         * 1. If the source is a multi-cell array (xltypeMulti), it transfers ownership of the array buffer
         *    and dimensions, then nullifies the source's buffer pointer and zeroes its dimensions.
         * 2. If the source is a single value (matching TValue::excel_type), it copies the value and type.
         * 3. For any other type, it sets this Array to an empty state (xltypeNil).
         *
         * @param other The source Array to move from.
         * @note The source Array is left in a valid but unspecified state after the move operation.
         */
        constexpr Array(Array&& other) noexcept : Array()
        {
            ensure(other.xltype == xltypeMulti);
            val.array.lparray       = other.val.array.lparray;
            other.val.array.lparray = nullptr;
            val.array.rows          = other.rows();
            val.array.columns       = other.cols();
            other.val.array.rows    = 0;
            other.val.array.columns = 0;
        }

        /**
         * @brief Destructor for the Array class.
         *
         * This destructor properly cleans up resources depending on the Array's type:
         * 1. For multi-cell arrays (xltypeMulti with valid pointer):
         *    - Calls the destructor for each TValue element in the array
         *    - Deallocates the memory used by the array
         *    - Sets the array pointer to nullptr to prevent double deletion
         * 2. For single values (not xltypeMulti):
         *    - Calls the destructor for the TValue that this Array represents
         *    - Does not deallocate memory since the value is part of the XLOPER12 structure
         *
         * Finally, sets the type to xltypeNil to indicate the Array is empty.
         */
        constexpr ~Array()
        {
            if (xltype != xltypeMulti) std::unreachable();
            if (val.array.lparray != nullptr) {
                std::destroy(static_cast<TValue*>(val.array.lparray),
                             static_cast<TValue*>(val.array.lparray) + size());
                delete[] val.array.lparray;
                val.array.lparray = nullptr;
            }

            xltype            = xltypeMulti;
            val.array.lparray = nullptr;
            val.array.rows    = 0;
            val.array.columns = 0;
        }

        /**
         * @brief Copy assignment operator for the Array class.
         *
         * This operator creates a deep copy of another Array instance, replacing the current contents.
         * It handles three cases:
         * 1. If the source is a multi-cell array (xltypeMulti):
         *    - Cleans up any existing array data
         *    - Allocates a new buffer of the same size as the source
         *    - Copies the dimensions from the source array
         *    - Performs element-by-element copying of array contents
         * 2. If the source is a single value (matching TValue::excel_type):
         *    - Calls the destructor to clean up current resources
         *    - Copies the type from the source
         *    - Uses TValue's assignment operator to copy the value
         * 3. For any other type, it sets this Array to an empty state (xltypeNil).
         *
         * @param other The source Array to copy from.
         * @return Reference to this Array after assignment.
         * @throws std::bad_alloc if memory allocation fails.
         */
        constexpr Array& operator=(const Array& other)
        {
            Array tmp(other);          // copy constructor handles allocation + element construction
            *this = std::move(tmp);    // move assignment steals the buffer
            return *this;
        }

        /**
         * @brief Move assignment operator for the Array class.
         *
         * This operator efficiently transfers ownership of resources from another Array instance,
         * replacing the current contents without unnecessary copying. It handles three cases:
         * 1. If the source is a multi-cell array (xltypeMulti):
         *    - Calls the destructor to clean up current resources
         *    - Copies the type from the source
         *    - Transfers ownership of the array buffer from the source to this instance
         *    - Nullifies the source's buffer pointer to prevent deletion during destruction
         *    - Copies the dimensions from the source
         *    - Resets the source's dimensions to zero
         * 2. If the source is a single value (matching TValue::excel_type):
         *    - Calls the destructor to clean up current resources
         *    - Copies the type from the source
         *    - Uses TValue's assignment operator to copy the value (should be updated to use move)
         * 3. For any other type, it sets this Array to an empty state (xltypeNil).
         *
         * @param other The source Array to move from.
         * @return Reference to this Array after assignment.
         * @note The source Array is left in a valid but unspecified state after the move operation.
         */
        constexpr Array& operator=(Array&& other) noexcept
        {
            if (this == &other) return *this;
            if (other.xltype != xltypeMulti) std::unreachable();

            std::destroy_at(this);
            val.array.lparray       = other.val.array.lparray;
            val.array.rows          = other.rows();
            val.array.columns       = other.cols();
            other.val.array.lparray = nullptr;
            other.val.array.rows    = 0;
            other.val.array.columns = 0;
            return *this;
        }

        /**
         * @brief Assignment operator for Missing values.
         *
         * This operator resets the Array to an empty state when assigned a Missing value.
         * It performs the following operations:
         * - Calls the destructor to clean up any current resources
         * - Sets the type to xltypeMulti (multi-cell array)
         * - Sets the array pointer to nullptr
         * - Sets both dimensions (rows and columns) to zero
         *
         * @param _ Unused parameter representing a Missing value.
         * @return Reference to this Array after assignment.
         * @note This operator allows Arrays to be reset to empty state when
         *       assigned Missing values, which is useful in Excel contexts.
         */
        constexpr Array& operator=(const Missing&) noexcept
        {
            std::destroy_at(this);
            // destructor already resets xltype and array fields; nothing more to do
            return *this;
        }

        // -----------------------------------------------------------------------
        // Shape and dimension queries
        // -----------------------------------------------------------------------

        /**
         * @brief Returns the geometric shape of the array.
         *
         * Classifies the array into one of five shapes:
         * | Condition                     | Result          |
         * |-------------------------------|-----------------|
         * | `rows * cols == 0`            | `Empty{}`       |
         * | `rows * cols == 1`            | `Singular{}`    |
         * | `rows > 1 && cols == 1`       | `Vertical{r}`   |
         * | `rows == 1 && cols > 1`       | `Horizontal{c}` |
         * | otherwise                     | `TwoDimensional{r, c}` |
         *
         * The returned `Shape` value can be inspected with `.is<T>()` or
         * dispatched with `.visit(visitor)`.
         *
         * @return A `Shape` variant describing the current geometry.
         */
        [[nodiscard]]
        constexpr Shape shape() const
        {
            const size_t r = rows();
            const size_t c = cols();
            const size_t n = size();    // avoids repeated multiply and potential overflow
            if (n == 0) return Empty{};
            if (n == 1) return Singular{};
            if (r > 1 && c == 1) return Vertical{r};
            if (r == 1 && c > 1) return Horizontal{c};
            return TwoDimensional{r, c};
        }

        /**
         * @brief Returns the number of rows.
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return Row count as `size_t`.
         */
        [[nodiscard]]
        constexpr size_t rows() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<size_t>(val.array.rows);
        }

        /**
         * @brief Returns the number of columns.
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return Column count as `size_t`.
         */
        [[nodiscard]]
        constexpr size_t cols() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<size_t>(val.array.columns);
        }

        /**
         * @brief Returns the total number of elements (`rows * cols`).
         *
         * @pre `xltype == xltypeMulti` (checked via `std::unreachable()`).
         * @return Element count as `size_t`.
         */
        [[nodiscard]]
        constexpr size_t size() const
        {
            if (xltype != xltypeMulti) std::unreachable();
            return static_cast<size_t>(val.array.rows) * static_cast<size_t>(val.array.columns);
        }

        /**
         * @brief Returns `true` if the array contains no elements.
         *
         * Equivalent to `size() == 0`.
         *
         * @return `true` iff `rows() * cols() == 0`.
         */
        [[nodiscard]]
        constexpr bool empty() const
        {
            return size() == 0;
        }

        /**
         * @brief Changes the row/column layout without reallocating or moving elements.
         *
         * The element buffer is reinterpreted with new dimensions. The total
         * element count must remain identical (`rows * cols == size()`).
         * Elements are stored in row-major order, so after a reshape the
         * logical position of each element changes.
         *
         * @param rows New row count.
         * @param cols New column count.
         *
         * @throws std::out_of_range    if @p rows or @p cols exceeds the
         *                              corresponding Excel limit.
         * @throws std::overflow_error  if `rows * cols` overflows `size_t`.
         * @throws std::invalid_argument if `rows * cols != size()`.
         *
         * @code
         * Array<Number> arr(1, 6);   // 1×6
         * arr.reshape(2, 3);         // now 2×3, same elements
         * @endcode
         */
        constexpr void reshape(size_t rows, size_t cols)
        {
            if (rows > static_cast<size_t>(std::numeric_limits<RW>::max()))
                throw std::out_of_range("Row count exceeds Excel limit");
            if (cols > static_cast<size_t>(std::numeric_limits<COL>::max()))
                throw std::out_of_range("Column count exceeds Excel limit");
            if (rows != 0 && cols > std::numeric_limits<size_t>::max() / rows)
                throw std::overflow_error("Array dimensions overflow");
            if (rows * cols != size())
                throw std::invalid_argument("reshape: new dimensions must preserve element count");
            val.array.rows    = static_cast<RW>(rows);
            val.array.columns = static_cast<COL>(cols);
        }

        // -----------------------------------------------------------------------
        // Iterators
        // -----------------------------------------------------------------------

        /**
         * @brief Returns a pointer to the first element (non-const).
         *
         * Together with `end()`, provides a contiguous iterator range suitable
         * for range-based for loops and standard algorithms.
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `TValue*` pointing to `val.array.lparray[0]`, or `nullptr`
         *         if the array is empty.
         */
        constexpr TValue* begin()
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue*>(static_cast<XLOPER12*>(val.array.lparray));
        }

        /**
         * @brief Returns a pointer to the first element (const overload).
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `const TValue*` pointing to `val.array.lparray[0]`.
         */
        constexpr TValue const* begin() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue const*>(static_cast<XLOPER12 const*>(val.array.lparray));
        }

        /**
         * @brief Returns a pointer one past the last element (non-const).
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `TValue*` pointing one past the last element.
         */
        constexpr TValue* end()
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue*>(static_cast<XLOPER12*>(val.array.lparray)) + size();
        }

        /**
         * @brief Returns a pointer one past the last element (const overload).
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `const TValue*` pointing one past the last element.
         */
        constexpr TValue const* end() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue const*>(static_cast<XLOPER12 const*>(val.array.lparray)) + size();
        }

        /**
         * @brief Returns a pointer to the first element (const, explicit).
         *
         * Equivalent to `begin() const`. Provided for compatibility with
         * `std::ranges` algorithms and range adaptors that use `cbegin()`.
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `const TValue*` pointing to the first element.
         */
        constexpr const_iterator cbegin() const
        {
            return begin();
        }

        /**
         * @brief Returns a pointer one past the last element (const, explicit).
         *
         * Equivalent to `end() const`. Provided for compatibility with
         * `std::ranges` algorithms and range adaptors that use `cend()`.
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `const TValue*` pointing one past the last element.
         */
        constexpr const_iterator cend() const
        {
            return end();
        }

        /**
         * @brief Returns a pointer to the underlying contiguous element storage (non-const).
         *
         * Required by `std::ranges::contiguous_range`. The returned pointer
         * is valid for `[data(), data() + size())`. Returns `nullptr` for
         * an empty array.
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `TValue*` pointing to the first element, or `nullptr`.
         */
        [[nodiscard]]
        constexpr pointer data() noexcept
        {
            return static_cast<TValue*>(static_cast<XLOPER12*>(val.array.lparray));
        }

        /**
         * @brief Returns a pointer to the underlying contiguous element storage (const).
         *
         * @pre `xltype == xltypeMulti` (checked via `ensure`).
         * @return `const TValue*` pointing to the first element, or `nullptr`.
         */
        [[nodiscard]]
        constexpr const_pointer data() const noexcept
        {
            return static_cast<const TValue*>(static_cast<const XLOPER12*>(val.array.lparray));
        }

        // -----------------------------------------------------------------------
        // Element access
        // -----------------------------------------------------------------------

        /**
         * @brief Accesses an element by flat (row-major) index.
         *
         * Uses deducing-this to provide a single implementation for both the
         * const and non-const cases.
         *
         * @tparam Self  Deduced type of `*this`. Determines const-ness of the
         *               returned reference.
         *
         * @param index  Zero-based flat index in `[0, size())`.
         * @return       Reference to the element at @p index. The reference
         *               is `const` when `*this` is const.
         *
         * @pre  `xltype == xltypeMulti` (checked via `ensure`).
         * @throws std::out_of_range if `index >= size()`.
         *
         * @code
         * Array<Number> arr(1, 3);
         * arr[1] = Number(42.0);           // write
         * double d = arr[1];               // read
         * @endcode
         */
        template<typename Self>
        constexpr auto& operator[](this Self&& self, size_t index)
        {
            using QualifiedValue = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const TValue,
                TValue
            >;

            ensure(self.xltype == xltypeMulti, "Array is not valid");
            const size_t size = self.size();
            if (index >= size) throw std::out_of_range("Array index out of range");
            auto s = std::span<QualifiedValue>(static_cast<QualifiedValue*>(self.val.array.lparray), size);
            return s[index];
        }

        /**
         * @brief Accesses an element by (row, column) index.
         *
         * Uses `std::mdspan` for row-major two-dimensional indexing. Uses
         * deducing-this for a single const/non-const implementation.
         *
         * @tparam Self  Deduced type of `*this`. Determines const-ness of the
         *               returned reference.
         *
         * @param row    Zero-based row index in `[0, rows())`.
         * @param col    Zero-based column index in `[0, cols())`.
         * @return       Reference to the element at `(row, col)`. The reference
         *               is `const` when `*this` is const.
         *
         * @pre  `xltype == xltypeMulti` (checked via `ensure`).
         * @throws std::out_of_range if @p row or @p col is out of range.
         *
         * @code
         * Array<Number> m(2, 3);
         * m[1, 2] = Number(99.0);          // write
         * double d = m[0, 1];              // read
         * @endcode
         */
        template<typename Self>
        constexpr auto& operator[](this Self&& self, size_t row, size_t col)
        {
            using QualifiedValue = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const TValue,
                TValue
            >;

            ensure(self.xltype == xltypeMulti, "Array is not valid");
            if (row >= static_cast<size_t>(self.val.array.rows) || col >= static_cast<size_t>(self.val.array.columns))
                throw std::out_of_range("Array index out of range");

            auto* ptr = static_cast<QualifiedValue*>(self.val.array.lparray);
            return ptr[row * static_cast<size_t>(self.val.array.columns) + col];
        }

        // -----------------------------------------------------------------------
        // Conversions
        // -----------------------------------------------------------------------

        /**
         * @brief Implicit conversion to any sequence container constructible
         *        from a pair of `TValue*` iterators.
         *
         * Constructs a `TContainer<TValue>` using `TContainer<TValue>(begin(), end())`.
         * Works for `std::vector`, `std::deque`, `std::list`, and similar containers.
         *
         * @tparam TContainer A class template that accepts `TValue` as its first
         *                    argument and supports iterator-range construction.
         *
         * @return A newly constructed container holding copies of all elements.
         *
         * @code
         * Array<Number> arr { 1.0, 2.0, 3.0 };
         * std::vector<Number> v = arr;   // implicit conversion
         * std::deque<Number>  d = arr;   // implicit conversion
         * @endcode
         */
        template<template<typename, typename...> class TContainer>
            requires requires { TContainer<TValue>(std::declval<TValue*>(), std::declval<TValue*>()); }
        constexpr operator TContainer<TValue>() const
        {
            return TContainer<TValue>(begin(), end());
        }

        /**
         * @brief Converts the array to a container, converting each element to @p TElem.
         *
         * Each element is cast via `static_cast<TElem>(v)`. Calls `reserve()`
         * on containers that support it.
         *
         * @tparam TContainer A class template (e.g. `std::vector`, `std::deque`)
         *                    that supports `push_back`.
         * @tparam TElem      Target element type. Must be convertible from `TValue`.
         *
         * @return A `TContainer<TElem>` holding the converted elements.
         *
         * @code
         * Array<Number> arr { 1.0, 2.0, 3.0 };
         * auto v = arr.to<std::vector, double>();   // std::vector<double>
         * @endcode
         */
        template<template<typename, typename...> class TContainer, typename TElem>
            requires std::convertible_to<TValue, TElem>
        constexpr auto to() const
        {
            TContainer<TElem> result {};
            if constexpr (requires { result.reserve(size_t{}); })
                result.reserve(size());
            for (auto const& v : *this) result.push_back(static_cast<TElem>(v));
            return result;
        }

        /**
         * @brief Converts the array to a container of `TValue` elements
         *        (no element conversion).
         *
         * Equivalent to `to<TContainer, TValue>()`. Provided for convenience
         * when no element type conversion is needed.
         *
         * @tparam TContainer A class template that supports `push_back`.
         *
         * @return A `TContainer<TValue>` holding copies of all elements.
         *
         * @code
         * Array<Number> arr { 1.0, 2.0, 3.0 };
         * auto v = arr.to<std::vector>();   // std::vector<Number>
         * auto d = arr.to<std::deque>();    // std::deque<Number>
         * @endcode
         */
        template<template<typename, typename...> class TContainer>
        constexpr auto to() const
        {
            return to<TContainer, TValue>();
        }

    private:

        /**
         * @brief Allocates and initialises a raw `XLOPER12` array of @p size elements.
         *
         * Uses `std::make_unique_for_overwrite` to allocate uninitialized storage,
         * then constructs each element in-place as a `TValue` copy of @p fill via
         * `std::construct_at`. Returns `nullptr` for size 0.
         *
         * @param size  Number of elements to allocate.
         * @param fill  Value to copy-construct into every slot. Defaults to `TValue{}`.
         * @return      A `unique_ptr<XLOPER12[]>` owning the buffer,
         *              or `nullptr` if `size == 0`.
         *
         * @throws std::bad_alloc if allocation fails.
         */
        constexpr static std::unique_ptr<XLOPER12[]> make_array(size_t size, TValue fill = {})
        {
            if (size == 0) return nullptr;

            auto buffer = std::make_unique_for_overwrite<XLOPER12[]>(size);
            if (!buffer) throw std::bad_alloc();
            for (size_t i = 0; i < size; ++i) std::construct_at(static_cast<TValue*>(&buffer[i]), fill);
            return buffer;
        }
    };

    /**
     * @brief Returns a copy of @p arr with new row and column dimensions.
     *
     * Creates a copy of @p arr and calls `reshape()` on it. The total element
     * count must be preserved (`rows * cols == arr.size()`); the element values
     * and their row-major order are unchanged.
     *
     * @tparam TValue  The element type of the array.
     *
     * @param arr   The source array to copy and reshape.
     * @param rows  New row count.
     * @param cols  New column count.
     * @return      A new `Array<TValue>` with the same elements and the
     *              requested dimensions.
     *
     * @throws std::out_of_range     if @p rows or @p cols exceeds the
     *                               corresponding Excel limit.
     * @throws std::overflow_error   if `rows * cols` overflows `size_t`.
     * @throws std::invalid_argument if `rows * cols != arr.size()`.
     * @throws std::bad_alloc        if the copy allocation fails.
     *
     * @code
     * Array<Number> flat { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };  // 1×6
     * auto matrix = reshape(flat, 2, 3);                       // 2×3 copy
     * @endcode
     */
    template<typename TValue>
    [[nodiscard]] constexpr Array<TValue> reshape(const Array<TValue>& arr, size_t rows, size_t cols)
    {
        Array<TValue> result(arr);
        result.reshape(rows, cols);
        return result;
    }

    /**
     * @brief Returns a transposed copy of @p arr.
     *
     * Creates a new `Array<TValue>` with rows and columns swapped. Element
     * `arr[r, c]` maps to `result[c, r]` in the returned array. The source
     * array is not modified.
     *
     * @tparam TValue  The element type of the array.
     *
     * @param arr  The source array to transpose.
     * @return     A new `Array<TValue>` with dimensions `cols × rows` and
     *             elements reordered accordingly. Returns an empty array if
     *             @p arr is empty.
     *
     * @throws std::bad_alloc if the allocation for the result fails.
     *
     * @code
     * Array<Number> h { 1.0, 2.0, 3.0 };       // 1×3
     * auto v = transpose(h);                     // 3×1
     *
     * Array<Number> m(2, 3, Number(0.0));
     * m[0, 0] = 1.0; m[0, 1] = 2.0; m[0, 2] = 3.0;
     * m[1, 0] = 4.0; m[1, 1] = 5.0; m[1, 2] = 6.0;
     * auto t = transpose(m);                     // 3×2
     * // t[0,0]==1, t[1,0]==2, t[2,0]==3
     * // t[0,1]==4, t[1,1]==5, t[2,1]==6
     * @endcode
     */
    template<typename TValue>
    [[nodiscard]] constexpr Array<TValue> transpose(const Array<TValue>& arr)
    {
        const size_t r = arr.rows();
        const size_t c = arr.cols();
        if (arr.empty()) return Array<TValue>{};
        Array<TValue> result(c, r);
        for (size_t row = 0; row < r; ++row)
            for (size_t col = 0; col < c; ++col)
                result[col, row] = arr[row, col];
        return result;
    }

    /**
     * @brief Returns a new array containing the specified rows of @p arr.
     *
     * Each index in @p indices selects a row from @p arr. The selected rows
     * are copied into a new array in the order they appear in @p indices,
     * producing a result with `indices.size() × arr.cols()` elements.
     * Duplicate indices are allowed and produce duplicate rows in the result.
     *
     * @tparam TValue   The element type of the array.
     *
     * @param arr      The source array.
     * @param indices  Row indices to select, in the desired output order.
     *                 Each index must be in `[0, arr.rows())`.
     * @return         A new `Array<TValue>` with `indices.size()` rows and
     *                 `arr.cols()` columns. Returns an empty array if
     *                 @p indices is empty.
     *
     * @throws std::out_of_range if any index in @p indices is ≥ `arr.rows()`.
     * @throws std::bad_alloc    if allocation fails.
     *
     * @code
     * Array<Number> m({ 1, 2, 3,
     *                   4, 5, 6,
     *                   7, 8, 9 }, Array<Number>::TwoDimensional{3, 3});
     * auto sub = get_rows(m, {0, 2});   // rows 0 and 2 → 2×3
     * // sub[0,0]==1, sub[0,1]==2, sub[0,2]==3
     * // sub[1,0]==7, sub[1,1]==8, sub[1,2]==9
     * @endcode
     */
    template<typename TValue>
    [[nodiscard]] constexpr Array<TValue> get_rows(const Array<TValue>& arr,
                                                   std::initializer_list<size_t> indices)
    {
        if (indices.size() == 0) return Array<TValue>{};
        for (size_t idx : indices)
            if (idx >= arr.rows())
                throw std::out_of_range("get_rows: row index out of range");

        const size_t c = arr.cols();
        Array<TValue> result(indices.size(), c);
        size_t dst_row = 0;
        for (size_t src_row : indices) {
            for (size_t col = 0; col < c; ++col)
                result[dst_row, col] = arr[src_row, col];
            ++dst_row;
        }
        return result;
    }

    /**
     * @brief Returns a new array containing the specified columns of @p arr.
     *
     * Each index in @p indices selects a column from @p arr. The selected
     * columns are copied into a new array in the order they appear in
     * @p indices, producing a result with `arr.rows() × indices.size()`
     * elements. Duplicate indices are allowed and produce duplicate columns
     * in the result.
     *
     * @tparam TValue   The element type of the array.
     *
     * @param arr      The source array.
     * @param indices  Column indices to select, in the desired output order.
     *                 Each index must be in `[0, arr.cols())`.
     * @return         A new `Array<TValue>` with `arr.rows()` rows and
     *                 `indices.size()` columns. Returns an empty array if
     *                 @p indices is empty.
     *
     * @throws std::out_of_range if any index in @p indices is ≥ `arr.cols()`.
     * @throws std::bad_alloc    if allocation fails.
     *
     * @code
     * Array<Number> m({ 1, 2, 3,
     *                   4, 5, 6,
     *                   7, 8, 9 }, Array<Number>::TwoDimensional{3, 3});
     * auto sub = get_cols(m, {0, 2});   // cols 0 and 2 → 3×2
     * // sub[0,0]==1, sub[0,1]==3
     * // sub[1,0]==4, sub[1,1]==6
     * // sub[2,0]==7, sub[2,1]==9
     * @endcode
     */
    template<typename TValue>
    [[nodiscard]] constexpr Array<TValue> get_cols(const Array<TValue>& arr,
                                                   std::initializer_list<size_t> indices)
    {
        if (indices.size() == 0) return Array<TValue>{};
        for (size_t idx : indices)
            if (idx >= arr.cols())
                throw std::out_of_range("get_cols: column index out of range");

        const size_t r = arr.rows();
        Array<TValue> result(r, indices.size());
        size_t dst_col = 0;
        for (size_t src_col : indices) {
            for (size_t row = 0; row < r; ++row)
                result[row, dst_col] = arr[row, src_col];
            ++dst_col;
        }
        return result;
    }

}    // namespace xll

