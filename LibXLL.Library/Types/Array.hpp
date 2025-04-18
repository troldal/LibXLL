//
// Created by kenne on 24/03/2025.
//

#pragma once

#include "Variant.hpp"
#include <span>
#include <expected>
#include "Expected.hpp"
#ifdef _MSC_VER
    #include <mdspan>
    namespace mds = std;
#else
    #include <experimental/mdspan>
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

        Array() : XLOPER12() { xltype = xltypeMulti; }

        Array(size_t rows, size_t cols) : Array()
        {
            if (rows * cols == 0) {
                xltype = xltypeNil;
                return;
            }

            val.array.lparray = make_array(rows * cols).release();
            val.array.rows = static_cast<RW>(rows);
            val.array.columns = static_cast<COL>(cols);
        }

        /**
         * \brief Constructs an Array from an initializer list.
         *
         * This constructor creates a single-row array containing the elements from
         * the provided initializer list. It initializes the Array with the appropriate
         * dimensions based on the number of elements in the list.
         *
         * \param values The initializer list of values to populate the array with.
         * \throws std::bad_alloc if memory allocation fails.
         */
        Array(std::initializer_list<TValue> values) : Array()
        {
            if (values.size() == 0) {
                xltype = xltypeNil;
                return;
            }

            val.array.lparray = make_array(values.size()).release();
            val.array.rows    = 1;
            val.array.columns = static_cast<COL>(values.size());

            auto it = values.begin();
            for (size_t i = 0; i < values.size(); ++i, ++it) {
                static_cast<TValue&>(val.array.lparray[i]) = *it;
            }
        }

        Array(Array&& other) noexcept : Array()
        {
            switch (other.xltype == xltypeMulti) {
                case true: {
                    val.array.lparray = other.val.array.lparray;
                    other.val.array.lparray = nullptr;
                    val.array.rows = other.rows();
                    val.array.columns = other.cols();
                    other.val.array.rows = 0;
                    other.val.array.columns = 0;
                }
                break;
                default:
                    xltype = xltypeNil;
            }
        }

        Array(const Array& other) : Array()
        {
            switch (other.xltype == xltypeMulti) {
                case true: {
                    val.array.lparray = make_array(other.size()).release();
                    val.array.rows = other.rows();
                    val.array.columns = other.cols();
                    for (unsigned i = 0; i < size(); ++i) { static_cast<TValue&>(val.array.lparray[i]) = static_cast<TValue&>(other.val.array.lparray[i]); }
                }
                break;
                default:
                    xltype = xltypeNil;
            }
        }

        ~Array()
        {
            for (auto& item : *this) item.~TValue();
            delete[] val.array.lparray;
            val.array.lparray = nullptr;
            xltype  = xltypeNil;
        }

        Array& operator=(const Array& other)
        {
            switch (other.xltype == xltypeMulti) {
                case true: {
                    delete[] val.array.lparray;
                    val.array.lparray = make_array(other.size()).release();
                    val.array.rows = other.rows();
                    val.array.columns = other.cols();
                    for (unsigned i = 0; i < size(); ++i) { static_cast<TValue&>(val.array.lparray[i]) = static_cast<TValue&>(other.val.array.lparray[i]); }
                }
                break;
                default:
                    xltype = xltypeNil;
            }
            return *this;
        }

        Array& operator=(Array&& other) noexcept
        {
            if (this == &other) return *this;

            switch (other.xltype == xltypeMulti) {
                case true: {
                    delete[] val.array.lparray;
                    val.array.lparray = other.val.array.lparray;
                    other.val.array.lparray = nullptr;
                    val.array.rows = other.rows();
                    val.array.columns = other.cols();
                    other.val.array.rows = 0;
                    other.val.array.columns = 0;

                }
                break;
                default:
                    xltype = xltypeNil;
            }
            return *this;
        }

        [[nodiscard]]
        ArrayShape shape() const
        {
            if (val.array.rows * val.array.columns == 0) return ArrayShape::Empty;
            if (val.array.rows * val.array.columns == 1) return ArrayShape::Singular;
            if (val.array.rows > 1 && val.array.columns == 1) return ArrayShape::Vertical;
            if (val.array.rows == 1 && val.array.columns > 1) return ArrayShape::Horizontal;
            return ArrayShape::TwoDimensional;
        }

        [[nodiscard]]
        size_t rows() const
        {
            return val.array.rows;
        }

        [[nodiscard]]
        size_t cols() const
        {
            return val.array.columns;
        }

        [[nodiscard]]
        size_t size() const
        {
            return val.array.rows * val.array.columns;
        }

        void reshape(size_t rows, size_t cols)
        {
            if (rows * cols != size()) throw std::runtime_error("Array reshape failed");
            val.array.rows = static_cast<RW>(rows);
            val.array.columns = static_cast<COL>(cols);
        }

        TValue* begin()
        {
            return static_cast<TValue*>(val.array.lparray);
        }

        TValue const* begin() const
        {
            return static_cast<TValue const*>(val.array.lparray);
        }

        TValue* end()
        {
            return static_cast<TValue*>(val.array.lparray + size());
        }

        TValue const* end() const
        {
            return static_cast<TValue const*>(val.array.lparray + size());
        }

        TValue& operator[](int index) {
            if (index + 1 > val.array.rows * val.array.columns) throw std::out_of_range("Array index out of range");
            auto s = std::span<TValue, std::dynamic_extent>(static_cast<TValue*>(val.array.lparray), val.array.rows * val.array.columns);
            return s[index];
        }

        const TValue& operator[](int index) const {
            if (index + 1 > val.array.rows * val.array.columns) throw std::out_of_range("Array index out of range");
            auto s = std::span<TValue, std::dynamic_extent>(static_cast<TValue*>(val.array.lparray), val.array.rows * val.array.columns);
            return s[index];
        }

        TValue operator[](int row, int col) const {
            if ((row + 1) > val.array.rows  || (col + 1) > val.array.columns) throw std::out_of_range("Array index out of range");
            using ext_t = mds::extents<uint32_t, std::dynamic_extent, std::dynamic_extent>;
            auto m = mds::mdspan<TValue, ext_t>(
                static_cast<TValue*>(val.array.lparray),
                val.array.rows,
                val.array.columns);
            return m[row, col];
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

    // auto make_array(const auto& input)
    // {
    //     Array<typename std::remove_cvref_t<decltype(input)>::value_type> result(input.size(), 1);
    //     for (unsigned i = 0; i < input.size(); ++i) result[i] = input[i];
    //     return result;
    // }

    // template<template<typename> class TContainer, typename T, typename E>
    auto make_array(const auto& input) //-> Array<Expected<Number>>
    {
        using T = typename std::remove_cvref_t<decltype(input)>::value_type::value_type;
        using E = typename std::remove_cvref_t<decltype(input)>::value_type::error_type;

        using value_t =
            std::conditional_t<std::floating_point<T>, xll::Number,
            std::conditional_t<std::integral<T>, xll::Int,
            std::conditional_t<std::convertible_to<T, std::string>, xll::String, void>>>;

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