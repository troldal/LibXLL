//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Yields processor time and checks whether the user has pressed break (Esc).
     *
     * @param preserve_break  When `true` (default), the break condition is
     *                        preserved so that other functions can detect the
     *                        same event.  When `false`, the break condition is
     *                        cleared — use this in commands that handle the
     *                        break and wish to resume.
     *
     * @return `xll::Bool` — `true` if a user break was detected, `false` otherwise.
     *
     * @note Thread-safe when `preserve_break = true`.
     *       **Not** thread-safe when `preserve_break = false`.
     *
     * @code
     * // Worksheet function: detect break without clearing it
     * for (long i = 0; i < count; ++i) {
     *     if (xll::user_break()) return xll::Error(xlerrNA);
     *     // ... do work ...
     * }
     *
     * // Command: detect and clear the break before exiting
     * while (running) {
     *     if (xll::user_break(false)) return 0;
     *     // ... do work ...
     * }
     * @endcode
     */
    inline Bool user_break(bool preserve_break = true)
    {
        XLOPER12 arg {};
        arg.xltype    = xltypeBool;
        arg.val.xbool = preserve_break ? TRUE : FALSE;

        XLOPER12 raw {};
        if (Excel12(xlAbort, &raw, 1, &arg) != xlretSuccess || raw.xltype != xltypeBool)
            return Bool(false);

        return Bool(raw);
    }

}    // namespace xll


