/**
 * @file GetName.hpp
 * @brief Wrapper for the Excel C API function `xlGetName`.
 *
 * Provides `xll::get_name()`, which returns the full path and file name of
 * the currently executing DLL.  The primary use of this value is as the first
 * argument to `xlfRegister` when registering XLL functions with Excel.
 *
 * **Excel SDK reference — xlGetName (0x4009)**
 *
 * - Takes no arguments.
 * - Returns an `xltypeStr` string on success.
 *
 * @see xlfRegister
 */

#pragma once

#include "../Types/Optional.hpp"
#include "../Types/String.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Returns the full path and file name of the currently executing DLL.
     *
     * @return The DLL path (e.g. `C:\MyAddins\MyXLL.xll`) as an `xll::String`,
     *         or `xll::None` if the call fails.
     */
    inline Optional<String> get_name()
    {
        XLOPER12 raw {};
        if (Excel12(xlGetName, &raw, 0) != xlretSuccess || raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll

