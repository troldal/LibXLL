/*
**  Microsoft Excel Developer's Toolkit
**  Version 14.0
**
**  File:           SRC\XLCALL.CPP
**  Description:    Code file for Excel callbacks
**  Platform:       Microsoft Windows
**
**  This file defines the entry points 
**  which are used in the Microsoft Excel C API.
**
*/
#pragma once

#ifndef _WINDOWS_
// #include <windows.h>
#endif

#include "xlcall.h"

/*
** Excel 12 entry points backwards compatible with Excel 11
**
** Excel12 and Excel12v ensure backwards compatibility with Excel 11
** and earlier versions. These functions will return xlretFailed when
** used to callback into Excel 11 and earlier versions
*/

#define cxloper12Max 255
#define EXCEL12ENTRYPT "MdCallBack12"

typedef int (*EXCEL12PROC) (int xlfn, int coper, LPXLOPER12 *rgpxloper12, LPXLOPER12 xloper12Res);

// inline HMODULE hmodule;
// inline EXCEL12PROC pexcel12;

inline __attribute__((used)) void FetchExcel12EntryPt(void)
{
	// Stub for compiling on Linux.
}

/*
** This function explicitly sets EXCEL12ENTRYPT.
**
** If the XLL is loaded not by Excel.exe, but by a HPC cluster container DLL,
** then GetModuleHandle(NULL) would return the process EXE module handle.
** In that case GetProcAddress would fail, since the process EXE doesn't
** export EXCEL12ENTRYPT ( since it's not Excel.exe).
**
** First try to fetch the known good entry point,
** then set the passed in address.
*/
#ifdef __cplusplus
extern "C"
#endif	
__attribute__((dllexport))
inline __attribute__((used)) void SetExcel12EntryPt(EXCEL12PROC pexcel12New)
{
	// Stub for compiling on Linux.
}

inline __attribute__((used)) int Excel12(int xlfn, LPXLOPER12 operRes, int count, ...)
{
	// Stub for compiling on Linux.
	return 0;
}

inline __attribute__((used)) int Excel12v(int xlfn, LPXLOPER12 operRes, int count, LPXLOPER12 opers[])
{
	// Stub for compiling on Linux.
	return 0;
}
