if (NOT Qt6_FOUND)
    return()
endif()

add_executable(qtDemo WIN32 qtDemo.cpp)
target_link_libraries(qtDemo PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_definitions(qtDemo PRIVATE NOMINMAX)

add_library(xllDemoQtDialog SHARED xllDemoQtDialog.cpp)
target_link_libraries(xllDemoQtDialog PUBLIC LibXLL project_warnings project_hardening
    Qt6::Core Qt6::Gui Qt6::Widgets Pal::Sigslot ole32 oleaut32)
set_target_properties(xllDemoQtDialog PROPERTIES OUTPUT_NAME "qt_dialog_demo" SUFFIX ".xll" PREFIX "")
target_compile_definitions(xllDemoQtDialog PRIVATE NOMINMAX)

add_library(xllDemoQtModalDialog SHARED xllDemoQtModalDialog.cpp)
target_link_libraries(xllDemoQtModalDialog PUBLIC LibXLL project_warnings project_hardening
    Qt6::Core Qt6::Gui Qt6::Widgets)
set_target_properties(xllDemoQtModalDialog PROPERTIES OUTPUT_NAME "qt_modal_dialog_demo" SUFFIX ".xll" PREFIX "")
target_compile_definitions(xllDemoQtModalDialog PRIVATE NOMINMAX)

if (TARGET Qt6::Quick AND TARGET Qt6::Qml AND TARGET Qt6::QuickControls2)
    cmrc_add_resource_library(qml-demo-resources
        ALIAS qml_demo::rc
        NAMESPACE qml_demo
        QML/StatusWindow.qml
        QML/GreetingDialog.qml
    )

    add_library(xllDemoQmlDialog SHARED xllDemoQmlDialog.cpp)
    target_link_libraries(xllDemoQmlDialog PUBLIC LibXLL project_warnings project_hardening
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2
        Pal::Sigslot ole32 oleaut32
        qml_demo::rc)
    set_target_properties(xllDemoQmlDialog PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME "qml_dialog_demo" SUFFIX ".xll" PREFIX "")
    target_compile_definitions(xllDemoQmlDialog PRIVATE NOMINMAX)

    add_library(xllDemoQmlModalDialog SHARED xllDemoQmlModalDialog.cpp)
    target_link_libraries(xllDemoQmlModalDialog PUBLIC LibXLL project_warnings project_hardening
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2
        qml_demo::rc)
    set_target_properties(xllDemoQmlModalDialog PROPERTIES
        OUTPUT_NAME "qml_modal_dialog_demo" SUFFIX ".xll" PREFIX "")
    target_compile_definitions(xllDemoQmlModalDialog PRIVATE NOMINMAX)
endif()

# Copy Qt DLLs next to the built targets so they can be loaded at runtime.
qt_generate_deploy_app_script(TARGET qtDemo OUTPUT_SCRIPT QT_DEPLOY_SCRIPT NO_UNSUPPORTED_PLATFORM_ERROR)

