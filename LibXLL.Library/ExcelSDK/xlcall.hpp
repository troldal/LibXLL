//
// Created by kenne on 25/03/2025.
//

#pragma once

#ifdef _WIN32
    #include <windows.h>
    #include "windows/xlcall.h"
    #include "windows/xlcall_cpp.h"

// extern "C" inline __declspec(dllexport) BOOL WINAPI
// DllMain(HINSTANCE hDLL, ULONG reason, LPVOID lpReserved)
// {
//     switch (reason) {
//         case DLL_PROCESS_ATTACH:
//             //xll_hModule = hDLL;
//             DisableThreadLibraryCalls(hDLL);
//             break;
//         case DLL_THREAD_ATTACH:
//             break;
//         case DLL_THREAD_DETACH:
//             break;
//         case DLL_PROCESS_DETACH:
//             break;
//     }
//
//     return TRUE;
// }

#else
    #include "unix/xlcall.h"
    #include "unix/xlcall_cpp.h"
#endif