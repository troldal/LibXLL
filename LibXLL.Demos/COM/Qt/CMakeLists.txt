# ---------------------------------------------------------------------------
# xlCOM-Qt.dll — COM add-in demo using Qt Widgets as task pane content.
# Qt Quick (QML) alternative: swap QtTaskPane for QtQuickTaskPane in
# Handlers.cpp and link Qt6::Quick + Qt6::Qml instead.
# ---------------------------------------------------------------------------

if(NOT Qt6_FOUND)
    return()
endif()

add_library(xlCOM-Qt SHARED Handlers.cpp)

set_target_properties(xlCOM-Qt PROPERTIES
    PREFIX ""
    AUTOMOC ON      # Required for Q_OBJECT classes inside Qt task pane headers
)

target_include_directories(xlCOM-Qt PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # QtTaskPane.hpp, QtQuickTaskPane.hpp
)

target_link_libraries(xlCOM-Qt PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets    # Qt Widgets — used by QtTaskPane
    # Qt6::Quick Qt6::Qml   # Enable for QtQuickTaskPane
    foo::rc         # Embedded resources (ribbon XML, images)
)

# ---------------------------------------------------------------------------
# Per-addin COM identity — GUIDs, ProgIDs, friendly names.
# Configures .reg files and register/unregister targets.
# ---------------------------------------------------------------------------
xlcom_configure_addin(xlCOM-Qt
    ADDIN_GUID              "1E0739E4-A1B5-4AB4-953C-2D8959196A13"
    ADDIN_PROGID            "xlCOM.Qt.Connect"
    ADDIN_FRIENDLY_NAME     "xlCOM Qt"
    ADDIN_DESCRIPTION       "xlCOM Excel COM Add-in (Qt Widgets)"
    TASKPANE_GUID           "7C2D4F8E-3E1B-4A5C-9D6E-8F0B2A1C3D5E"
    TASKPANE_PROGID         "xlCOM.Qt.TaskPane"
    TASKPANE_FRIENDLY_NAME  "xlCOM Qt Task Pane Control"
)

