//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Base.hpp"

namespace xll
{
    class Missing;

    class Nil : public impl::Base<Nil, xltypeNil>
    {
        using BASE = impl::Base<Nil, xltypeNil>;

    public:
        using BASE::BASE;

        constexpr Nil() = default;

        constexpr Nil(const Nil&)
        {
            xltype = xltypeNil;
        }

        constexpr Nil(const xll::Missing&)
        {
            xltype = xltypeNil;
        }

        constexpr Nil& operator=(const Nil&)
        {
            xltype = xltypeNil;
            return *this;
        }

        constexpr Nil& operator=(const Missing&)
        {
            xltype = xltypeNil;
            return *this;
        }

        constexpr friend bool operator==(const Nil&, const Nil&)
        {
            return true;
        }
    };
}    // namespace xll