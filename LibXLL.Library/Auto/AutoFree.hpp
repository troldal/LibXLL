//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "Auto.hpp"
#include "../Utils/MemoryManager.hpp"

extern "C" inline void __declspec(dllexport) WINAPI xlAutoFree12(LPXLOPER12 px)
{
    // if (px->xltype & xlbitDLLFree) {
    //     //		reinterpret_cast<XOPER<XLOPER12>*>(px)->~XOPER<XLOPER12>();
    //     delete reinterpret_cast<XOPER<XLOPER12>*>(px);
    //     px = nullptr;
    // }

    xll::MemoryManager::Instance().erase(px);

}