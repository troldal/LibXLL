# ---------------------------------------------------------------------------
# xlCOM-ImGui.dll — COM add-in demo using Dear ImGui (Win32 + DirectX 11)
# as the custom task pane content.
# ---------------------------------------------------------------------------

add_library(xlCOM-ImGui SHARED
    Handlers.cpp
    # ImGui Win32 + DX11 backends — compiled directly into the DLL so it
    # carries its own copy and does not conflict with other imgui consumers.
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
)

set_target_properties(xlCOM-ImGui PROPERTIES PREFIX "")

target_include_directories(xlCOM-ImGui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # ImGuiTaskPane.hpp, SetStyle.hpp, ...
    "${imgui_SOURCE_DIR}/backends"     # imgui_impl_win32.h, imgui_impl_dx11.h
)

target_link_libraries(xlCOM-ImGui PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    imgui           # Dear ImGui core
    d3d11           # Direct3D 11 — swap chain / device / context
    dxgi            # DXGI — adapter / swap chain creation
    d3dcompiler     # D3DCompile — shader compilation (imgui_impl_dx11)
    dwmapi          # DWM — dark-mode support in imgui_impl_win32
    foo::rc         # Embedded resources (ribbon XML, images, font)
)

# ---------------------------------------------------------------------------
# .reg file generation
# ---------------------------------------------------------------------------

set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/xlCOM-ImGui.dll")
cmake_path(NATIVE_PATH _dll_path NORMALIZE _dll_path_native)
string(REPLACE "\\" "\\\\" XLCOM_DLL_PATH_REG "${_dll_path_native}")
unset(_dll_path)
unset(_dll_path_native)

configure_file(
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/Registry/register_dev.reg.in"
    "${CMAKE_CURRENT_BINARY_DIR}/register_dev.reg"
    @ONLY
)
configure_file(
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/Registry/unregister_dev.reg.in"
    "${CMAKE_CURRENT_BINARY_DIR}/unregister_dev.reg"
    @ONLY
)

# ---------------------------------------------------------------------------
# Convenience targets — run as Administrator for HKCR writes.
# ---------------------------------------------------------------------------
add_custom_target(RegisterDll-ImGui
    COMMAND regsvr32 /s "$<TARGET_FILE:xlCOM-ImGui>"
    DEPENDS xlCOM-ImGui
    COMMENT "Registering xlCOM-ImGui.dll (requires elevated privileges for HKCR)"
)

add_custom_target(UnregisterDll-ImGui
    COMMAND regsvr32 /u /s "$<TARGET_FILE:xlCOM-ImGui>"
    DEPENDS xlCOM-ImGui
    COMMENT "Unregistering xlCOM-ImGui.dll"
)

