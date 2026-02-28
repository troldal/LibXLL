// DemoAsanLeak.cpp
//
// A minimal program that exercises two kinds of memory errors to verify that
// Address Sanitizer (ASan) is correctly configured and working.
//
// ─── LEAK DETECTION ──────────────────────────────────────────────────────────
// On Linux, LeakSanitizer (LSan) is integrated into ASan and runs
// automatically at process exit.  No extra steps are needed; you will see:
//
//   ==PID==ERROR: LeakSanitizer: detected memory leaks
//   Direct leak of 40 byte(s) in 1 object(s) …
//
// On Windows, clang-cl's ASan does NOT include LSan — it is not supported
// on Windows.  Leak detection is therefore NOT available via ASan on Windows.
// The leak in this program will be silently ignored on Windows even when the
// .address target is run.
//
// ─── USE-AFTER-FREE DETECTION ────────────────────────────────────────────────
// Use-after-free IS detected by ASan on both Linux and Windows.  This program
// deliberately triggers one so that you can confirm ASan is operational on
// Windows.  When the .address target is run you should see:
//
//   ==PID==ERROR: AddressSanitizer: heap-use-after-free on address …
//
// ─── BUILD TARGETS ───────────────────────────────────────────────────────────
//   Plain (no sanitizer) : DemoAsanLeak          — exits without errors
//   Sanitized            : DemoAsanLeak.address   — triggers UAF report
//     Linux   : if (NOT WIN32) block in LibXLL.Demos/CMakeLists.txt
//     Windows : elseif (clang-cl) block in LibXLL.Demos/CMakeLists.txt

#include <iostream>

int main()
{
    std::cout << "DemoAsanLeak: starting\n";

    // ── deliberate heap leak (detected by LSan on Linux only) ────────────────
    auto* leaked = new int[10];
    for (int i = 0; i < 10; ++i)
        leaked[i] = i * i;
    std::cout << "DemoAsanLeak: leaked[3] = " << leaked[3]
              << "  (leak — reported on Linux, silent on Windows)\n";
    // Intentionally NOT calling  delete[] leaked;

    // ── deliberate use-after-free (detected by ASan on both platforms) ───────
    auto* buf = new int[4];
    for (int i = 0; i < 4; ++i)
        buf[i] = i;
    delete[] buf;

    // Reading from freed memory: ASan will halt here on both Linux and Windows.
    std::cout << "DemoAsanLeak: about to read freed memory (use-after-free)...\n";
    volatile int uaf_val = buf[0];          // undefined behaviour — ASan catches this
    std::cout << "DemoAsanLeak: buf[0] = " << uaf_val
              << "  (if you see this, ASan is NOT active)\n";

    std::cout << "DemoAsanLeak: done\n";
    return 0;
}
