cmake_minimum_required(VERSION 3.30)
project(LibXLL)

set(CMAKE_CXX_STANDARD 23)

set(CLANG_WARNINGS
        -Wall
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor.
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation

        # Silence unnecessary warnings
        -Wno-switch-bool
)

add_subdirectory(LibXLL.Library)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(LibXLL.Demos)
    add_subdirectory(LibXLL.Tests)
endif()

#target_compile_options(LibXLL PRIVATE "/J" "/utf-8" "/bigobj")


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O1")
# C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-cl.exe


#target_compile_options(LibXLL PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
#target_link_directories(LibXLL PRIVATE "C:\\MinGW_LLVM\\lib\\clang\\20\\lib\\windows")
#target_link_libraries(LibXLL PRIVATE -Wl,--whole-archive clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64 -Wl,--no-whole-archive)

#target_compile_options(LibXLLTest PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
#target_link_directories(LibXLLTest PRIVATE "C:\\MinGW_LLVM\\lib\\clang\\20\\lib\\windows")
#target_link_libraries(LibXLLTest PRIVATE -Wl,--whole-archive clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64 -Wl,--no-whole-archive)


# For GCC/MinGW
#target_link_options(LibXLL PRIVATE
#    -Wl,--whole-archive
#    "clang_rt.asan_dynamic_runtime_thunk-x86_64"
#    "-Wl,--no-whole-archive"
#)

#target_compile_options(LibXLL PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
#target_link_directories(LibXLL PRIVATE "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/lib/clang/19/lib/windows")
#target_link_libraries(LibXLL PRIVATE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
#target_link_options(LibXLL PRIVATE /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)
#
#target_compile_options(LibXLLTest PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
#target_link_directories(LibXLLTest PRIVATE "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/lib/clang/19/lib/windows")
#target_link_libraries(LibXLLTest PRIVATE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
#target_link_options(LibXLLTest PRIVATE /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)