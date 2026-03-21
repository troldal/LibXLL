//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Optional.hpp"
#include "../Types/SingleRef.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Returns a reference to the active cell on the active worksheet.
     *
     * @return An `xll::SingleRef` identifying the active cell,
     *         or `xll::None` if the reference could not be obtained.
     */
    inline Optional<SingleRef> active_cell()
    {
        XLOPER12 raw {};
        if (Excel12(xlfActiveCell, &raw, 0) != xlretSuccess || raw.xltype != xltypeSRef)
            return None;

        // xltypeSRef carries no heap allocation — xlFree is not required.
        return SingleRef(raw.val.sref.ref.rwFirst, raw.val.sref.ref.rwLast,
                         raw.val.sref.ref.colFirst, raw.val.sref.ref.colLast);
    }

}    // namespace xll

