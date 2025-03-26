#ifdef _WIN32
    #include <windows.h>
#else
    #define __declspec(a) __attribute__((a))
    #define WINAPI
#endif

#include <Types.hpp>
#include <Commands.hpp>
#include <Functions.hpp>
#include <Auto.hpp>

using namespace std::literals;
using namespace xll::literals;

xll::AddInManagerInfo dllName([] { return xll::String("Awesome Add-In"); });


extern "C" {

// The actual function that returns 42
    __declspec(dllexport) xll::Array const* WINAPI MakeNum(xll::Variant const* d, xll::Array const* arr)
    {

        static xll::Array result;
        // result = xll::String("Hello");
        result = *arr;
        result.reshape(1,9);



        // auto elem1 = (*arr)[2,2];
        // auto elem2 = (*arr)[3,0];

        // auto f = xll::overload
        // {
        //     [](auto arg) { std::cerr << "Some value\n"; return true; },
        //     [](xll::Bool arg) { std::cerr << "Boolean\n"; return true; },
        //     [](xll::Number arg) { std::cerr << "Number\n"; return true; }
        // };
        //
        // auto r = xll::visit(f, *d);
        //
        // // auto x = get<xll::Number>(*d);
        // // std::cerr << x << std::endl;
        //
        //
        // auto _int1 = xll::Int(42);
        // auto _int2 = xll::Int(43);
        // auto _int3 = _int1 + 43;
        // xll::Int _int6 = 23;
        //
        // auto num1 = xll::Number(3.14);
        // auto num2 = xll::Number(2.71);
        // static auto num3 = num1 + num2;
        //
        // xll::Int _int4 = num1;
        //
        // bool var1 = true;
        // var1 -= true;
        //
        // xll::Bool var2 = true;
        //
        // xll::Nil nil1;
        // xll::Nil nil2;
        //
        // auto str1 = xll::String("42");
        // auto str2 = xll::String("43");
        // static auto str3 = str1 + str2;
        //
        // xll::Variant var3 = str3;




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

        // static auto result = xll::String("42");
        //result.xltype |= xlbitDLLFree;

        //return xll::MemoryManager::Instance().add(result);
        return &result;

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