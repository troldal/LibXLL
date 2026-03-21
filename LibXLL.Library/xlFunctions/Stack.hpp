//
// Created by kenne on 21/03/2026.
//

#pragma once

#include <xlcall.hpp>
#include "../Types/Optional.hpp"

namespace xll
{
    // Returns the number of bytes currently available on Excel's stack,
    // or xll::None if the call fails.
    //
    // Excel caps the reported value at 64 KB (min(64 KB, actual free space)).
    // The raw val.w is a signed short, but must be treated as unsigned to
    // avoid a negative result when the available space exceeds 32 767 bytes.
    //
    // Note: Microsoft's documentation specifies xltypeInt as the return type,
    // but an early Excel 12 beta returned xltypeNum; both cases are handled.
    inline Optional<Int> stack()
    {
        XLOPER12 retval {};
        if (Excel12(xlStack, &retval, 0) != xlretSuccess)
            return None;

        if (retval.xltype == xltypeInt)
            return Int {} = retval.val.w;

        // Early Excel 12 beta quirk: result may arrive as xltypeNum.
        if (retval.xltype == xltypeNum)
            return Int {} = retval.val.num;

        return None;
    }

}    // namespace xll

