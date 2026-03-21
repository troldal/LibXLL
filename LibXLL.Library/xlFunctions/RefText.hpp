//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Types/Optional.hpp"
#include "../Types/String.hpp"
#include "../Utils/Concepts.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Converts a cell reference to its text representation.
     *
     * Wraps the Excel C API function `xlfReftext` (enumeration value 146).
     *
     * The returned string has the form `[Book1.xls]Sheet1!A1` (A1 style) or
     * `[Book1.xls]Sheet1!R1C1` (R1C1 style).
     *
     * @tparam TRef    An `xll::SingleRef` (`xltypeSRef`) or `xll::MultiRef`
     *                 (`xltypeRef`), or any other `is_xll_type` that Excel
     *                 accepts as a reference argument.
     *
     * @param ref      The reference to convert.
     * @param a1_style `true` (default) for A1 notation; `false` for R1C1 notation.
     *
     * @return The reference as an `xll::String`, or `xll::None` if the call
     *         fails or @p ref is not a valid reference type.
     *
     * @note Callable from commands and macro sheet functions only.
     */
    template<typename TRef>
        requires is_xll_type<TRef>
    inline Optional<String> ref_text(const TRef& ref, Bool a1_style = Bool(true))
    {
        XLOPER12 raw {};
        if (Excel12(xlfReftext, &raw, 2,
                    static_cast<LPXLOPER12>(const_cast<TRef*>(&ref)),
                    static_cast<LPXLOPER12>(&a1_style)) != xlretSuccess ||
            raw.xltype != xltypeStr)
            return None;

        // Copy the Excel-owned Pascal string into an xll::String, then free
        // the original buffer — same pattern as xll::get_name().
        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll
