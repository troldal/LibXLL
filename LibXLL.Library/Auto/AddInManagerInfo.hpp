//
// Created by kenne on 21/03/2025.
//

#pragma once

#include <functional>

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

    // namespace impl
    // {
    //
    // }    // namespace impl

}    // namespace xll

extern "C" __declspec(dllexport) inline LPXLOPER12 WINAPI xlAddInManagerInfo12(xll::Int* action)
{
    static XLOPER12 o;
    // static const auto name = xll::String("My Add-in");
    static const auto name = xll::AddInManagerInfo::Execute();

    try {
        // coerce to int and check if action is 1
        if (const auto action_ = xll::coerce<xll::Int>(action); action_.has_value() && action_ == 1) {
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