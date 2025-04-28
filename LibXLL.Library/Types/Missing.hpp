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

        constexpr Missing() = default;

        constexpr Missing(const Missing&)
        {
            xltype = xltypeMissing;
        }

        constexpr Missing& operator=(const Missing&)
        {
            xltype = xltypeMissing;
            return *this;
        }

        constexpr friend bool operator==(const Missing&, const Missing&)
        {
            return true;
        }

        constexpr operator xll::Nil() const // NOLINT
        {
            return {};
        }
    };
}    // namespace xll