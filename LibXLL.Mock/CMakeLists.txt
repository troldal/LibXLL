add_library(MockXL INTERFACE)

target_include_directories(MockXL INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(MockXL INTERFACE LibXLL Boost::dll)

target_compile_features(MockXL INTERFACE cxx_std_23)
target_compile_definitions(MockXL INTERFACE NOMINMAX)

include(../cmake/CompilerWarnings.cmake)
include(../cmake/Hardening.cmake)
create_project_warnings_target(project_warnings)
create_hardening_target(project_hardening)
target_link_libraries(MockXL INTERFACE project_warnings project_hardening)
