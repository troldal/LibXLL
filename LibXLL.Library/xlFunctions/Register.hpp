//
// Created by kenne on 21/03/2025.
//

#pragma once

namespace xll
{
    template<typename... Args>
        requires (sizeof...(Args) > 1)
    xll::Number Register(const Args&... args)
    {
        // Register the function with Excel
        xll::Number id;
        Excel12(xlfRegister, &id, sizeof...(args), &args...);
        xll::Number result = id;
        Excel12(xlFree, nullptr, 1, &id);
        return result;
    }

    inline xll::Number Register(std::vector<xll::Variant> args)
    {

        std::vector<LPXLOPER12> operArray;
        operArray.reserve(args.size());
        for (xll::Variant& var : args)
        {
            // Safe conversion since Variant inherits from XLOPER12.
            operArray.push_back(static_cast<LPXLOPER12>(&var));
        }

        // Register the function with Excel
        xll::Number id;
        // Excel12(xlfRegister, &id, sizeof...(args), &args...);
        Excel12v(xlfRegister, &id, static_cast<int>(operArray.size()), operArray.data());
        xll::Number result = id;
        Excel12(xlFree, nullptr, 1, &id);
        return result;
    }

}    // namespace xll