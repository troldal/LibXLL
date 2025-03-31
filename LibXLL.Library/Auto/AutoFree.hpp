/*

     .d88888b.                             Y88b   d88P 888      888
    d88P" "Y88b                             Y88b d88P  888      888
    888     888                              Y88o88P   888      888
    888     888 88888b.   .d88b.  88888b.     Y888P    888      888
    888     888 888 "88b d8P  Y8b 888 "88b    d888b    888      888
    888     888 888  888 88888888 888  888   d88888b   888      888
    Y88b. .d88P 888 d88P Y8b.     888  888  d88P Y88b  888      888
     "Y88888P"  88888P"   "Y8888  888  888 d88P   Y88b 88888888 88888888
                888
                888
                888

    This code is based on xlladdins by Keith Lewis provided under the MIT License.
    Modifications have been made by Kenneth Troldal Balslev/KinetiQ.dev

    MIT License

    Copyright (c) 2022 xlladdins
    Copyright (c) 2025 Kenneth Troldal Balslev and KinetiQ.dev

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include "Auto.hpp"
#include "../Utils/MemoryManager.hpp"

extern "C" inline XLL_EXPORTS void XLLAPI xlAutoFree12(LPXLOPER12 px)
{
    if (not px->xltype & xlbitDLLFree) return;

    px->xltype &= ~xlbitDLLFree;
    switch (px->xltype) {
        case xltypeMulti:
            delete reinterpret_cast<xll::Array*>(px);
            break;
        case xltypeBool:
            delete reinterpret_cast<xll::Bool*>(px);
            break;
        case xltypeErr:
            delete reinterpret_cast<xll::Error*>(px);
            break;
        case xltypeInt:
            delete reinterpret_cast<xll::Int*>(px);
            break;
        case xltypeMissing:
            delete reinterpret_cast<xll::Missing*>(px);
            break;
        case xltypeNil:
            delete reinterpret_cast<xll::Nil*>(px);
            break;
        case xltypeNum:
            delete reinterpret_cast<xll::Number*>(px);
            break;
        case xltypeStr:
            delete reinterpret_cast<xll::String*>(px);
            break;
        default:
            break;
    }
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoFree12")
#endif