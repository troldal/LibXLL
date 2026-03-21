//
// Created by kenne on 21/03/2026.
//

#pragma once

#include <xlcall.hpp>
#include <type_traits>

namespace xll
{
    // Frees memory allocated by Excel during a call to Excel12()/Excel12v()
    // for the returned XLOPER12 value.
    //
    // It is safe to call this even when the type is not xltypeStr,
    // xltypeRef, or xltypeMulti. xlFree nulls the pointer in the
    // XLOPER12 after freeing, preventing double-free.
    //
    // Warning: for xltypeMulti do NOT call free() on any of the
    // individual elements; only free the array XLOPER12 itself.
    //
    // Accepts one or more references to any XLOPER12-derived objects.
    template<typename... Ts>
        requires (sizeof...(Ts) >= 1) &&
                 (std::is_base_of_v<XLOPER12, std::remove_reference_t<Ts>> && ...)
    inline void free(Ts&... opers)
    {
        if constexpr (sizeof...(opers) == 1) {
            // Common single-argument case: use Excel12 directly.
            Excel12(xlFree, nullptr, 1, static_cast<LPXLOPER12>(&opers)...);
        }
        else {
            // Multiple arguments: collect pointers and use Excel12v.
            LPXLOPER12 ptrs[] = { static_cast<LPXLOPER12>(&opers)... };
            Excel12v(xlFree, nullptr, static_cast<int>(sizeof...(Ts)), ptrs);
        }
    }

}    // namespace xll
