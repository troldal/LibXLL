//
// Created by kenne on 24/03/2025.
//

#pragma once

#include <fxt.hpp>
#include "Expected.hpp"
#include "Variant.hpp"
#include <expected>
#include <span>
#ifdef _MSC_VER
#    include <mdspan>
namespace mds = std;
#else
#    include <experimental/mdspan>
namespace mds = std::experimental;
#endif

namespace xll
{

    // enum class ArrayShape { Empty, Singular, Horizontal, Vertical, TwoDimensional };



    template<typename TValue>
    class Array : public XLOPER12
    {
        struct ShapeBase{};
    public:
        using value_type = TValue;
        struct Horizontal : ShapeBase {
            std::optional<size_t> size;
            constexpr Horizontal() = default;
            constexpr explicit Horizontal(size_t n) : size(n) {}
        };
        struct Vertical : ShapeBase {
            std::optional<size_t> size;
            constexpr Vertical() = default;
            constexpr explicit Vertical(size_t n) : size(n) {}
        };
        struct TwoDimensional : ShapeBase {
            size_t rows;
            size_t cols;
            constexpr TwoDimensional(size_t rows, size_t cols) : rows(rows), cols(cols) {}
        };
        struct Empty : ShapeBase {};
        struct Singular : ShapeBase {};

        using Shape = fxt::type_enum<Empty, Singular, Horizontal, Vertical, TwoDimensional>;




        /**
         * @brief Default constructor for the Array class.
         *
         * This constructor initializes an Array object by setting its base class
         * (XLOPER12) and configuring the `xltype` member to `xltypeMulti`,
         * indicating that the Array is a multi-cell array by default.
         */
        constexpr Array() : XLOPER12()
        {
            xltype            = xltypeMulti;
            val.array.lparray = nullptr;
            val.array.rows    = 0;
            val.array.columns = 0;
        }

        /**
         * @brief Constructs a two-dimensional Array with the specified number of rows and columns.
         *
         * This constructor initializes the Array as a multi-cell array with the given dimensions.
         * If the total number of elements (rows * cols) is zero, the Array is set to an empty state (`xltypeNil`).
         * Otherwise, memory is allocated for the array elements, and the dimensions are set accordingly.
         *
         * @param rows The number of rows in the array.
         * @param cols The number of columns in the array.
         * @throws std::bad_alloc if memory allocation fails.
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

        template<typename TShape = Horizontal>
            requires (std::same_as<TShape, Horizontal> || std::same_as<TShape, Vertical> || std::same_as<TShape, TwoDimensional>)
        constexpr Array(std::initializer_list<TValue> values, TShape shape = {}, TValue fill = {}) : Array()
        {
            *this = Array(std::ranges::subrange(values.begin(), values.end()), shape, fill);
        }

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

        /**
         * @brief Determines the shape of the array.
         *
         * This method analyzes the dimensions of the array and returns an ArrayShape enum value
         * that indicates its geometrical configuration:
         * - Empty: An array with no elements (rows * cols = 0)
         * - Singular: A single-element array (rows * cols = 1)
         * - Vertical: A column vector (rows > 1, cols = 1)
         * - Horizontal: A row vector (rows = 1, cols > 1)
         * - TwoDimensional: A matrix with multiple rows and columns
         *
         * @return ArrayShape The shape classification of the array.
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

        [[nodiscard]]
        constexpr size_t rows() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<size_t>(val.array.rows);
        }

        [[nodiscard]]
        constexpr size_t cols() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<size_t>(val.array.columns);
        }

        [[nodiscard]]
        constexpr size_t size() const
        {
            if (xltype != xltypeMulti) std::unreachable();
            return static_cast<size_t>(val.array.rows) * static_cast<size_t>(val.array.columns);
        }

        [[nodiscard]]
        constexpr bool empty() const
        {
            return size() == 0;
        }

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

        constexpr TValue* begin()
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue*>(static_cast<XLOPER12*>(val.array.lparray));
        }

        constexpr TValue const* begin() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue const*>(static_cast<XLOPER12 const*>(val.array.lparray));
        }

        constexpr TValue* end()
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue*>(static_cast<XLOPER12*>(val.array.lparray)) + size();
        }

        constexpr TValue const* end() const
        {
            ensure(xltype == xltypeMulti, "Array is not valid");
            return static_cast<TValue const*>(static_cast<XLOPER12 const*>(val.array.lparray)) + size();
        }

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
            using ext_t = mds::extents<uint32_t, std::dynamic_extent, std::dynamic_extent>;
            auto m = mds::mdspan<QualifiedValue, ext_t>(static_cast<QualifiedValue*>(self.val.array.lparray), self.val.array.rows, self.val.array.columns);
            return m[row, col];
        }


        template<template<typename, typename...> class TContainer>
            requires requires { TContainer<TValue>(std::declval<TValue*>(), std::declval<TValue*>()); }
        constexpr operator TContainer<TValue>() const
        {
            return TContainer<TValue>(begin(), end());
        }

        // template<typename TElem>
        //     requires std::same_as<TElem, typename TValue::value_type>
        // constexpr operator std::vector<fxt::expected<TElem, xll::String>>() const
        // {
        //     std::vector<fxt::expected<TElem, xll::String>> result {};
        //     result.reserve(size());
        //     for (auto const& v : *this) result.push_back(v);
        //     return result;
        // }

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

        template<template<typename, typename...> class TContainer>
        constexpr auto to() const
        {
            return to<TContainer, TValue>();
        }

    private:

        constexpr static std::unique_ptr<XLOPER12[]> make_array(size_t size, TValue fill = {})
        {
            if (size == 0) return nullptr;

            auto buffer = std::make_unique_for_overwrite<XLOPER12[]>(size);
            if (!buffer) throw std::bad_alloc();
            for (size_t i = 0; i < size; ++i) std::construct_at(static_cast<TValue*>(&buffer[i]), fill);
            return buffer;
        }
    };

    // template<template<typename> class TContainer, typename T, typename E>
    // constexpr auto make_array(const auto& input)    //-> Array<Expected<Number>>
    // {
    //     using T = typename std::remove_cvref_t<decltype(input)>::value_type::value_type;
    //     using E = typename std::remove_cvref_t<decltype(input)>::value_type::error_type;
    //
    //     using value_t = std::conditional_t<
    //         std::floating_point<T>,
    //         xll::Number,
    //         std::conditional_t<std::integral<T>, xll::Int, std::conditional_t<std::convertible_to<T, std::string>, xll::String, void>>>;
    //
    //     Array<Expected<value_t, xll::Error>> result(input.size(), 1);
    //     for (unsigned i = 0; i < input.size(); ++i) {
    //         if (input[i].has_value())
    //             result[i].value() = *input[i];
    //         else
    //             result[i] = xll::ErrNull;
    //     }
    //     return result;
    // }

}    // namespace xll