# ---------------------------------------------------------------------------
# Require WIN32.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    return()
endif()

# ---------------------------------------------------------------------------
# Shared embedded resources — ribbon XML, button images, font.
# Defined here so all framework subdirectories can link against foo::rc.
# ---------------------------------------------------------------------------
cmrc_add_resource_library(foo-resources ALIAS foo::rc NAMESPACE foo
    Resources/XML/ribbon.xml
    Resources/Images/button-help_dark48px.png
    Resources/Images/button-help_light48px.png
    Resources/Fonts/Inter-VariableFont.ttf
)

# ---------------------------------------------------------------------------
# Per-framework COM add-in demos — one DLL per GUI framework.
# ---------------------------------------------------------------------------
add_subdirectory(imgui)
add_subdirectory(fltk)
add_subdirectory(wx)
add_subdirectory(Qt)
add_subdirectory(win32)
