//
// Created by kenne on 26/03/2025.
//

#pragma once

#ifdef _WIN32
  #include <windows.h>
  #define XLLAPI WINAPI
  #define XLL_EXPORTS __declspec(dllexport)
  #define XLL_SUCCESS 1
  #define XLL_FAILURE 0
#else
  #define __declspec(a) __attribute__((a))
  #define XLLAPI
  // #define XLL_EXPORTS __declspec(dllexport) __declspec(used)
  #define XLL_EXPORTS __declspec(used)
  #define XLL_SUCCESS 1
  #define XLL_FAILURE 0
  #define TRUE 1
  #define FALSE 0
#endif

// #ifdef _WIN32
//
// #else
// #
// #    define WINAPI
// #endif