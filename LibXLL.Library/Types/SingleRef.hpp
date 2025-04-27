//
// Created by kenne on 25/04/2025.
//

#pragma once

#include <xlcall.hpp>

namespace xll
{
    class SingleRef : public XLOPER12
    {
    public:
        enum Cell { First, Last };

        SingleRef() : XLOPER12()
        {
            xltype = xltypeSRef;
            val.sref.ref.rwFirst = 0;
            val.sref.ref.rwLast  = 0;
            val.sref.ref.colFirst = 0;
            val.sref.ref.colLast  = 0;
        }

        template<Cell cell>
        auto row() const
        {
            if constexpr (cell == First)
                return val.sref.ref.rwFirst;
            else
                return val.sref.ref.rwLast;
        }

        template<Cell cell>
        auto col() const
        {
            if constexpr (cell == First)
                return val.sref.ref.colFirst;
            else
                return val.sref.ref.colLast;
        }
    };
}    // namespace xll