//
// Created by kenne on 21/03/2025.
//

#pragma once
#include "Base.hpp"
#include <iostream>
#include <optional>

namespace xll
{

    class Int : public impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>
    {
        using BASE = impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>;

    public:
        using BASE::BASE;
        using BASE::operator=;

        Int() : Int(0) {}

        Int& operator++()
        {
            ++value();
            return *this;
        }

        Int operator++(int)
        {
            Int old = *this;
            ++value();
            return old;
        }
    };

    // class Int;
    // inline void swap(Int& lhs, Int& rhs) noexcept;
    //
    // class Int : public XLOPER12
    // {
    // public:
    //     using XLOPER12::val;
    //     using XLOPER12::xltype;
    //
    //     // Default constructor
    //     Int() : Int(0) {}
    //
    //     template<typename TNumber>
    //         requires std::is_arithmetic_v<TNumber>
    //     explicit Int(TNumber num) : XLOPER12()
    //     {
    //         xltype  = xltypeInt;
    //         val.w = static_cast<int>(num);
    //     }
    //
    //     Int(const Int& other) : XLOPER12(static_cast<XLOPER12>(other))
    //     {
    //         if (xltype == xltypeInt) return;
    //
    //         if (other.xltype == xltypeBool) {
    //             xltype  = xltypeInt;
    //             val.w = other.val.xbool;
    //             return;
    //         }
    //
    //         xltype = xltypeNil;
    //     }
    //
    //     Int(Int&& other) noexcept : XLOPER12()
    //     {
    //         using std::swap;
    //         swap(other, *this);
    //
    //         if (xltype == xltypeInt) return;
    //
    //         if (xltype == xltypeBool) {
    //             xltype  = xltypeInt;
    //             val.w = val.xbool;
    //             return;
    //         }
    //
    //         xltype = xltypeNil;
    //     }
    //
    //     ~Int() = default;
    //
    //     Int& operator=(const Int& other)
    //     {
    //         using std::swap;
    //
    //         if (this == &other) return *this;
    //         auto rhs = other;
    //         swap(rhs, *this);
    //         return *this;
    //     }
    //
    //     Int& operator=(Int&& other) noexcept
    //     {
    //         using std::swap;
    //         if (this == &other) return *this;
    //         swap(other, *this);
    //
    //         if (xltype == xltypeInt) return *this;
    //
    //         if (xltype == xltypeBool) {
    //             xltype  = xltypeInt;
    //             val.w = val.xbool;
    //             return *this;
    //         }
    //
    //         xltype = xltypeNil;
    //         return *this;
    //     }
    //
    //     friend std::ostream& operator<<(std::ostream& os, const Int& num)
    //     {
    //         if (num.xltype != xltypeInt) throw std::bad_cast();
    //         os << num.val.w;
    //         return os;
    //     }
    //
    //     operator int() const // NOLINT
    //     {
    //         if (xltype == xltypeInt) return val.w;
    //         if (xltype == xltypeBool) return val.xbool;
    //         if (xltype == xltypeNil) return val.num;
    //         throw std::bad_cast();
    //     }
    //
    //     [[nodiscard]]
    //     std::optional<Int> optional() const
    //     {
    //         if (xltype != xltypeInt) return std::nullopt;
    //         return *this;
    //     }
    //
    //     [[nodiscard]]
    //     bool has_value() const { return xltype == xltypeInt || xltype == xltypeNum || xltype == xltypeBool; }
    //
    //     explicit operator bool() const { return xltype == xltypeInt; }
    // };
    //
    // inline bool operator==(const Int& lhs, const Int& rhs) { return static_cast<int>(lhs) == static_cast<int>(rhs); }
    //
    // inline std::strong_ordering operator<=>(const Int& lhs, const Int& rhs)
    // {
    //     return static_cast<int>(lhs) <=> static_cast<int>(rhs);
    // }
    //
    // template<typename TOther>
    // Int operator+(const Int& lhs, TOther&& rhs)
    // {
    //     const int result = static_cast<int>(lhs) + static_cast<int>(rhs);
    //     return Int(result);
    // }
    //
    // inline void swap(Int& lhs, Int& rhs) noexcept
    // {
    //     using std::swap;
    //     swap(lhs.xltype, rhs.xltype);
    //     swap(lhs.val.w, rhs.val.w);
    // }

    // namespace literals
    // {
    //     inline String operator""_xs(const char* str, std::size_t len) { return String(std::string_view(str, len)); }
    // }    // namespace literals

}    // namespace xll