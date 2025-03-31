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
#include "../Macros/Defines.hpp"
#include "../Register.hpp"
#include "../Register/Function.hpp"

extern "C" inline XLL_EXPORTS int XLLAPI xlAutoOpen()
{
  return xll::xlAuto<xll::Open>("xlAutoOpen", []{ for (const auto& fn : xll::Function::functionArgs) xll::Register(xll::impl::All(fn)); });
    // using namespace xll::literals;
    // try {
    //
    //     xll::Registry::instance().register_all();
    //
    //     if (!xll::Auto<xll::Open>::Execute<xll::Auto<xll::Open>::BeforeTag>()) return XLL_FAILURE;
    //
    //     for (const auto& fn : xll::Function::functionArgs) xll::Register(xll::impl::All(fn));
    //
    //
    //     if (!xll::Auto<xll::Open>::Execute<xll::Auto<xll::Open>::AfterTag>()) return XLL_FAILURE;
    //
    //     xll::Auto<xll::Open>::HandleError("Blah");
    //
    // }
    // catch (const std::exception& ex) {
    //     xll::Auto<xll::Open>::HandleError(ex.what());
    //     return XLL_FAILURE;
    // }
    // catch (...) {
    //     xll::Auto<xll::Open>::HandleError("Unknown error in xlAutoOpen");
    //     return XLL_FAILURE;
    // }
    //
    // return XLL_SUCCESS;
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoOpen")
#endif