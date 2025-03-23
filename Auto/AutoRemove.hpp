//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Auto.hpp"

extern "C" inline int __declspec(dllexport) WINAPI xlAutoRemove(void)
{
    try {
        // if (!Auto<Remove>::Call()) {
        //     return FALSE;
        // }
    }
    catch (const std::exception& ex) {
        //XLL_ERROR(ex.what());

        return FALSE;
    }
    catch (...) {
        //XLL_ERROR("Unknown exception in xlAutoRemove");

        return FALSE;
    }

    return TRUE;
}