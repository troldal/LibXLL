//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Base.hpp"

namespace xll
{

    class Nil : public impl::Base<Nil, xltypeNil>
    {
        using BASE = impl::Base<Nil, xltypeNil>;

    public:
        using BASE::BASE;

        Nil() = default;

        Nil(const Nil& t)
        {
            xltype = xltypeNil;
        }

        Nil& operator=(const Nil& t)
        {
            xltype = xltypeNil;
            return *this;
        }

        friend bool operator==(const Nil& lhs, const Nil& rhs)
        {
            return true;
        }
    };
}    // namespace xll