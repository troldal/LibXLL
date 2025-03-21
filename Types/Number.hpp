//
// Created by kenne on 21/03/2025.
//

#pragma once

namespace xll
{

    class Number : public XLOPER12
    {
    public:
        using XLOPER12::val;
        using XLOPER12::xltype;

        // Default constructor
        Number() : Number(std::nan("")) {}

        template<typename TNumber>
            requires std::is_arithmetic_v<TNumber>
        explicit Number(TNumber num) : XLOPER12()
        {
            xltype  = xltypeNum;
            val.num = static_cast<double>(num);
        }

        Number(const Number& other) : XLOPER12(static_cast<XLOPER12>(other))
        {
            if (xltype == xltypeNum) return;

            if (other.xltype == xltypeBool) {
                xltype  = xltypeNum;
                val.num = other.val.xbool;
                return;
            }

            xltype = xltypeNil;
        }

        Number(Number&& other) noexcept : XLOPER12()
        {
            using std::swap;
            swap(other, *this);

            if (xltype == xltypeNum) return;

            if (xltype == xltypeBool) {
                xltype  = xltypeNum;
                val.num = val.xbool;
                return;
            }

            xltype = xltypeNil;
        }

        ~Number() = default;

        Number& operator=(const Number& other)
        {
            using std::swap;

            if (this == &other) return *this;
            auto rhs = other;
            swap(rhs, *this);
            return *this;
        }

        Number& operator=(Number&& other) noexcept
        {
            using std::swap;
            if (this == &other) return *this;
            swap(other, *this);

            if (xltype == xltypeNum) return *this;

            if (xltype == xltypeBool) {
                xltype  = xltypeNum;
                val.num = val.xbool;
                return *this;
            }

            xltype = xltypeNil;
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& os, const Number& num)
        {
            if (num.xltype != xltypeNum) throw std::bad_cast();
            os << num.val.num;
            return os;
        }

        operator double() const { return val.num; }    // NOLINT

        [[nodiscard]]
        std::optional<Number> optional() const
        {
            if (xltype != xltypeNum) return std::nullopt;
            return *this;
        }

        [[nodiscard]]
        bool has_value() const { return xltype == xltypeNum; }

        explicit operator bool() const { return xltype == xltypeNum; }
    };

}    // namespace xll