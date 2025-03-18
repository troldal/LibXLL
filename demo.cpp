#include <gtl/phmap.hpp>
#include <iostream>
#include <windows.h>
#include "XLCALL.H"
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

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
__declspec(dllexport) LPXLOPER12 WINAPI MakeNum(double d) {

    // auto* result = new XLOPER12;
    // result->xltype = xltypeNum;
    // result->val.num = 42;
    std::this_thread::sleep_for(2000ms);

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
    XLOPER12 xFuncName;
    XLOPER12 xTypeText;
    XLOPER12 xWSFuncName;
    XLOPER12 xFuncArgs;
    XLOPER12 xRegisterResult;

    // Get the path of this XLL
    Excel12(xlGetName, &xDLLName, 0);

     xFuncName.xltype = xltypeStr;
    xFuncName.val.str = new_xll12string(L"MakeNum");

     xTypeText.xltype = xltypeStr;
    xTypeText.val.str = new_xll12string(L"UB$");

     xWSFuncName.xltype = xltypeStr;
    xWSFuncName.val.str = new_xll12string(L"MAKE_NUM");

     xFuncArgs.xltype = xltypeStr;
    xFuncArgs.val.str = new_xll12string(L"Arg");


    LPXLOPER12 args[5] = {&xDLLName, &xFuncName, &xTypeText, &xWSFuncName, &xFuncArgs};
    Excel12v(xlfRegister, &xRegisterResult, 5, args);


    return 1;
}
}