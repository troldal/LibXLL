//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Optional.hpp"
#include "../Types/SheetId.hpp"
#include "../Types/String.hpp"
#include <string_view>
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Returns the `SheetId` of the currently active sheet.
     *
     * Calls `Excel12(xlSheetId, ...)` with no arguments.
     *
     * @return The `xll::SheetId` of the active sheet, or `std::nullopt` if
     *         the call fails (e.g. called outside a valid Excel context).
     */
    inline Optional<SheetId> sheet_id()
    {
        XLOPER12 raw {};
        if (Excel12(xlSheetId, &raw, 0) == xlretSuccess && raw.xltype == xltypeRef)
            return SheetId(raw);
        return None;
    }

    /**
     * @brief Returns the `SheetId` of the sheet named by @p name.
     *
     * Overload accepting an already-constructed `xll::String`, avoiding an
     * extra allocation when the caller already holds one.
     *
     * @param name The sheet name as an `xll::String` (`xltypeStr`).
     *
     * @return The `xll::SheetId` for the named sheet, or `std::nullopt` if
     *         the sheet is not found or the call fails.
     */
    inline Optional<SheetId> sheet_id(const xll::String& name)
    {
        XLOPER12 raw {};
        if (Excel12(xlSheetId, &raw, 1,
                    static_cast<LPXLOPER12>(const_cast<xll::String*>(&name))) == xlretSuccess &&
            raw.xltype == xltypeRef)
            return SheetId(raw);
        return None;
    }

}    // namespace xll

