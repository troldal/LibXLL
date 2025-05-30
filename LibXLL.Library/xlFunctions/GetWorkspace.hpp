//
// Created by kenne on 30-05-2025.
//

#pragma once
#include "Types/Array.hpp"

namespace xll
{
    enum struct Workspace
    {
        EnvironmentName = 1,
        ExcelVersion = 2,
        DecimalCount = 3,
        R1C1Mode = 4,
        ScrollBarsVisible = 5,
        /* ... */
        LocaleData = 37
    };

    namespace impl
    {
        template <xll::Workspace index>
        struct Index
        {
            constexpr static auto value = index;
        };

        inline auto workspace(Index<Workspace::LocaleData>)
        {
            XLOPER12 data {};
            Excel12(xlfGetWorkspace, &data, 1, xll::Int(37));
            auto result = reinterpret_cast<xll::Array<xll::Variant<xll::Nil, xll::String, xll::Number, xll::Bool>>&>(data);
            Excel12(xlFree, nullptr, 1, &data);
            return result;
        }

    }

    template <xll::Workspace index>
    auto workspace()
    {
        return impl::workspace(impl::Index<index>{});
    }

}    // namespace xll