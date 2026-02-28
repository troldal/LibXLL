# cmake/Sanitizers.cmake
#
# Provides:
#
#   target_enable_sanitizer(<target> <sanitizer> [RUNTIME_LIB_DIR <dir>])
#   target_enable_drmemory(<target>)
#
# Applies the compiler/linker flags required to instrument <target> with the
# requested <sanitizer>.  Supported sanitizer names (case-sensitive):
#
#   address    — AddressSanitizer (ASan)   — Linux + Windows clang-cl
#   undefined  — UndefinedBehaviorSanitizer (UBSan) — Linux only
#   thread     — ThreadSanitizer (TSan)    — Linux only
#   memory     — MemorySanitizer (MSan)    — Linux only
#
# On Linux (any Clang or GCC build) the GCC/Clang driver flags are used.
# On Windows with clang-cl the MSVC-compatible flags are used instead:
#   • MSVC_RUNTIME_LIBRARY is forced to MultiThreadedDLL (/MD)
#   • /RTC1 is stripped (incompatible with ASan)
#   • The dynamic ASan runtime is linked and its DLL is copied post-build
#
# RUNTIME_LIB_DIR (optional, Windows only):
#   Override the directory that contains clang_rt.asan_dynamic-x86_64.lib.
#   If omitted, the path is derived automatically from CMAKE_CXX_COMPILER:
#       <compiler-bin>/../lib/clang/XX/lib/windows
#
# Notes:
#   • LeakSanitizer (LSan) is integrated into ASan on Linux and runs
#     automatically.  It is NOT available on Windows — leaks are silently
#     ignored there.
#   • TSan, MSan, and UBSan are not supported by clang-cl on Windows.
#   • The caller is responsible for building any static libraries that are
#     linked into <target> with the same sanitizer flags when required (e.g.
#     Catch2 must be rebuilt with ASan flags so that all TUs agree on the
#     annotate_string ABI flag).
#
# target_enable_drmemory(<target>)
#   Adds the compiler/linker flags needed for useful Dr. Memory stack traces
#   on Windows with clang-cl or MSVC:
#     /Zi   — full debug information (PDB)
#     /Od   — disable optimisations so inlined frames are not lost
#     /Oy-  — keep frame pointers for reliable stack walks
#     /DEBUG (link) — instruct the linker to write the PDB
#   Dr. Memory itself is launched from the CLion Run/Debug Configuration;
#   this function only ensures the binary carries enough debug information
#   for Dr. Memory to resolve addresses back to source file and line numbers.
#   Has no effect on non-Windows or non-clang-cl/MSVC toolchains.

include_guard(GLOBAL)

# ---------------------------------------------------------------------------
# Internal helper: derive the clang-cl ASan runtime lib directory
# ---------------------------------------------------------------------------
function(_asan_clangcl_lib_dir out_var)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH _bin)
    # Detect the actual Clang version from the compiler path so the helper
    # works for both clang-20 and future releases.
    if (CMAKE_CXX_COMPILER_VERSION MATCHES "^([0-9]+)")
        set(_major "${CMAKE_MATCH_1}")
    else ()
        set(_major "20")   # safe fallback
    endif ()
    set(_dir "${_bin}/../lib/clang/${_major}/lib/windows")
    cmake_path(NORMAL_PATH _dir)
    set(${out_var} "${_dir}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Public function
# ---------------------------------------------------------------------------
function(target_enable_sanitizer target sanitizer)
    cmake_parse_arguments(ARG "" "RUNTIME_LIB_DIR" "" ${ARGN})

    # ── Linux / macOS ────────────────────────────────────────────────────────
    if (NOT WIN32)
        if (sanitizer STREQUAL "address")
            set(_flags -fsanitize=address -fno-omit-frame-pointer -g)
        elseif (sanitizer STREQUAL "undefined")
            set(_flags -fsanitize=undefined -fno-omit-frame-pointer -g)
        elseif (sanitizer STREQUAL "thread")
            set(_flags -fsanitize=thread -fPIE -pie -g)
        elseif (sanitizer STREQUAL "memory")
            set(_flags -fsanitize=memory -fPIE -fno-omit-frame-pointer -g
                    -fno-optimize-sibling-calls -fsanitize-recover=all -O1)
        else ()
            message(WARNING "target_enable_sanitizer: unknown sanitizer '${sanitizer}' — ignored")
            return()
        endif ()

        target_compile_options(${target} PRIVATE ${_flags})
        target_link_options(${target} PRIVATE ${_flags})

    # ── Windows clang-cl ─────────────────────────────────────────────────────
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
            CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")

        if (NOT sanitizer STREQUAL "address")
            message(WARNING
                    "target_enable_sanitizer: '${sanitizer}' is not supported "
                    "by clang-cl on Windows — ignored")
            return()
        endif ()

        # Resolve the runtime library directory
        if (ARG_RUNTIME_LIB_DIR)
            set(_lib_dir "${ARG_RUNTIME_LIB_DIR}")
        else ()
            _asan_clangcl_lib_dir(_lib_dir)
        endif ()

        # Force release CRT — /MDd is incompatible with ASan
        set_target_properties(${target} PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
        )

        # Strip /RTC1 injected by CMake for Debug configs; instrument the code
        target_compile_options(${target} PRIVATE
                $<$<CONFIG:Debug>:/RTC1->
                -fsanitize=address /Oy- -g
        )

        # Link the dynamic ASan runtime
        target_link_directories(${target} PRIVATE "${_lib_dir}")
        target_link_libraries(${target} PRIVATE clang_rt.asan_dynamic-x86_64.lib)
        target_link_options(${target} PRIVATE
                /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)

        # Copy the runtime DLL next to the executable so it can be found without
        # modifying PATH
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_lib_dir}/clang_rt.asan_dynamic-x86_64.dll"
                        "$<TARGET_FILE_DIR:${target}>"
                COMMENT "Copying clang_rt.asan_dynamic-x86_64.dll to $<TARGET_FILE_DIR:${target}>"
        )

    else ()
        message(STATUS
                "target_enable_sanitizer: sanitizers not configured for this "
                "compiler/platform combination — '${sanitizer}' ignored for ${target}")
    endif ()
endfunction()
