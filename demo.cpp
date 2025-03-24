#include <windows.h>
#include "Register/Arg.hpp"
#include "Register/Function.hpp"
#include "Types/Int.hpp"
#include "Types/Nil.hpp"
#include "Types/Number.hpp"
#include "Types/String.hpp"
#include "xlCommands/Alert.hpp"
#include "xlFunctions/GetName.hpp"
#include "xlFunctions/Register.hpp"
#include "xlFunctions/Coerce.hpp"
#include "xlcall.h"
#include <chrono>
// #include <gtl/phmap.hpp>
#include <iostream>
#include <thread>
#include "Auto/AddInManagerInfo.hpp"
#include "Auto/AutoOpen.hpp"
#include "Auto/AutoClose.hpp"
#include "Auto/AutoAdd.hpp"
#include "Auto/AutoRemove.hpp"
#include "Auto/AutoFree.hpp"
#include "Utils/MemoryManager.hpp"
#include "Types/Error.hpp"
#include "Types/Bool.hpp"
#include "Types/Missing.hpp"


using namespace std::literals;
using namespace xll::literals;

xll::AddInManagerInfo dllName([] { return xll::String("Awesome Add-In"); });


// static gtl::parallel_flat_hash_map<LPXLOPER12, std::unique_ptr<XLOPER12>> cache;
//
// void addOper(LPXLOPER12 ptr, std::unique_ptr<XLOPER12> oper) {
//     std::cerr << "Adding XLOPER to cache" << std::endl;
//     cache[ptr] = std::move(oper);
// }

extern "C" {

// The actual function that returns 42
    __declspec(dllexport) xll::String const* WINAPI MakeNum(xll::Number const* d, xll::String const* d2)
    {


        auto _int1 = xll::Int(42);
        auto _int2 = xll::Int(43);
        auto _int3 = _int1 + 43;
        xll::Int _int6 = 23;

        auto num1 = xll::Number(3.14);
        auto num2 = xll::Number(2.71);
        static auto num3 = num1 + num2;

        xll::Int _int4 = num1;

        bool var1 = true;
        var1 -= true;

        xll::Bool var2 = true;

        xll::Nil nil1;
        xll::Nil nil2;

        auto str1 = xll::String("42");
        auto str2 = xll::String("43");
        static auto str3 = str1 + str2;

        std::cerr << str3 << std::endl;



        // auto* result = new XLOPER12;
        // result->xltype = xltypeNum;
        // result->val.num = 42;
        // auto str = xll::String(d);
        // std::string str = *d;
        // std::cout << str << std::endl;
        // auto str = *d + ", "_xs + *d2;
        // if (str == "Hello, World"_xs)
        //     std::cout << str << std::endl;

        // auto opt = d->optional();
        // opt.transform([](xll::String str) { std::cout << str << std::endl; return str; });

        // double v = *d;
        // std::cout << *d << std::endl;

        // std::this_thread::sleep_for(2000ms);

        static auto result = xll::String("42");
        //result.xltype |= xlbitDLLFree;

        //return xll::MemoryManager::Instance().add(result);
        return &str3;

        // auto result     = std::make_unique<XLOPER12>();
        // result->xltype  = xltypeNum | xlbitDLLFree;
        // result->val.num = 42;
        //
        // auto ref = result.get();
        // // cache[ref] = std::move(result);
        // addOper(ref, std::move(result));
        //
        // return cache[ref].get();
    }

// __declspec(dllexport) void WINAPI xlAutoFree12(LPXLOPER12 px)
// {
//     std::cerr << "Deleting XLOPER from cache" << std::endl;
//     if (cache.contains(px))
//         cache.erase(px);
//
//     std::cerr << "Cache size: " << cache.size() << std::endl;
//
// }



}