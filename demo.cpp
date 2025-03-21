#include <gtl/phmap.hpp>
#include <iostream>
#include <windows.h>
#include "xlcall.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "Types/String.hpp"
#include "Types/Number.hpp"
#include "Types/Int.hpp"
#include "Types/Nil.hpp"
#include "Register/Arg.hpp"
#include "Register/Function.hpp"
#include "Commands/Alert.hpp"

using namespace std::literals;
using namespace xll::literals;

wchar_t* new_xll12string(const wchar_t* text) {
    size_t len;

    if (!text || !(len = wcslen(text))) {
        return NULL;
    }

    wchar_t* p = (wchar_t*) malloc((len + 2) * sizeof(wchar_t));
    if (!p) return NULL;

    memcpy(p+1, text, (len + 1) * sizeof(wchar_t));
    p[0] = len;
    return p;
}

static gtl::parallel_flat_hash_map<LPXLOPER12, std::unique_ptr<XLOPER12>> cache;

void addOper(LPXLOPER12 ptr, std::unique_ptr<XLOPER12> oper) {
    std::cerr << "Adding XLOPER to cache" << std::endl;
    cache[ptr] = std::move(oper);
}

extern "C" {

// The actual function that returns 42
__declspec(dllexport) LPXLOPER12 WINAPI MakeNum(xll::Number const* d, xll::String const* d2) {

    // auto* result = new XLOPER12;
    // result->xltype = xltypeNum;
    // result->val.num = 42;
    // auto str = xll::String(d);
    //std::string str = *d;
    //std::cout << str << std::endl;
    // auto str = *d + ", "_xs + *d2;
    // if (str == "Hello, World"_xs)
    //     std::cout << str << std::endl;

    // auto opt = d->optional();
    // opt.transform([](xll::String str) { std::cout << str << std::endl; return str; });

    double v = *d;
    std::cout << *d << std::endl;

    // std::this_thread::sleep_for(2000ms);

    auto result = std::make_unique<XLOPER12>();
    result->xltype = xltypeNum | xlbitDLLFree;
    result->val.num = 42;

    auto ref = result.get();
    // cache[ref] = std::move(result);
    addOper(ref, std::move(result));

    return cache[ref].get();
}

__declspec(dllexport) void WINAPI xlAutoFree12(LPXLOPER12 px)
{
    std::cerr << "Deleting XLOPER from cache" << std::endl;
    if (cache.contains(px))
        cache.erase(px);

    std::cerr << "Cache size: " << cache.size() << std::endl;

}

// Add-in initialization function
__declspec(dllexport) int WINAPI xlAutoOpen(void) {
    XLOPER12 xDLLName;
    XLOPER12 xRegisterResult;

    auto arg = xll::Arg<xll::String>("BLAH"_xs, "BLAH_Help"_xs);

    xll::alert("Test!"_xs, xll::Alert::Question);

    // Get the path of this XLL
    Excel12(xlGetName, &xDLLName, 0);

    auto xFuncName = xll::String("MakeNum");
    auto xTypeText = xll::String("UQQ$");
    auto xWSFuncName = xll::String("MAKE_NUM");
    auto xFuncArgs = xll::String("召唤 with активный");

    LPXLOPER12 args[5] = {&xDLLName, &xFuncName, &xTypeText, &xWSFuncName, &xFuncArgs};
    Excel12v(xlfRegister, &xRegisterResult, 5, args);


    return 1;
}
}