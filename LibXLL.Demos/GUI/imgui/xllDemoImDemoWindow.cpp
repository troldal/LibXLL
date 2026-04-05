// xllDemoImDemoWindow.cpp
//
// XLL command that opens an 800 x 1200 modal window running the Dear ImGui
// built-in demo (ImGui::ShowDemoWindow).

#define WIN32_LEAN_AND_MEAN
#include "ImGuiGreetingDialog.hpp"
#include "ImGuiDemoDialog.hpp"

#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

thread_local ImGuiContext*   GImGui = NULL;

// ============================================================================
// Command: IM.DEMO
// ============================================================================

auto imDemoCmd =
    xll::Command("IM.DEMO")
    | xll::Procedure("ShowImDemo")
    | xll::Category("ImGui Examples")
    | xll::Description(
        "Opens an 800 x 1200 modal window running the built-in Dear ImGui demo.");
XLL_REGISTER(imDemoCmd);

XLL_FUNCTION void XLLAPI ShowImDemo()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    ImGuiDemoDialog::show(excelHwnd);
}

