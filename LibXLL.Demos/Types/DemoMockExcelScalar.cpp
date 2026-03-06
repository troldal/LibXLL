//
// Created by Copilot on 06/03/2026.
//
// Mock-Excel executable that loads the scalar_validation.xll add-in at runtime
// and calls its exported functions with both correct and incorrect XLOPER12 types.
//
// This demonstrates the runtime type-safety of LibXLL's basic/scalar types:
// when an xll::Number function receives an XLOPER12 whose xltype is xltypeStr,
// the ensure() validation inside the arithmetic operators throws an exception.
//
// Build: this is a standalone .exe that uses LoadLibrary / GetProcAddress
// to call into the .xll (which is just a .dll with a different extension).

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <filesystem>
#include <iostream>
#include <string>
#include <format>
#include <xlcall.hpp>

// ============================================================================
// Portable dynamic library helpers
// ============================================================================

#ifdef _WIN32
using LibHandle = HMODULE;
using FuncPtr   = FARPROC;

LibHandle load_library(const char* path)
{
    // Convert to absolute path — LoadLibraryA does not search the current
    // working directory by default on modern Windows.
    auto abs = std::filesystem::absolute(path);
    auto abs_str = abs.string();

    // Add the XLL's parent directory to the DLL search path so that any
    // runtime dependencies (e.g. MinGW's libstdc++-6.dll, libgcc_s_seh-1.dll)
    // that sit next to the .xll can be found automatically.
    auto dir = abs.parent_path().string();
    SetDllDirectoryA(dir.c_str());

    return LoadLibraryA(abs_str.c_str());
}
void      free_library(LibHandle h)      { if (h) FreeLibrary(h); }
FuncPtr   get_proc(LibHandle h, const char* name) { return GetProcAddress(h, name); }
std::string last_error()
{
    DWORD err = GetLastError();
    char buf[256]{};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buf, sizeof(buf), nullptr);
    return buf;
}
#else
using LibHandle = void*;
using FuncPtr   = void*;

LibHandle load_library(const char* path)
{
    // Convert to absolute path for consistency with the Windows branch.
    auto abs = std::filesystem::absolute(path).string();
    return dlopen(abs.c_str(), RTLD_NOW);
}
void      free_library(LibHandle h)      { if (h) dlclose(h); }
FuncPtr   get_proc(LibHandle h, const char* name) { return dlsym(h, name); }
std::string last_error() { return dlerror() ? dlerror() : "unknown error"; }
#endif

// ============================================================================
// Function pointer types matching the XLL exports
// ============================================================================

// All XLL-exported functions use the XLOPER12 layout via xll:: types,
// but at the ABI level they are just XLOPER12 pointers.
// On x86_64, __stdcall is ignored (only one calling convention), so
// we use __cdecl-style pointers universally. On 32-bit MSVC builds,
// __stdcall decoration would apply, but this demo targets x86_64.
using AddNumbersFn    = XLOPER12* (*)(const XLOPER12*, const XLOPER12*);
using NegateBoolFn    = XLOPER12* (*)(const XLOPER12*);
using StringLengthFn  = XLOPER12* (*)(const XLOPER12*);
using AutoFreeFn      = void      (*)(const XLOPER12*);

// ============================================================================
// Helpers to create raw XLOPER12 values of various types
// ============================================================================

XLOPER12 make_number(double v)
{
    XLOPER12 x{};
    x.xltype  = xltypeNum;
    x.val.num = v;
    return x;
}

XLOPER12 make_bool(bool v)
{
    XLOPER12 x{};
    x.xltype    = xltypeBool;
    x.val.xbool = v ? 1 : 0;
    return x;
}

XLOPER12 make_int(int v)
{
    XLOPER12 x{};
    x.xltype = xltypeInt;
    x.val.w  = v;
    return x;
}

// Creates a Pascal-style wide string owned by a static buffer
// (sufficient for this demo — no dynamic allocation needed).
static wchar_t g_str_buf[64];

XLOPER12 make_string(const wchar_t* text)
{
    size_t len = wcslen(text);
    g_str_buf[0] = static_cast<wchar_t>(len);
    for (size_t i = 0; i < len; ++i) g_str_buf[i + 1] = text[i];
    g_str_buf[len + 1] = L'\0';

    XLOPER12 x{};
    x.xltype  = xltypeStr;
    x.val.str = g_str_buf;
    return x;
}

XLOPER12 make_error(int err_code)
{
    XLOPER12 x{};
    x.xltype  = xltypeErr;
    x.val.err = err_code;
    return x;
}

// ============================================================================
// Print helpers
// ============================================================================

const char* xltype_name(decltype(XLOPER12::xltype) t)
{
    constexpr auto mask = static_cast<decltype(XLOPER12::xltype)>(~(xlbitDLLFree | xlbitXLFree));
    t &= mask;
    switch (t) {
        case xltypeNum:     return "xltypeNum";
        case xltypeStr:     return "xltypeStr";
        case xltypeBool:    return "xltypeBool";
        case xltypeErr:     return "xltypeErr";
        case xltypeMulti:   return "xltypeMulti";
        case xltypeMissing: return "xltypeMissing";
        case xltypeNil:     return "xltypeNil";
        case xltypeInt:     return "xltypeInt";
        default:            return "unknown";
    }
}

void print_result(const char* label, const XLOPER12* result)
{
    if (!result) {
        std::cout << "  " << label << ": [null pointer]\n";
        return;
    }
    constexpr auto mask = static_cast<decltype(XLOPER12::xltype)>(~(xlbitDLLFree | xlbitXLFree));
    auto base_type = result->xltype & mask;
    switch (base_type) {
        case xltypeNum:
            std::cout << std::format("  {}: {} (xltypeNum)\n", label, result->val.num);
            break;
        case xltypeBool:
            std::cout << std::format("  {}: {} (xltypeBool)\n", label, result->val.xbool ? "TRUE" : "FALSE");
            break;
        case xltypeStr:
            std::cout << std::format("  {}: [string of {} chars] (xltypeStr)\n",
                label, static_cast<int>(result->val.str[0]));
            break;
        case xltypeErr:
            std::cout << std::format("  {}: #ERROR({}) (xltypeErr)\n", label, result->val.err);
            break;
        default:
            std::cout << std::format("  {}: [type={}]\n", label, xltype_name(base_type));
            break;
    }
}

// ============================================================================
// Test runner
// ============================================================================

void run_test(const char* description,
              auto callable)
{
    std::cout << "  " << description << "\n";
    try {
        callable();
    }
    catch (const std::exception& ex) {
        std::cout << "    CAUGHT EXCEPTION: " << ex.what() << "\n";
    }
    catch (...) {
        std::cout << "    CAUGHT UNKNOWN EXCEPTION\n";
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[])
{
    std::cout << "=== Mock-Excel: Scalar Type Validation Demo ===\n\n";

    // -----------------------------------------------------------------
    // Determine XLL path
    // -----------------------------------------------------------------
    std::string xll_path = "scalar_validation.xll";
    if (argc > 1) {
        xll_path = argv[1];
    }

    std::cout << "Loading XLL: " << xll_path << "\n";

    LibHandle lib = load_library(xll_path.c_str());


    if (!lib) {
        auto resolved = std::filesystem::absolute(xll_path).string();
        std::cerr << "FATAL: Cannot load " << resolved << ": " << last_error() << "\n";
        return 1;
    }
    std::cout << "Loaded: " << std::filesystem::absolute(xll_path).string() << "\n\n";

    // -----------------------------------------------------------------
    // Resolve exported functions
    // -----------------------------------------------------------------
    auto addNumbers   = reinterpret_cast<AddNumbersFn>(get_proc(lib, "AddNumbers"));
    auto negateBool   = reinterpret_cast<NegateBoolFn>(get_proc(lib, "NegateBool"));
    auto stringLength = reinterpret_cast<StringLengthFn>(get_proc(lib, "StringLength"));
    auto autoFree     = reinterpret_cast<AutoFreeFn>(get_proc(lib, "xlAutoFree12"));

    if (!addNumbers)   std::cerr << "WARNING: AddNumbers not found\n";
    if (!negateBool)   std::cerr << "WARNING: NegateBool not found\n";
    if (!stringLength) std::cerr << "WARNING: StringLength not found\n";
    if (!autoFree)     std::cerr << "WARNING: xlAutoFree12 not found\n";

    auto safe_free = [&](const XLOPER12* p) {
        if (p && autoFree && (p->xltype & xlbitDLLFree)) autoFree(p);
    };

    // =================================================================
    // TEST GROUP 1: AddNumbers
    // =================================================================
    if (addNumbers) {
        std::cout << "--- AddNumbers ---\n";

        // Test 1a: correct types (Number + Number)
        run_test("1a. Number(3.0) + Number(4.0)  => expected: 7.0", [&] {
            auto a = make_number(3.0);
            auto b = make_number(4.0);
            auto* r = addNumbers(&a, &b);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 1b: wrong type — pass a String where Number is expected
        run_test("1b. String(\"hello\") + Number(4.0)  => expected: EXCEPTION", [&] {
            auto a = make_string(L"hello");
            auto b = make_number(4.0);
            auto* r = addNumbers(&a, &b);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 1c: wrong type — pass a Bool where Number is expected
        run_test("1c. Bool(true) + Number(4.0)  => expected: EXCEPTION", [&] {
            auto a = make_bool(true);
            auto b = make_number(4.0);
            auto* r = addNumbers(&a, &b);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 1d: wrong type — pass an Error where Number is expected
        run_test("1d. Error(#N/A) + Number(4.0)  => expected: EXCEPTION", [&] {
            auto a = make_error(42);  // xlerrNA = 42
            auto b = make_number(4.0);
            auto* r = addNumbers(&a, &b);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 1e: wrong type — pass an Int where Number is expected
        run_test("1e. Int(5) + Number(4.0)  => expected: EXCEPTION", [&] {
            auto a = make_int(5);
            auto b = make_number(4.0);
            auto* r = addNumbers(&a, &b);
            print_result("Result", r);
            safe_free(r);
        });

        std::cout << "\n";
    }

    // =================================================================
    // TEST GROUP 2: NegateBool
    // =================================================================
    if (negateBool) {
        std::cout << "--- NegateBool ---\n";

        // Test 2a: correct type (Bool)
        run_test("2a. Bool(true)  => expected: FALSE", [&] {
            auto v = make_bool(true);
            auto* r = negateBool(&v);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 2b: wrong type — pass a Number where Bool is expected
        run_test("2b. Number(1.0)  => expected: EXCEPTION", [&] {
            auto v = make_number(1.0);
            auto* r = negateBool(&v);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 2c: wrong type — pass a String where Bool is expected
        run_test("2c. String(\"true\")  => expected: EXCEPTION", [&] {
            auto v = make_string(L"true");
            auto* r = negateBool(&v);
            print_result("Result", r);
            safe_free(r);
        });

        std::cout << "\n";
    }

    // =================================================================
    // TEST GROUP 3: StringLength
    // =================================================================
    if (stringLength) {
        std::cout << "--- StringLength ---\n";

        // Test 3a: correct type (String)
        run_test("3a. String(\"Hello\")  => expected: 5.0", [&] {
            auto v = make_string(L"Hello");
            auto* r = stringLength(&v);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 3b: wrong type — pass a Number where String is expected
        run_test("3b. Number(42.0)  => expected: EXCEPTION", [&] {
            auto v = make_number(42.0);
            auto* r = stringLength(&v);
            print_result("Result", r);
            safe_free(r);
        });

        // Test 3c: wrong type — pass a Bool where String is expected
        run_test("3c. Bool(false)  => expected: EXCEPTION", [&] {
            auto v = make_bool(false);
            auto* r = stringLength(&v);
            print_result("Result", r);
            safe_free(r);
        });

        std::cout << "\n";
    }

    // =================================================================
    // Summary
    // =================================================================
    std::cout << "=== Demo complete ===\n";
    std::cout << "The tests above demonstrate that LibXLL's basic/scalar types\n"
              << "throw exceptions when the underlying XLOPER12's xltype does not\n"
              << "match the expected type. This is the correct behaviour for\n"
              << "scalar types used as UDF parameters — the user cannot guarantee\n"
              << "the argument type in Excel, so runtime validation is essential.\n\n"
              << "To handle wrong types gracefully (without exceptions), use the\n"
              << "wrapper types: xll::Optional, xll::Expected, xll::Any, or xll::Variant.\n";

    free_library(lib);
    return 0;
}

