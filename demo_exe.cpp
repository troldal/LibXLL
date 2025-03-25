//
// Created by kenne on 24/03/2025.
//

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <windows.h>
#include "Types/String.hpp"
#include "Types/Variant.hpp"
#include "Types/Array.hpp"

extern "C" xll::String const* WINAPI MakeNum(xll::String const* d, xll::String const* arr);

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    {
        xll::String arg1 = xll::String("arg1");
        xll::String arg2 = xll::String("arg2");

        auto result = MakeNum(&arg1, &arg2);
        std::cout << *result << std::endl;

        // std::string* str = new std::string("Hello");
        // std::cout << *str << std::endl;

        auto arg3 = *arg1;
        auto arg4 = xll::String::lift(&arg3);

    }


    // _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    // _CrtDumpMemoryLeaks();

    _CrtMemState state;
    _CrtMemCheckpoint(&state);
    // Optional: Print only leaks above a certain size
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtMemDumpAllObjectsSince(&state);

    return 0;
}