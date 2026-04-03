add_library(xllDemoFltkModalDialog SHARED xllDemoFltkModalDialog.cpp)
target_link_libraries(xllDemoFltkModalDialog PUBLIC LibXLL project_warnings project_hardening fltk::fltk)
set_target_properties(xllDemoFltkModalDialog PROPERTIES OUTPUT_NAME "fltk_modal_dialog_demo" SUFFIX ".xll" PREFIX "")
target_compile_definitions(xllDemoFltkModalDialog PRIVATE NOMINMAX)

add_library(xllDemoFltkDialog SHARED xllDemoFltkDialog.cpp)
target_link_libraries(xllDemoFltkDialog PUBLIC LibXLL project_warnings project_hardening fltk::fltk
    Pal::Sigslot ole32 oleaut32)
set_target_properties(xllDemoFltkDialog PROPERTIES OUTPUT_NAME "fltk_dialog_demo" SUFFIX ".xll" PREFIX "")
target_compile_definitions(xllDemoFltkDialog PRIVATE NOMINMAX)

add_executable(fltkDemo WIN32 fltkDemo.cpp)
target_link_libraries(fltkDemo PRIVATE fltk::fltk)
target_compile_definitions(fltkDemo PRIVATE NOMINMAX)

