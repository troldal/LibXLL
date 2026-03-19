// GetInst.hpp
#pragma once

#include <xlcall.hpp>

#ifdef _WIN32
#  include <windows.h>
#endif

namespace xll
{
    /// Returns the HINSTANCE of the running Excel process.
    /// Uses xlGetInstPtr, which is correct for both 32- and 64-bit Excel.
    /// Must NOT use xll::Int as the result operand: xlGetInstPtr returns
    /// xltypeBigData with the pointer in val.bigdata.h.hdata, not xltypeInt.
    inline HINSTANCE get_instance()
    {
        XLOPER12 result {};
        if (Excel12(xlGetInstPtr, &result, 0) != xlretSuccess)
            return nullptr;
        if (result.xltype != xltypeBigData)
            return nullptr;
        return static_cast<HINSTANCE>(result.val.bigdata.h.hdata);
    }

}   // namespace xll