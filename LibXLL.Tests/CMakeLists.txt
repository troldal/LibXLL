include(../cmake/Sanitizers.cmake)
include(../cmake/CompilerWarnings.cmake)
include(../cmake/Hardening.cmake)
create_project_warnings_target(project_warnings)
create_hardening_target(project_hardening)

add_subdirectory(UnitTests)

# AddinTests use MockXL — only built when MockXL is enabled.
if(LIBXLL_ENABLE_MOCKXL)
    add_subdirectory(AddinTests)
endif()
