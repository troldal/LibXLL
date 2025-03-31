//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Function.hpp"
#include "Registry.hpp"

namespace xll
{

    class AddIn
    {
    public:
        template<typename TFunc>
        explicit AddIn(TFunc& f)
        {
            xll::Registry::instance().add(f);
        }
    };

}    // namespace xll