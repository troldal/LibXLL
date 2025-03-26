//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"
#include "Nil.hpp"

namespace xll
{

    class Missing : public impl::Base<Missing, xltypeMissing>
    {
        using BASE = impl::Base<Missing, xltypeMissing>;

    public:
        using BASE::BASE;

        Missing() = default;

        Missing(const Missing&)
        {
            xltype = xltypeMissing;
        }

        Missing& operator=(const Missing&)
        {
            xltype = xltypeMissing;
            return *this;
        }

        friend bool operator==(const Missing&, const Missing&)
        {
            return true;
        }

        operator xll::Nil() const // NOLINT
        {
            return {};
        }
    };
}    // namespace xll