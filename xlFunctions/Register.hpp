//
// Created by kenne on 21/03/2025.
//

#pragma once

namespace xll
{
    template<typename... Args>
    xll::Number Register(const Args&... args)
    {
        // Register the function with Excel
        xll::Number id;
        Excel12(xlfRegister, &id, sizeof...(args), &args...);
        xll::Number result = id;
        Excel12(xlFree, nullptr, 1, &id);
        return result;
    }

}    // namespace xll