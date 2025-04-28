//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Base.hpp"
#include <cmath>

namespace xll
{

    class Number : public impl::Base<Number, xltypeNum, xltypeInt, xltypeBool>
    {
        using BASE = impl::Base<Number, xltypeNum, xltypeInt, xltypeBool>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Number";

    public:
        using BASE::BASE;
        using BASE::operator=;

        constexpr Number() : Number(0.0) {}
    };
}    // namespace xll