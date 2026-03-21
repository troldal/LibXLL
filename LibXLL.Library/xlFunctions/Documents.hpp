//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Array.hpp"
#include "../Types/String.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Returns a row vector of all open workbook document names.
     *
     * Wraps the Excel C API function `xlfDocuments` (enumeration value 93).
     *
     * Each element in the returned array is the name of an open workbook,
     * exactly as Excel reports it (typically the file name without path).
     *
     * @return A 1 × N `Array<String>` containing one entry per open workbook,
     *         or an empty `Array<String>` if the call fails or no workbooks
     *         are open (Excel returns #NA in that case).
     *
     * @note Callable from commands and macro sheet functions only.
     */
    inline Array<String> documents()
    {
        XLOPER12 raw {};
        if (Excel12(xlfDocuments, &raw, 0) != xlretSuccess || raw.xltype != xltypeMulti)
            return {};

        // Copy-construct while the Excel-owned buffer is still live.
        // Array<String>'s copy constructor deep-copies every String element
        // into freshly owned (new[]) storage, so the result is independent
        // of the Excel buffer that xlFree releases immediately after.
        Array<String> result = reinterpret_cast<const Array<String>&>(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll

