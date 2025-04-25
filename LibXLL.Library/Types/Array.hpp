//
// Created by kenne on 24/03/2025.
//

#pragma once

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

    enum class ArrayShape { Empty, Singular, Horizontal, Vertical, TwoDimensional };

    template<typename TValue>
    class Array : public XLOPER12
    {
    public:
        using value_type = TValue;

        /**
         * @brief Default constructor for the Array class.
         *
         * This constructor initializes an Array object by setting its base class
         * (XLOPER12) and configuring the `xltype` member to `xltypeMulti`,
         * indicating that the Array is a multi-cell array by default.
         */
        Array() : XLOPER12()
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
        Array(size_t rows, size_t cols) : Array()
        {
            if (rows * cols == 0) return;

            val.array.lparray = make_array(rows * cols).release();
            val.array.rows    = static_cast<RW>(rows);
            val.array.columns = static_cast<COL>(cols);
        }

        /**
         * @brief Constructs an Array from an initializer list.
         *
         * This constructor creates a single-row array containing the elements from
         * the provided initializer list. It initializes the Array with the appropriate
         * dimensions based on the number of elements in the list.
         *
         * @param values The initializer list of values to populate the array with.
         * @throws std::bad_alloc if memory allocation fails.
         */
        Array(std::initializer_list<TValue> values) : Array()
        {
            if (values.size() == 0) return;

            val.array.lparray = make_array(values.size()).release();
            val.array.rows    = 1;
            val.array.columns = static_cast<COL>(values.size());

            auto it = values.begin();
            for (size_t i = 0; i < values.size(); ++i, ++it) {
                static_cast<TValue&>(val.array.lparray[i]) = *it;
            }
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
        Array(const Missing& _) : Array() {}

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
        Array(Array&& other) noexcept : Array()
        {
            if (other.xltype == xltypeMulti) {
                val.array.lparray       = other.val.array.lparray;
                other.val.array.lparray = nullptr;
                val.array.rows          = other.rows();
                val.array.columns       = other.cols();
                other.val.array.rows    = 0;
                other.val.array.columns = 0;
                return;
            }

            // if (other.xltype == TValue::excel_type) {
           else {
                xltype = other.xltype;
                reinterpret_cast<TValue*>(this)->operator=(std::move(*reinterpret_cast<TValue*>(&other)));
                return;
            }
        }

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
        Array(const Array& other) : Array()
        {
            if (other.xltype == xltypeMulti) {
                val.array.lparray = make_array(other.size()).release();
                val.array.rows    = other.rows();
                val.array.columns = other.cols();
                for (unsigned i = 0; i < size(); ++i) {
                    static_cast<TValue&>(val.array.lparray[i]) = static_cast<TValue&>(other.val.array.lparray[i]);
                }
                return;
            }

            // if (other.xltype == TValue::excel_type) {
            else {
                xltype = other.xltype;
                reinterpret_cast<TValue*>(this)->operator=(*reinterpret_cast<TValue const*>(&other));
                return;
            }
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
        ~Array()
        {
            if (xltype == xltypeMulti && val.array.lparray != nullptr) {
                for (auto& item : *this) item.~TValue();
                delete[] val.array.lparray;
                val.array.lparray = nullptr;
            }
            else if (xltype != xltypeMulti) {
                // For single values, we need to call the destructor
                // but not delete anything, as this is part of the XLOPER12 itself
                reinterpret_cast<TValue*>(this)->~TValue();
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
        Array& operator=(const Array& other)
        {
            if (this == &other) return *this;

            if (other.xltype == xltypeMulti) {
                this->~Array();
                xltype            = other.xltype;
                val.array.lparray = make_array(other.size()).release();
                val.array.rows    = other.rows();
                val.array.columns = other.cols();
                for (unsigned i = 0; i < size(); ++i) {
                    static_cast<TValue&>(val.array.lparray[i]) = static_cast<TValue&>(other.val.array.lparray[i]);
                }
                return *this;
            }

            // if (other.xltype == TValue::excel_type) {
            else {
                this->~Array();
                xltype = other.xltype;
                reinterpret_cast<TValue*>(this)->operator=(*reinterpret_cast<TValue const*>(&other));
                return *this;
            }

            // Should not reach this point:
            throw std::runtime_error("Array assignment failed");
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
        Array& operator=(Array&& other) noexcept
        {
            if (this == &other) return *this;

            if (other.xltype == xltypeMulti) {
                this->~Array();
                xltype                  = other.xltype;
                val.array.lparray       = other.val.array.lparray;
                other.val.array.lparray = nullptr;
                val.array.rows          = other.rows();
                val.array.columns       = other.cols();
                other.val.array.rows    = 0;
                other.val.array.columns = 0;
                return *this;
            }

            // if (other.xltype == TValue::excel_type) {
            else {
                this->~Array();
                xltype = other.xltype;
                reinterpret_cast<TValue*>(this)->operator=(*reinterpret_cast<TValue const*>(&other));
                return *this;
            }

            // Should not reach this point:
            throw std::runtime_error("Array assignment failed");
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
        Array& operator=(const Missing& _)
        {
            this->~Array();
            xltype            = xltypeMulti;
            val.array.lparray = nullptr;
            val.array.rows    = 0;
            val.array.columns = 0;
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
        ArrayShape shape() const
        {
            if (rows() * cols() == 0) return ArrayShape::Empty;
            if (rows() * cols() == 1) return ArrayShape::Singular;
            if (rows() > 1 && cols() == 1) return ArrayShape::Vertical;
            if (rows() == 1 && cols() > 1) return ArrayShape::Horizontal;
            return ArrayShape::TwoDimensional;
        }

        [[nodiscard]]
        size_t rows() const
        {
            if (xltype == xltypeMulti) return val.array.rows;
            if (xltype == xltypeNil || xltype == xltypeMissing || xltype == 0) return 0;
            // if (xltype == TValue::excel_type) return 1;
            //
            // throw std::runtime_error("Array is not a multi-cell array");

            return 1;
        }

        [[nodiscard]]
        size_t cols() const
        {
            if (xltype == xltypeMulti) return val.array.columns;
            if (xltype == xltypeNil || xltype == xltypeMissing || xltype == 0) return 0;
            // if (xltype == TValue::excel_type) return 1;
            //
            // throw std::runtime_error("Array is not a multi-cell array");
            return 1;
        }

        [[nodiscard]]
        size_t size() const
        {
            if (xltype == xltypeMulti) return val.array.rows * val.array.columns;
            if (xltype == xltypeNil || xltype == xltypeMissing || xltype == 0) return 0;

            return 1;

        }

        bool empty() const
        {
            return size() == 0;
        }

        void reshape(size_t rows, size_t cols)
        {
            if (rows * cols != size()) throw std::runtime_error("Array reshape failed");
            val.array.rows    = static_cast<RW>(rows);
            val.array.columns = static_cast<COL>(cols);
        }

        TValue* begin()
        {
            if (xltype == xltypeMulti) {
                return static_cast<TValue*>(val.array.lparray);
            }
            else {
                return reinterpret_cast<TValue*>(this);
            }
        }

        TValue const* begin() const
        {
            if (xltype == xltypeMulti) {
                return static_cast<TValue const*>(val.array.lparray);
            }
            else {
                return reinterpret_cast<TValue const*>(this);
            }
        }

        TValue* end()
        {
            if (xltype == xltypeMulti) {
                return static_cast<TValue*>(val.array.lparray + size());
            }
            else {
                return reinterpret_cast<TValue*>(this) + 1;
            }
        }

        TValue const* end() const
        {
            if (xltype == xltypeMulti) {
                return static_cast<TValue const*>(val.array.lparray + size());
            }
            else {
                return reinterpret_cast<TValue const*>(this) + 1;
            }
        }

        TValue& operator[](size_t index)
        {
            if (xltype == xltypeMulti) {
                if (index + 1 > val.array.rows * val.array.columns) throw std::out_of_range("Array index out of range");
                auto s =
                    std::span<TValue, std::dynamic_extent>(static_cast<TValue*>(val.array.lparray), val.array.rows * val.array.columns);
                return s[index];
            }
            else {
                if (index != 0) throw std::out_of_range("Array index out of range");
                return *reinterpret_cast<TValue*>(this);
            }
        }

        const TValue& operator[](size_t index) const
        {
            if (xltype == xltypeMulti) {
                if (index + 1 > val.array.rows * val.array.columns) throw std::out_of_range("Array index out of range");
                auto s =
                    std::span<TValue, std::dynamic_extent>(static_cast<TValue*>(val.array.lparray), val.array.rows * val.array.columns);
                return s[index];
            }
            else {
                if (index != 0) throw std::out_of_range("Array index out of range");
                return *reinterpret_cast<TValue const*>(this);
            }
        }

        TValue operator[](size_t row, size_t col) const
        {
            if (xltype == xltypeMulti) {
                if ((row + 1) > val.array.rows || (col + 1) > val.array.columns) throw std::out_of_range("Array index out of range");
                using ext_t = mds::extents<uint32_t, std::dynamic_extent, std::dynamic_extent>;
                auto m      = mds::mdspan<TValue, ext_t>(static_cast<TValue*>(val.array.lparray), val.array.rows, val.array.columns);
                return m[row, col];
            }
            else {
                if (row != 0 || col != 0) throw std::out_of_range("Array index out of range");
                return *static_cast<TValue const*>(this);
            }
        }

        operator std::vector<TValue>() const
        {
            std::vector<TValue> result {};
            result.reserve(size());
            for (auto const& v : *this) result.push_back(v);
            return result;
        }

        template<typename TElem>
            requires std::same_as<TElem, typename TValue::value_type>
        operator std::vector<fxt::expected<TElem, xll::String>>() const
        {
            std::vector<fxt::expected<TElem, xll::String>> result {};
            result.reserve(size());
            for (auto const& v : *this) result.push_back(v);
            return result;
        }

        template<template<typename> class TContainer, typename TElem>
            requires std::convertible_to<TValue, TElem>
        auto to() const
        {
            TContainer<TElem> result {};
            result.reserve(size());
            for (auto const& v : *this) result.push_back(v);
            return result;
        }

        template<typename TElem>
            requires std::convertible_to<TValue, TElem>
        auto to() const
        {
            std::vector<TElem> result {};
            result.reserve(size());
            for (auto const& v : *this) result.push_back(v);
            return result;
        }

        auto to() const
        {
            std::vector<TValue> result {};
            result.reserve(size());
            for (auto const& v : *this) result.push_back(v);
            return result;
        }

    private:
        static std::unique_ptr<XLOPER12[]> make_array(size_t size)
        {
            if (size == 0) return nullptr;

            // Allocate memory with proper size and error checking
            auto buffer = std::make_unique<XLOPER12[]>(size);
            if (!buffer) throw std::bad_alloc();
            for (unsigned i = 0; i < size; ++i) *static_cast<TValue*>(&buffer[i]) = TValue();

            return buffer;
        }
    };

    // template<template<typename> class TContainer, typename T, typename E>
    auto make_array(const auto& input)    //-> Array<Expected<Number>>
    {
        using T = typename std::remove_cvref_t<decltype(input)>::value_type::value_type;
        using E = typename std::remove_cvref_t<decltype(input)>::value_type::error_type;

        using value_t = std::conditional_t<
            std::floating_point<T>,
            xll::Number,
            std::conditional_t<std::integral<T>, xll::Int, std::conditional_t<std::convertible_to<T, std::string>, xll::String, void>>>;

        Array<Expected<value_t>> result(input.size(), 1);
        for (unsigned i = 0; i < input.size(); ++i) {
            if (input[i].has_value())
                result[i].value() = *input[i];
            else
                result[i] = xll::ErrNull;
        }
        return result;
    }

}    // namespace xll