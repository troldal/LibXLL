# ---------------------------------------------------------------------------
# xlCOM-Fltk.dll — COM add-in demo using FLTK as task pane content.
# ---------------------------------------------------------------------------

add_library(xlCOM-Fltk SHARED Handlers.cpp)

set_target_properties(xlCOM-Fltk PROPERTIES PREFIX "")

target_include_directories(xlCOM-Fltk PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # FltkTaskPane.hpp
)

target_link_libraries(xlCOM-Fltk PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    fltk::fltk      # FLTK toolkit
    foo::rc         # Embedded resources (ribbon XML, images)
)

# ---------------------------------------------------------------------------
# Per-addin COM identity — GUIDs, ProgIDs, friendly names.
# Configures .reg files and register/unregister targets.
# ---------------------------------------------------------------------------
xlcom_configure_addin(xlCOM-Fltk
    ADDIN_GUID              "1E0739E1-A1B5-4AB4-953C-2D8959196A13"
    ADDIN_PROGID            "xlCOM.Fltk.Connect"
    ADDIN_FRIENDLY_NAME     "xlCOM FLTK"
    ADDIN_DESCRIPTION       "xlCOM Excel COM Add-in (FLTK)"
    TASKPANE_GUID           "7C2D4F8B-3E1B-4A5C-9D6E-8F0B2A1C3D5E"
    TASKPANE_PROGID         "xlCOM.Fltk.TaskPane"
    TASKPANE_FRIENDLY_NAME  "xlCOM FLTK Task Pane Control"
)

