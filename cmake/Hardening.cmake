# cmake/Hardening.cmake
#
# Provides:
#
#   create_hardening_target(<target_name>)
#
# Creates a reusable INTERFACE library that carries compiler and linker flags
# for binary hardening on each supported platform/toolchain combination.
# Calling the function a second time with the same name is a no-op, so it is
# safe to include() from multiple subdirectories.
#
# Link against the resulting target with:
#   target_link_libraries(<your_target> PRIVATE <target_name>)
#
# ── Platform coverage ────────────────────────────────────────────────────────
#
# Windows – MSVC / clang-cl
#   Compiler:  /GS  /sdl  /guard:cf  /Qspectre
#   Linker:    /DYNAMICBASE  /NXCOMPAT  /HIGHENTROPYVA  /GUARD:CF  /CETCOMPAT
#
# Windows – MinGW-w64 (GCC or Clang with GNU driver)
#   Compiler:  -fstack-protector-strong  -fPIE  -fwrapv
#              -fno-strict-overflow  (GCC only; Clang uses -fwrapv for the same effect)
#   Linker:    (relies on toolchain defaults; explicit PE-flag control requires
#              ld/lld-specific options not universally portable here)
#
# Linux – GCC / Clang
#   Compiler:  -fstack-protector-strong  -D_FORTIFY_SOURCE=2  -fPIE
#              -fstack-clash-protection  -fcf-protection=full
#   Linker:    -Wl,-z,relro  -Wl,-z,now  -Wl,-z,noexecstack  -pie
#
# ── Notes ────────────────────────────────────────────────────────────────────
# • /GS and /DYNAMICBASE//NXCOMPAT//HIGHENTROPYVA are already on by default in
#   modern MSVC/link.exe, but are listed explicitly so the intent is auditable.
# • /Qspectre requires the Spectre-mitigated libraries to be installed via the
#   Visual Studio Installer (component "MSVC vXXX … Spectre-mitigated libs").
# • -D_FORTIFY_SOURCE=2 requires at least -O1 to be effective; it is combined
#   with -O2 here for that reason.
# • -fcf-protection=full (IBT + shadow stack) is a Clang/GCC x86_64 feature;
#   it is guarded behind a compiler-support check below.
# • -fstack-clash-protection is supported by GCC and Clang ≥ 11 on x86/x86_64;
#   it is likewise guarded behind a compile-check.

include_guard(GLOBAL)

function(create_hardening_target target_name)
    if(TARGET ${target_name})
        return()
    endif()

    add_library(${target_name} INTERFACE)

    # ── MSVC / clang-cl ──────────────────────────────────────────────────────
    if(MSVC)
        target_compile_options(${target_name} INTERFACE
            /GS            # Stack buffer overrun detection (cookie/canary)
            /sdl           # Additional SDL security checks (superset of /GS)
            /guard:cf      # Control Flow Guard instrumentation
            $<$<CXX_COMPILER_ID:MSVC>:/Qspectre>   # Spectre v1 mitigation (cl.exe only; not supported by clang-cl)
        )
        target_link_options(${target_name} INTERFACE
            /DYNAMICBASE   # Enable ASLR (Address Space Layout Randomisation)
            /NXCOMPAT      # Mark image as compatible with DEP / NX
            /HIGHENTROPYVA # 64-bit high-entropy ASLR
            /GUARD:CF      # Embed CFG metadata in the PE image
            /CETCOMPAT     # Intel CET shadow-stack compatibility
        )

    # ── Linux / macOS (GCC or Clang with GNU driver) ─────────────────────────
    elseif(NOT WIN32)
        # Base flags supported by all GCC/Clang versions we care about
        set(_compile_flags
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
            -O2             # Required for _FORTIFY_SOURCE to be effective
            -fPIE
            -fwrapv
        )

        # -fno-strict-overflow is GCC-only; Clang does not support it.
        # (-fwrapv above already covers the same ground on both compilers.)
        # check_cxx_compiler_flag is unreliable here because Clang accepts
        # the flag without error but warns at compile time (-Wunused-command-line-argument).
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            list(APPEND _compile_flags -fno-strict-overflow)
        endif()
        set(_link_flags
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,-z,noexecstack
            -pie
        )

        # -fstack-clash-protection (GCC + Clang ≥ 11, x86/x86_64/aarch64)
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag(-fstack-clash-protection _HAS_STACK_CLASH)
        if(_HAS_STACK_CLASH)
            list(APPEND _compile_flags -fstack-clash-protection)
        endif()

        # -fcf-protection=full  (x86_64 IBT + shadow stack; GCC ≥ 8, Clang ≥ 7)
        check_cxx_compiler_flag(-fcf-protection=full _HAS_CF_PROTECTION)
        if(_HAS_CF_PROTECTION)
            list(APPEND _compile_flags -fcf-protection=full)
        endif()

        target_compile_options(${target_name} INTERFACE ${_compile_flags})
        target_link_options(${target_name} INTERFACE ${_link_flags})

    # ── Windows MinGW-w64 (GCC or Clang with GNU driver on Windows) ──────────
    else()
        set(_mingw_compile_flags
            -fstack-protector-strong
            -fPIE
            -fwrapv
        )
        # -fno-strict-overflow is GCC-only; Clang silently ignores it but warns
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            list(APPEND _mingw_compile_flags -fno-strict-overflow)
        endif()
        target_compile_options(${target_name} INTERFACE ${_mingw_compile_flags})
        # Note: PE-level flags (DYNAMICBASE, NXCOMPAT, HIGHENTROPYVA) depend on
        # how the MinGW-w64 ld/lld was configured. Modern MinGW-w64 toolchains
        # typically enable DYNAMICBASE and NXCOMPAT by default; add explicit
        # -Wl,--dynamicbase / -Wl,--nxcompat / -Wl,--high-entropy-va here if
        # your toolchain does not.
    endif()
endfunction()

