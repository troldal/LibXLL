// xllDemoImModalDialog.cpp
//
// Minimal demo: a single XLL command that shows a modal Dear ImGui dialog.
//
// Mirrors xllDemoWxModalDialog.cpp but uses ImGui + Win32 + DX11 instead
// of wxWidgets.
//
// Unlike xllDemoImDialog.cpp there is no dedicated UI thread, no dispatcher,
// and no cross-thread marshaling:
//
//   - ImGui has no global state to initialize — each dialog instance creates
//     its own ImGuiContext and destroys it on exit.  No onOpen / onClose hooks
//     are needed.
//   - The command creates the dialog, runs a local PeekMessage loop, reads the
//     result — all on Excel's main thread.
//   - NativeOwnerModal handles owner-window wiring, centring, and Excel
//     disable/re-enable for proper modal semantics.

#define WIN32_LEAN_AND_MEAN
#include "ImGuiGreetingDialog.hpp"

#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

// ============================================================================
// Command: IM.MODAL.GREETING
// ============================================================================

auto imModalGreetingCmd =
    xll::Command("IM.MODAL.GREETING")
    | xll::Procedure("ShowImModalGreeting")
    | xll::Category("ImGui Examples")
    | xll::Description(
        "Shows a modal Dear ImGui dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(imModalGreetingCmd);

XLL_FUNCTION void XLLAPI ShowImModalGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    const auto input = ImGuiGreetingDialog::show(excelHwnd);
    if (!input.empty())
        xll::alert(xll::String("Hello, " + input + "!"));
}
