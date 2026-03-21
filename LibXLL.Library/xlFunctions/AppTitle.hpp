//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Types/String.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Resets the Excel application title to the default ("Microsoft Excel").
     *
     * @return `xll::Bool` — `true` if the title was reset successfully.
     */
    inline Bool set_app_title()
    {
        XLOPER12 raw {};
        if (Excel12(xlfAppTitle, &raw, 0) != xlretSuccess || raw.xltype != xltypeBool)
            return {false};

        return Bool(raw);
    }

    /**
     * @brief Sets the Excel application title bar to @p title.
     *
     * The title is also shown on the application's taskbar button when minimised.
     *
     * @param title  The string to display as the application title.
     *
     * @return `xll::Bool` — `true` if the title was set successfully,
     *         `false` if the argument could not be coerced to a string.
     */
    inline Bool set_app_title(const xll::String& title)
    {
        XLOPER12 raw {};
        if (Excel12(xlfAppTitle, &raw, 1,
                    static_cast<LPXLOPER12>(const_cast<xll::String*>(&title))) != xlretSuccess ||
            raw.xltype != xltypeBool)
            return {false};

        return Bool(raw);
    }

}    // namespace xll


