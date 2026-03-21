//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Types/String.hpp"
#include <xlcall.hpp>

namespace xll {
    /**
     * @brief Resets the active document window title to its default value.
     *
     * @return `xll::Bool` — `true` if the title was reset successfully.
     */
    inline Bool set_window_title()
    {
        XLOPER12 raw {};
        if (Excel12(xlfWindowTitle, &raw, 0) != xlretSuccess || raw.xltype != xltypeBool)
            return {false};

        return Bool(raw);
    }

    /**
     * @brief Sets the active document window title to @p title.
     *
     * @param title  The string to display as the document window title.
     *
     * @return `xll::Bool` — `true` if the title was set successfully,
     *         `false` if the argument could not be coerced to a string.
     */
    inline Bool set_window_title(const xll::String& title)
    {
        XLOPER12 raw {};
        if (Excel12(xlfWindowTitle, &raw, 1,
                    static_cast<LPXLOPER12>(const_cast<xll::String*>(&title))) != xlretSuccess ||
            raw.xltype != xltypeBool)
            return {false};

        return Bool(raw);
    }

}    // namespace xll

