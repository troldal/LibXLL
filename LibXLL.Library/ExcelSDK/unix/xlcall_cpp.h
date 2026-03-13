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
#include <cstdarg>

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

// Single callback-pointer instance within this shared library.
// Hidden visibility prevents the Linux dynamic linker from merging this symbol
// with identically-named symbols in other XLLs loaded into the same process.
__attribute__((visibility("hidden"))) inline EXCEL12PROC pexcel12 = nullptr;

// FetchExcel12EntryPt is a no-op on Linux: there is no Excel process to query.
// It exists solely to mirror the Windows API so shared XLL source code compiles
// unchanged on both platforms.
inline __attribute__((used)) void FetchExcel12EntryPt(void)
{
}

/*
** SetExcel12EntryPt — called by the host (e.g. MockXL) to inject the dispatch
** callback.  Exported with default visibility so the host can locate it via
** dlsym() after loading the XLL.  Mirrors the Windows __declspec(dllexport)
** behaviour.
**
** On Windows, SetExcel12EntryPt only overwrites pexcel12 when it is still null
** (FetchExcel12EntryPt may have already filled it from MdCallBack12).  On Linux,
** FetchExcel12EntryPt is a no-op, so pexcel12 is always null on entry and the
** same conditional is preserved for source compatibility.
*/
#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default")))
inline __attribute__((used)) void SetExcel12EntryPt(EXCEL12PROC pexcel12New)
{
    FetchExcel12EntryPt();
    if (pexcel12 == nullptr)
        pexcel12 = pexcel12New;
}

// Excel12 and Excel12v are defined here as inline functions that forward calls
// through the pexcel12 callback installed by SetExcel12EntryPt.
//
// Hidden visibility ensures:
//   • Each XLL has its own copy resolved at link time — the dynamic linker
//     will not substitute a definition from another XLL or the host.
//   • Calls from within the XLL bind to this definition at link time
//     (no PLT indirection), so pexcel12 is always the local copy.

__attribute__((visibility("hidden")))
inline __attribute__((used)) int Excel12(int xlfn, LPXLOPER12 operRes, int count, ...)
{
    FetchExcel12EntryPt();
    if (pexcel12 == nullptr)
        return xlretFailed;

    if (count < 0 || count > cxloper12Max)
        return xlretInvCount;

    LPXLOPER12 rgxloper12[cxloper12Max];
    std::va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; ++i)
        rgxloper12[i] = va_arg(ap, LPXLOPER12);
    va_end(ap);

    return pexcel12(xlfn, count, &rgxloper12[0], operRes);
}

__attribute__((visibility("hidden")))
inline __attribute__((used)) int Excel12v(int xlfn, LPXLOPER12 operRes, int count, LPXLOPER12 opers[])
{
    FetchExcel12EntryPt();
    if (pexcel12 == nullptr)
        return xlretFailed;

    return pexcel12(xlfn, count, &opers[0], operRes);
}

#ifdef __cplusplus
}
#endif
