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

        Nil() = default;

        Nil(const Nil&)
        {
            xltype = xltypeNil;
        }

        Nil(const xll::Missing&)
        {
            xltype = xltypeNil;
        }

        Nil& operator=(const Nil&)
        {
            xltype = xltypeNil;
            return *this;
        }

        Nil& operator=(const Missing&)
        {
            xltype = xltypeNil;
            return *this;
        }

        friend bool operator==(const Nil&, const Nil&)
        {
            return true;
        }
    };
}    // namespace xll