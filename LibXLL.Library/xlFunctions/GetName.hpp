//
// Created by kenne on 21/03/2025.
//

#pragma once

namespace xll
{

    inline String get_name()
    {
        XLOPER12 name {};
        Excel12(xlGetName, &name, 0);
        xll::String result = xll::String(name);
        Excel12(xlFree, nullptr, 1, &name);
        return result;
    }

}    // namespace xll