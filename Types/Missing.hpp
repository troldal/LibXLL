//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"

namespace xll
{

    class Missing : public impl::Base<Missing, xltypeMissing>
    {
        using BASE = impl::Base<Missing, xltypeMissing>;

    public:
        using BASE::BASE;
    };
}    // namespace xll