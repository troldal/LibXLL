//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Optional.hpp"
#include "../Types/SheetId.hpp"
#include "../Types/String.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Returns the full name of the sheet identified by @p id.
     *
     * `xll::SheetId` is itself an `xltypeRef` `XLOPER12` with `lpmref == nullptr`,
     * so it is passed directly as the argument to `xlSheetNm` without any
     * intermediate construction.
     *
     * @param id  The internal sheet identifier (obtain via `xll::sheet_id()`).
     *            Defaults to `SheetId(0)`, which corresponds to the current sheet.
     *            Note that the current sheet may change between calls, so explicitly
     *            passing the desired sheet ID is safer when the caller has already
     *            obtained it or when the current sheet may not be the intended one.
     *
     * @return The sheet name (e.g. `[Book1.xlsx]Sheet1`) as an `xll::String`,
     *         or `xll::None` if the call fails.
     *
     * @warning Passing an ID that no longer corresponds to an open sheet can
     *          crash Excel.  Validate the ID before calling this function.
     */
    inline Optional<String> sheet_name(const SheetId& id = SheetId(0))
    {
        XLOPER12 raw {};
        if (Excel12(xlSheetNm, &raw, 1,
                    static_cast<LPXLOPER12>(const_cast<SheetId*>(&id))) != xlretSuccess ||
            raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll

