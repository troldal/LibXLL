//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Utils/Concepts.hpp"
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Blanks all cells in @p target.
     *
     * Calls `xlSet` with no `Value` argument, which clears every cell in the
     * target range.
     *
     * @tparam TRef  An `xll::SingleRef` or `xll::MultiRef` (any `is_xll_type`
     *               whose `xltype` is `xltypeSRef` or `xltypeRef`).
     *
     * @param target  Reference to the cell(s) to blank.
     *
     * @return `xll::Bool` — `true` if the operation succeeded.
     */
    template<typename TRef>
        requires is_xll_type<TRef>
    inline Bool set_cell_value(const TRef& target)
    {
        XLOPER12 raw {};
        if (Excel12(xlSet, &raw, 1,
                    static_cast<LPXLOPER12>(const_cast<TRef*>(&target))) != xlretSuccess ||
            raw.xltype != xltypeBool)
            return Bool(false);

        return Bool(raw);
    }

    /**
     * @brief Writes @p value into the cell(s) identified by @p target.
     *
     * @tparam TRef    An `xll::SingleRef` or `xll::MultiRef`.
     * @tparam TValue  Any xll value or array type (`xll::Number`, `xll::Int`,
     *                 `xll::String`, `xll::Bool`, `xll::Error`,
     *                 `xll::Array<T>`, `xll::Nil`).
     *
     * @param target  Reference to the target cell(s).
     * @param value   The value (or array) to write.  An `xll::Nil` value
     *                blanks the corresponding cell(s).
     *
     * @return `xll::Bool` — `true` if the operation succeeded.
     */
    template<typename TRef, typename TValue>
        requires is_xll_type<TRef> && is_xll_type<TValue>
    inline Bool set_cell_value(const TRef& target, const TValue& value)
    {
        XLOPER12 raw {};
        if (Excel12(xlSet, &raw, 2,
                    static_cast<LPXLOPER12>(const_cast<TRef*>(&target)),
                    static_cast<LPXLOPER12>(const_cast<TValue*>(&value))) != xlretSuccess ||
            raw.xltype != xltypeBool)
            return Bool(false);

        return Bool(raw);
    }

}    // namespace xll

