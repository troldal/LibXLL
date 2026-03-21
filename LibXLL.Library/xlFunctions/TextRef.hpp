//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Types/MultiRef.hpp"
#include "../Types/Optional.hpp"
#include "../Types/String.hpp"
#include "SheetId.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Converts a text cell reference to an external range reference on a
     *        specified sheet.
     *
     * Wraps the Excel C API function `xlfTextref` (enumeration value 147) and
     * overrides the sheet identifier with the supplied @p sheet argument.
     *
     * @param sheet    The sheet on which the reference should be located.
     * @param ref      The cell reference as an `xll::String`, e.g. `"A1"` or
     *                 `"B3:C10"`. Must not have a leading `'='`.
     * @param a1_style `true` (default) for A1 notation; `false` for R1C1 notation.
     *
     * @return An `xll::MultiRef` with the decoded row/column bounds and the
     *         supplied sheet ID, or `xll::None` if the call fails or the
     *         reference string is invalid.
     *
     * @note Callable from commands and macro sheet functions only.
     */
    inline Optional<MultiRef> text_ref(const SheetId& sheet, const String& ref,
                                        Bool a1_style = Bool(true))
    {
        XLOPER12 raw {};
        if (Excel12(xlfTextref, &raw, 2,
                    static_cast<LPXLOPER12>(const_cast<String*>(&ref)),
                    static_cast<LPXLOPER12>(&a1_style)) != xlretSuccess ||
            raw.xltype != xltypeRef)
            return None;

        MultiRef result = reinterpret_cast<const MultiRef&>(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        result.set_sheet_id(sheet);
        return result;
    }

    /**
     * @brief Converts a text cell reference to an external range reference on the
     *        currently active sheet.
     *
     * Wraps the Excel C API function `xlfTextref` (enumeration value 147).
     * Because `xlfTextref` resolves unqualified references (e.g. `"A1"`) relative
     * to the XLL's internal macro execution sheet rather than the visible worksheet,
     * the returned sheet ID is always replaced with the value from `xll::sheet_id()`.
     *
     * @param ref      The cell reference as an `xll::String`, e.g. `"A1"` or
     *                 `"B3:C10"`. Must not have a leading `'='`.
     * @param a1_style `true` (default) for A1 notation; `false` for R1C1 notation.
     *
     * @return An `xll::MultiRef` with the decoded row/column bounds and the active
     *         sheet's ID, or `xll::None` if the call fails, the reference string is
     *         invalid, or the active sheet ID cannot be obtained.
     *
     * @note Callable from commands and macro sheet functions only.
     * @note `xlfGetName` returns named-range addresses prefixed with `'='`;
     *       strip the leading character before passing to this function.
     */
    inline Optional<MultiRef> text_ref(const String& ref, Bool a1_style = Bool(true))
    {
        auto sid = sheet_id();
        if (!sid)
            return None;
        return text_ref(*sid, ref, a1_style);
    }

}    // namespace xll
