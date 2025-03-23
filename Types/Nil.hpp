//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Base.hpp"
#include <cmath>

namespace xll
{

    class Nil : public impl::Base<Nil, xltypeNil>
    {
        using BASE = impl::Base<Nil, xltypeNil>;

    public:
        using BASE::BASE;
    };



// namespace xll
// {

    // class Nil : public XLOPER12
    // {
    // public:
    //     using XLOPER12::val;
    //     using XLOPER12::xltype;
    //
    //     // Default constructor
    //     Nil() : XLOPER12() { xltype = xltypeNil; }
    //
    //
    //     Nil(const Nil& other) : XLOPER12(static_cast<XLOPER12>(other))
    //     {
    //         xltype = xltypeNil;
    //     }
    //
    //     Nil(Nil&& other) noexcept : XLOPER12()
    //     {
    //         using std::swap;
    //         swap(other, *this);
    //
    //         xltype = xltypeNil;
    //     }
    //
    //     ~Nil() = default;
    //
    //     Nil& operator=(const Nil& other)
    //     {
    //         using std::swap;
    //
    //         if (this == &other) return *this;
    //         auto rhs = other;
    //         swap(rhs, *this);
    //         return *this;
    //     }
    //
    //     Nil& operator=(Nil&& other) noexcept
    //     {
    //         using std::swap;
    //         if (this == &other) return *this;
    //         swap(other, *this);
    //
    //         xltype = xltypeNil;
    //         return *this;
    //     }
    //
    //     // friend std::ostream& operator<<(std::ostream& os, const Nil& num)
    //     // {
    //     //     if (num.xltype != xltypeNil) throw std::bad_cast();
    //     //     return os;
    //     // }
    //
    //     // operator double() const { return val.num; }    // NOLINT
    //
    //     // [[nodiscard]]
    //     // std::optional<Nil> optional() const
    //     // {
    //     //     if (xltype != xltypeInt) return std::nullopt;
    //     //     return *this;
    //     // }
    //
    //     // [[nodiscard]]
    //     // bool has_value() const { return xltype == xltypeInt; }
    //
    //     // explicit operator bool() const { return xltype == xltypeInt; }
    // };

}    // namespace xll