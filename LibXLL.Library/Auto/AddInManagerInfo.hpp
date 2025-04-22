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

#include <functional>
#include "../Types.hpp"
#include "../Functions.hpp"
#include "../Commands.hpp"

namespace xll
{

    class AddInManagerInfo
    {
        inline static std::function<xll::String()> s_func;

    public:
        AddInManagerInfo(std::function<xll::String()>&& f) { s_func = std::move(f); }
        static xll::String Execute()
        {
            if (s_func) return s_func();
            return xll::String("NONAME");
        }
    };
}    // namespace xll

extern "C" inline XLL_EXPORTS LPXLOPER12 XLLAPI xlAddInManagerInfo12(xll::Int* action)
{
    static XLOPER12 o;
    // static const auto name = xll::String("My Add-in");
    static const auto name = xll::AddInManagerInfo::Execute();

    try {
        // coerce to int and check if action is 1
        if (const auto action_ = xll::coerce<xll::Int>(action); action_ == 1) {
            o.xltype  = xltypeStr;
            o.val.str = name.val.str;
        }
        else {
            o.xltype  = xltypeErr;
            o.val.err = 42;
        }
    }
    catch (const std::exception& ex) {
        xll::alert(ex.what(), xll::Alert::Error);
        o.xltype  = xltypeErr;
        o.val.err = 42;
    }
    catch (...) {
        xll::alert("Unknown exception in xlAddInManagerInfo12", xll::Alert::Error);
        o.xltype  = xltypeErr;
        o.val.err = 42;
    }

    return &o;
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAddInManagerInfo12")
#endif