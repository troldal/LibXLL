//
// Created by kenne on 24/03/2025.
//

#pragma once

#include "Variant.hpp"
#include <span>
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

}    // namespace xll