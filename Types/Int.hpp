//
// Created by kenne on 21/03/2025.
//

#pragma once

namespace xll
{

    class Int : public XLOPER12
    {
    public:
        using XLOPER12::val;
        using XLOPER12::xltype;

        // Default constructor
        Int() : Int(std::nan("")) {}

        template<typename TNumber>
            requires std::is_arithmetic_v<TNumber>
        explicit Int(TNumber num) : XLOPER12()
        {
            xltype  = xltypeInt;
            val.w = static_cast<int>(num);
        }

        Int(const Int& other) : XLOPER12(static_cast<XLOPER12>(other))
        {
            if (xltype == xltypeInt) return;

            if (other.xltype == xltypeBool) {
                xltype  = xltypeInt;
                val.w = other.val.xbool;
                return;
            }

            xltype = xltypeNil;
        }

        Int(Int&& other) noexcept : XLOPER12()
        {
            using std::swap;
            swap(other, *this);

            if (xltype == xltypeInt) return;

            if (xltype == xltypeBool) {
                xltype  = xltypeInt;
                val.w = val.xbool;
                return;
            }

            xltype = xltypeNil;
        }

        ~Int() = default;

        Int& operator=(const Int& other)
        {
            using std::swap;

            if (this == &other) return *this;
            auto rhs = other;
            swap(rhs, *this);
            return *this;
        }

        Int& operator=(Int&& other) noexcept
        {
            using std::swap;
            if (this == &other) return *this;
            swap(other, *this);

            if (xltype == xltypeInt) return *this;

            if (xltype == xltypeBool) {
                xltype  = xltypeInt;
                val.w = val.xbool;
                return *this;
            }

            xltype = xltypeNil;
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& os, const Int& num)
        {
            if (num.xltype != xltypeInt) throw std::bad_cast();
            os << num.val.w;
            return os;
        }

        operator double() const { return val.num; }    // NOLINT

        [[nodiscard]]
        std::optional<Int> optional() const
        {
            if (xltype != xltypeInt) return std::nullopt;
            return *this;
        }

        [[nodiscard]]
        bool has_value() const { return xltype == xltypeInt; }

        explicit operator bool() const { return xltype == xltypeInt; }
    };

}    // namespace xll