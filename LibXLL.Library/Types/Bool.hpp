//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"
#include <cmath>

namespace xll
{
    class Bool : public impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>
    {
        using BASE = impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Bool";

    public:
        using BASE::BASE;
        using BASE::operator=;

        constexpr Bool() : Bool(false) {}

        constexpr friend bool operator==(const Bool& lhs, bool rhs)
        {
            return static_cast<bool>(lhs.value()) == rhs;
        }
    };
}    // namespace xll
