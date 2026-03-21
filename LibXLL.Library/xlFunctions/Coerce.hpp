//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Types/Int.hpp"
#include "Types/Optional.hpp"
#include "Utils/Concepts.hpp"

#include <xlcall.hpp>

namespace xll
{
    // Converts an XLOPER12 to a TResult using Excel's xlCoerce.
    //
    // Returns xll::None if:
    //   - Excel12(xlCoerce) does not return xlretSuccess, or
    //   - The type of the returned XLOPER12 does not match TResult::excel_type.
    //
    // Memory: a raw XLOPER12 is used as the output buffer so that no memory is
    // leaked by a non-trivial TResult default constructor (e.g. xll::String
    // allocates a heap buffer for "" — overwriting val.str via Excel12 without
    // freeing that buffer would leak it). Excel's allocation is freed with
    // xlFree after TResult has been copy-constructed from the raw result.
    template<typename TResult, typename TArg>
        requires is_xll_type<TResult> && is_xll_type<TArg>
    Optional<TResult> coerce(const TArg& src)
    {
        // Raw XLOPER12: zero-initialised, no constructor side-effects.
        XLOPER12 raw {};

        xll::Int destType {};
        destType.val.w = static_cast<short>(TResult::excel_type);

        if (Excel12(xlCoerce, &raw, 2, static_cast<LPXLOPER12>(const_cast<TArg*>(&src)), &destType) != xlretSuccess)
            return None;

        // Verify Excel actually produced the requested type.
        // (A bit-AND is used because target_type may be a combination of bits.)
        if ((raw.xltype & static_cast<int>(TResult::excel_type)) == 0) {
            Excel12(xlFree, nullptr, 1, &raw);
            return None;
        }

        // Construct TResult from the raw result — this is a deep copy for
        // types like xll::String that own heap memory.
        TResult result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll

