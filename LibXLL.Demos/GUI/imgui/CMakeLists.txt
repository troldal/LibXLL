add_library(xllDemoImDialog SHARED
    xllDemoImDialog.cpp
    # Compile the ImGui Win32 + DX11 backends directly into the XLL so that
    # the add-in carries its own copy and does not conflict with xlCOM.dll.
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
)

target_include_directories(xllDemoImDialog PRIVATE
    "${imgui_SOURCE_DIR}/backends"          # imgui_impl_win32.h, imgui_impl_dx11.h
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/ActiveX" # SetStyle.hpp
)

target_link_libraries(xllDemoImDialog PUBLIC
    LibXLL
    project_warnings
    project_hardening
    Pal::Sigslot # palacaze/sigslot — cross-thread signal/slot library
    imgui        # Dear ImGui core
    d3d11        # Direct3D 11 device / context
    dxgi         # DXGI swap chain / adapter
    d3dcompiler  # D3DCompile (required by imgui_impl_dx11)
    dwmapi       # DWM dark-mode title bar (DwmSetWindowAttribute)
    advapi32     # Registry API (RegGetValueW) for dark-mode detection
    ole32        # CoTaskMemAlloc, CoCreateInstance, ...
    oleaut32     # SafeArrayXxx, VariantXxx, ...
)

set_target_properties(xllDemoImDialog PROPERTIES
    OUTPUT_NAME "im_dialog_demo"
    SUFFIX      ".xll"
)

target_compile_definitions(xllDemoImDialog PRIVATE
    NOMINMAX
    # /sdl promotes C4996 to an error after /wd4996 has suppressed it.
    # _CRT_SECURE_NO_WARNINGS removes the deprecation markup at the
    # preprocessor level so /sdl has nothing to promote.
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

# ----------------------------------------------------------------------------
# xllDemoImModalDialog — minimal modal-only demo (mirrors xllDemoWxModalDialog)
# ----------------------------------------------------------------------------

add_library(xllDemoImModalDialog SHARED
    xllDemoImModalDialog.cpp
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
)

target_include_directories(xllDemoImModalDialog PRIVATE
    "${imgui_SOURCE_DIR}/backends"
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/ActiveX"
)

target_link_libraries(xllDemoImModalDialog PUBLIC
    LibXLL
    project_warnings
    project_hardening
    imgui
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    advapi32
)

set_target_properties(xllDemoImModalDialog PROPERTIES
    OUTPUT_NAME "im_modal_dialog_demo"
    SUFFIX      ".xll"
)

target_compile_definitions(xllDemoImModalDialog PRIVATE
    NOMINMAX
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

# ----------------------------------------------------------------------------
# xllDemoImDemoWindow — modal 800x1200 window running the Dear ImGui built-in demo
# ----------------------------------------------------------------------------

add_library(xllDemoImDemoWindow SHARED
    xllDemoImDemoWindow.cpp
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
)

target_include_directories(xllDemoImDemoWindow PRIVATE
    "${imgui_SOURCE_DIR}/backends"
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/ActiveX"
)

target_link_libraries(xllDemoImDemoWindow PUBLIC
    LibXLL
    project_warnings
    project_hardening
    imgui
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    advapi32
)

set_target_properties(xllDemoImDemoWindow PROPERTIES
    OUTPUT_NAME "im_demo_window"
    SUFFIX      ".xll"
)

target_compile_definitions(xllDemoImDemoWindow PRIVATE
    NOMINMAX
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)
