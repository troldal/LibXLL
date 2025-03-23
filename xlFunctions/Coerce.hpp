//
// Created by kenne on 21/03/2025.
//

#pragma once

namespace xll
{

    // template<typename... TResult>
    // TResult coerce(LPXLOPER12 src)
    // {
    //     TResult result {};
    //     auto destType = (TResult{} | ...);
    //     Excel12(xlCoerce, &result, 2, src, destType);
    //
    // }

    template<typename TResult>
    TResult coerce(LPXLOPER12 src)
    {
        TResult value {};
        // auto destType = (TResult{} | ...);
        xll::Int destType {};
        destType.val.w = value.xltype;
        Excel12(xlCoerce, &value, 2, src, &destType);
        TResult result = value;
        Excel12(xlFree, nullptr, 1, &value);
        return result;

    }

}    // namespace xll