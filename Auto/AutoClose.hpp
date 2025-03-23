//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Auto.hpp"

extern "C" inline int __declspec(dllexport) WINAPI xlAutoClose(void)
{
    try {
        // if (!Auto<CloseBefore>::Call()) {
        //     return FALSE;
        // }
        //
        // for (const auto& [key, args] : AddIn::Map) {
        //     Excel12(xlfSetName, key);
        // }
        //
        // if (!Auto<Close>::Call()) {
        //     return FALSE;
        // }
    }
    catch (const std::exception& ex) {
        //XLL_ERROR(ex.what());

        return FALSE;
    }
    catch (...) {
        //XLL_ERROR("Unknown exception in xlAutoClose");

        return FALSE;
    }

    return TRUE;
}