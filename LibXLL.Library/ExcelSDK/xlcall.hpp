//
// Created by kenne on 25/03/2025.
//

#pragma once

#ifdef _WIN32
    #include <windows.h>
    #include "windows/xlcall.h"
    #include "windows/xlcall_cpp.h"
#else
    #include "unix/xlcall.h"
    #include "unix/xlcall_cpp.h"
#endif