add_library(xllDemoWxDialog SHARED xllDemoWxDialog.cpp)
target_link_libraries(xllDemoWxDialog PUBLIC LibXLL project_warnings project_hardening wx::core wx::base
    Pal::Sigslot ole32 oleaut32)
set_target_properties(xllDemoWxDialog PROPERTIES OUTPUT_NAME "wx_dialog_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoWxDialog PRIVATE NOMINMAX
    # /sdl (from project_hardening) promotes C4996 "deprecated function" to
    # an error AFTER /wd4996 has suppressed it, because /sdl is processed
    # later on the command line and overrides /wd flags.  The only fix that
    # wins against /sdl is _CRT_SECURE_NO_WARNINGS, which tells the CRT
    # headers to omit the deprecation markup entirely at the preprocessor
    # level - the warning is never emitted so /sdl has nothing to promote.
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)
# No post-build DLL copy needed: wxWidgets is statically linked
# (BUILD_SHARED_LIBS OFF), so TARGET_RUNTIME_DLLS is empty.

add_library(xllDemoWxModalDialog SHARED xllDemoWxModalDialog.cpp)
target_link_libraries(xllDemoWxModalDialog PUBLIC LibXLL project_warnings project_hardening wx::core wx::base)
set_target_properties(xllDemoWxModalDialog PROPERTIES OUTPUT_NAME "wx_modal_dialog_demo" SUFFIX ".xll" PREFIX "")
target_compile_definitions(xllDemoWxModalDialog PRIVATE NOMINMAX
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

add_executable(wxDemo WIN32 wxDemo.cpp wxDemo.rc)
target_link_libraries(wxDemo PRIVATE wx::core wx::base)
target_compile_definitions(wxDemo PRIVATE NOMINMAX)
# MSVC (and clang-cl) auto-generate a manifest and embed it at link time.
# We embed our own via wxDemo.rc, so suppress the auto-generated one to
# avoid a duplicate RT_MANIFEST resource (CVT1100 / LNK1123).
if (MSVC)
    target_link_options(wxDemo PRIVATE /MANIFEST:NO)
endif()
# No post-build DLL copy needed: wxWidgets is statically linked.

