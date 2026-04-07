# ---------------------------------------------------------------------------
# xlCOM-Wx.dll — COM add-in demo using wxWidgets as task pane content.
# ---------------------------------------------------------------------------

add_library(xlCOM-Wx SHARED Handlers.cpp)

set_target_properties(xlCOM-Wx PROPERTIES PREFIX "")

target_include_directories(xlCOM-Wx PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # WxTaskPane.hpp
)

target_link_libraries(xlCOM-Wx PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    wx::core        # wxWidgets core
    wx::base        # wxWidgets base
    foo::rc         # Embedded resources (ribbon XML, images)
)

target_compile_definitions(xlCOM-Wx PRIVATE
    # /sdl promotes C4996 to an error after /wd4996 has suppressed it.
    # _CRT_SECURE_NO_WARNINGS removes the deprecation markup at the
    # preprocessor level so /sdl has nothing to promote.
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

# ---------------------------------------------------------------------------
# Per-addin COM identity — GUIDs, ProgIDs, friendly names.
# Configures .reg files and register/unregister targets.
# ---------------------------------------------------------------------------
xlcom_configure_addin(xlCOM-Wx
    ADDIN_GUID              "1E0739E3-A1B5-4AB4-953C-2D8959196A13"
    ADDIN_PROGID            "xlCOM.Wx.Connect"
    ADDIN_FRIENDLY_NAME     "xlCOM wxWidgets"
    ADDIN_DESCRIPTION       "xlCOM Excel COM Add-in (wxWidgets)"
    TASKPANE_GUID           "7C2D4F8D-3E1B-4A5C-9D6E-8F0B2A1C3D5E"
    TASKPANE_PROGID         "xlCOM.Wx.TaskPane"
    TASKPANE_FRIENDLY_NAME  "xlCOM wxWidgets Task Pane Control"
)

