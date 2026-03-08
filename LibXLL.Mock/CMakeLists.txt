add_library(MockXL STATIC src/Excel12Impl.cpp)

target_include_directories(MockXL PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(MockXL PUBLIC LibXLL Boost::dll)

target_compile_features(MockXL PUBLIC cxx_std_23)
target_compile_definitions(MockXL PUBLIC NOMINMAX)

if(UNIX)

    # Excel12Impl.cpp defines Excel12/Excel12v which are never directly called
    # by the mock executable — the XLL resolves them at dlopen time.  Without
    # --whole-archive the linker would silently discard the object file and the
    # symbols would be absent from the executable's dynamic symbol table.
    target_link_options(MockXL INTERFACE
        -Wl,--whole-archive
        $<TARGET_FILE:MockXL>
        -Wl,--no-whole-archive
    )
endif()

include(../cmake/CompilerWarnings.cmake)
include(../cmake/Hardening.cmake)
create_project_warnings_target(project_warnings)
create_hardening_target(project_hardening)
target_link_libraries(MockXL PUBLIC project_warnings project_hardening)
