# ---------------------------------------------------------------------------
# xlCOM-Win32.dll — COM add-in demo using pure Win32 as task pane content.
# No additional GUI framework required beyond the Windows SDK.
# ---------------------------------------------------------------------------

add_library(xlCOM-Win32 SHARED Handlers.cpp)

set_target_properties(xlCOM-Win32 PROPERTIES PREFIX "")

target_include_directories(xlCOM-Win32 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # Win32TaskPane.hpp
)

target_link_libraries(xlCOM-Win32 PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    foo::rc         # Embedded resources (ribbon XML, images)
)

# ---------------------------------------------------------------------------
# Per-addin COM identity — GUIDs, ProgIDs, friendly names.
# Configures .reg files and register/unregister targets.
# ---------------------------------------------------------------------------
xlcom_configure_addin(xlCOM-Win32
    ADDIN_GUID              "1E0739E2-A1B5-4AB4-953C-2D8959196A13"
    ADDIN_PROGID            "xlCOM.Win32.Connect"
    ADDIN_FRIENDLY_NAME     "xlCOM Win32"
    ADDIN_DESCRIPTION       "xlCOM Excel COM Add-in (Win32)"
    TASKPANE_GUID           "7C2D4F8C-3E1B-4A5C-9D6E-8F0B2A1C3D5E"
    TASKPANE_PROGID         "xlCOM.Win32.TaskPane"
    TASKPANE_FRIENDLY_NAME  "xlCOM Win32 Task Pane Control"
)

