//
// Created by kenne on 19/03/2026.
//

#pragma once

#include <xlcall.hpp>

#ifdef _WIN32
#  include <windows.h>

namespace xll
{
    /// Returns the HWND of the top-level Excel window.
    /// xlGetHwnd returns xltypeInt with the handle in val.w (32-bit).
    /// On 64-bit Windows HWND is pointer-sized, so the value is sign-extended
    /// via INT_PTR before being reinterpreted as HWND.
    inline HWND get_hwnd()
    {
        XLOPER12 result {};
        if (Excel12(xlGetHwnd, &result, 0) != xlretSuccess)
            return nullptr;
        if (result.xltype != xltypeInt)
            return nullptr;
        return reinterpret_cast<HWND>(static_cast<INT_PTR>(result.val.w));
    }

}   // namespace xll

#endif  // _WIN32

