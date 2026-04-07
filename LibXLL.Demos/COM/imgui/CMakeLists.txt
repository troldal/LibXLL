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
# Per-addin COM identity — GUIDs, ProgIDs, friendly names.
# Configures .reg files and register/unregister targets.
# ---------------------------------------------------------------------------
xlcom_configure_addin(xlCOM-ImGui
    ADDIN_GUID              "1E0739E0-A1B5-4AB4-953C-2D8959196A13"
    ADDIN_PROGID            "xlCOM.ImGui.Connect"
    ADDIN_FRIENDLY_NAME     "xlCOM ImGui"
    ADDIN_DESCRIPTION       "xlCOM Excel COM Add-in (Dear ImGui)"
    TASKPANE_GUID           "7C2D4F8A-3E1B-4A5C-9D6E-8F0B2A1C3D5E"
    TASKPANE_PROGID         "xlCOM.ImGui.TaskPane"
    TASKPANE_FRIENDLY_NAME  "xlCOM ImGui Task Pane Control"
)

