//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Auto.hpp"

extern "C" inline int __declspec(dllexport) WINAPI xlAutoOpen(void)
{
    using namespace xll::literals;
    try {
        // if (!xll::Auto<xll::BeforeOpen>::Execute()) {
        //     return FALSE;
        // }

        auto fun = xll::Function<xll::Number>("MakeNum"_xs, "MAKE_NUM"_xs, xll::Function<xll::Number>::Visible)
                       .Argument<xll::Number>("召唤"_xs, "FirstArgHelp"_xs)
                       .Argument<xll::Number>("активный"_xs, "SecondArgHelp"_xs);
                       //.ThreadSafe();

        xll::Register(fun.args.modulePath, fun.args.procedureName, fun.args.argTypes, fun.args.functionName, fun.args.argNames);

        xll::alert("Success!");

        // for (auto& [key, arg] : AddIn::Map) {
        //     ensure(Register(arg));
        // }
        //
        // if (!Auto<OpenAfter>::Call()) {
        //     return FALSE;
        // }
    }
    catch (const std::exception& ex) {
        // XLL_ERROR(ex.what());

        return FALSE;
    }
    catch (...) {
        // XLL_ERROR("Unknown exception in xlAutoOpen");

        return FALSE;
    }

    return TRUE;
}