// xllDemoWxDialog.cpp
//
// Demonstrates hosting a wxWidgets modal dialog inside an XLL command.
//
// How it works
// ------------
// wxWidgets is initialised once when the XLL loads (xlAutoOpen → wxInitialize)
// and cleaned up when it unloads (xlAutoClose → wxUninitialize).
// wxInitialize/wxUninitialize are reference-counted, so multiple XLLs can
// safely call them independently.
//
// To make the dialog properly modal over Excel the dialog is created with a
// nullptr wx parent and modal behaviour is implemented manually via Win32,
// encapsulated in NativeOwnerModal (see below).
//
// Wrapping Excel's HWND in a wxWindow (the ExcelParentGuard approach) does
// NOT work: wxWidgets' internal button/dialog code traverses the parent chain,
// encounters the fake wrapper, and ultimately destroys wxDummyConsoleApp
// while ShowModal's event loop is still running, causing a segfault.
//
// Non-modal windows
// -----------------
// Because an XLL is a DLL loaded into Excel's process on Excel's main thread,
// Excel's own GetMessage/DispatchMessage loop automatically dispatches messages
// for any wx window we Show() — no separate event loop is needed.
// Non-modal windows must be heap-allocated (new) so they outlive the command
// function; wxWidgets deletes them when closed (via Destroy()).
// Use NativeOwnerSetup (not NativeOwnerModal) — no EnableWindow blocking.
//
// Include order note
// ------------------
// wx headers MUST come before any LibXLL / ExcelSDK headers.
// wx/msw/wrapwin.h (pulled in early by wx/wx.h) defines WIN32_LEAN_AND_MEAN
// and then includes <windows.h>. WIN32_LEAN_AND_MEAN prevents windows.h from
// pulling in the old winsock.h; wxWidgets then includes winsock2.h safely.
// If LibXLL headers (which also include windows.h) came first they would
// include windows.h *without* WIN32_LEAN_AND_MEAN, dragging in winsock.h,
// and the subsequent winsock2.h inclusion would produce hundreds of
// redefinition errors.

#include <wx/wx.h>

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>        // pulls in xll::get_hwnd() via GetHwnd.hpp

// ============================================================================
// NativeOwnerSetup
//
// Low-level Win32 helper: wires a window to a foreign owner and centres it.
// Framework-agnostic — pass any HWND regardless of GUI toolkit:
//
//   wxWidgets : dlg.GetHWND()   (WXHWND, implicitly convertible to HWND)
//   Qt        : reinterpret_cast<HWND>(dlg.winId())
//
// This is the common foundation for both modal and non-modal windows.
// Use it directly for non-modal windows; use NativeOwnerModal (below) for
// modal dialogs.
//
// If this pattern is needed more widely, consider moving both classes to a
// shared utility header in LibXLL.
// ============================================================================

class NativeOwnerSetup
{
public:
    NativeOwnerSetup(HWND windowHwnd, HWND ownerHwnd)
    {
        SetWindowLongPtr(windowHwnd, GWLP_HWNDPARENT,
                           reinterpret_cast<LONG_PTR>(ownerHwnd));

        RECT ow{}, wd{};
        GetWindowRect(ownerHwnd,  &ow);
        GetWindowRect(windowHwnd, &wd);
        const int dw = wd.right  - wd.left;
        const int dh = wd.bottom - wd.top;
        SetWindowPos(windowHwnd, nullptr,
                       ow.left + (ow.right  - ow.left - dw) / 2,
                       ow.top  + (ow.bottom - ow.top  - dh) / 2,
                       0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    // No destructor logic — the window is unaffected when this goes out of scope.
};

// ============================================================================
// NativeOwnerModal
//
// Extends NativeOwnerSetup with RAII modal semantics: disables the owner
// window for exclusive input while the dialog is open, then restores it.
// ============================================================================

class NativeOwnerModal
{
public:
    NativeOwnerModal(HWND dialogHwnd, HWND ownerHwnd)
        : m_setup(dialogHwnd, ownerHwnd), m_owner(ownerHwnd)
    {
        EnableWindow(ownerHwnd, FALSE);
    }

    ~NativeOwnerModal()
    {
        EnableWindow(m_owner, TRUE);
        SetForegroundWindow(m_owner);
    }

    NativeOwnerModal(const NativeOwnerModal&)            = delete;
    NativeOwnerModal& operator=(const NativeOwnerModal&) = delete;

private:
    NativeOwnerSetup m_setup;   // owner relationship + centering
    HWND             m_owner;
};

// ============================================================================
// DLL-level wxWidgets initialisation state
// ============================================================================

static bool s_wxInitialized = false;

// A minimal wxApp subclass for hosting wx GUI elements from a DLL.
//
// wxInitialize() creates wxDummyConsoleApp, which only inherits from
// wxAppConsole — NOT from wxApp.  In a GUI build, wxTheApp is typed as
// wxApp*, so any virtual call through it (e.g. wxApp::MSWGetDefaultLayout
// in toplevel.cpp calling wxTheApp->GetLayoutDirection()) is a call through
// an invalid pointer → segfault.
//
// The fix: wxInitialize() only creates wxDummyConsoleApp when
// wxApp::GetInstance() returns nullptr.  Setting our own wxApp-derived
// instance first causes wxInitialize() to skip that step and use ours.
class XllApp : public wxApp
{
public:
    bool OnInit() override { return true; }
};

// ============================================================================
// Add-in lifecycle
// ============================================================================

xll::AddInManagerInfo dllName([] { return xll::String("wxDialog Demo"); });

auto onOpen =
    xll::OnOpen()
    | xll::Before([] {
        if (!s_wxInitialized) {
            if (!wxApp::GetInstance())
                wxApp::SetInstance(new XllApp());
            if (wxInitialize())
                s_wxInitialized = true;
        }
    });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] {
        if (s_wxInitialized) {
            wxUninitialize();
            s_wxInitialized = false;
        }
    });
XLL_REGISTER(onClose);

// ============================================================================
// Modal dialog (WX.GREETING)
// ============================================================================

class GreetingDialog : public wxDialog
{
public:
    explicit GreetingDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "wxWidgets inside an XLL",
                   wxDefaultPosition, wxSize(360, 150))
    {
        auto* panel  = new wxPanel(this);
        auto* vbox   = new wxBoxSizer(wxVERTICAL);
        auto* hbox   = new wxBoxSizer(wxHORIZONTAL);
        auto* btns   = new wxBoxSizer(wxHORIZONTAL);

        auto* label  = new wxStaticText(panel, wxID_ANY, "Enter your name:");
        m_input      = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxSize(200, -1),
                                      wxTE_PROCESS_ENTER);

        hbox->Add(label,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        hbox->Add(m_input, 1, wxEXPAND);

        auto* btnOk     = new wxButton(panel, wxID_OK,     "OK");
        auto* btnCancel = new wxButton(panel, wxID_CANCEL, "Cancel");
        btns->Add(btnOk,     0, wxRIGHT, 6);
        btns->Add(btnCancel, 0);

        vbox->AddStretchSpacer();
        vbox->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
        vbox->AddSpacer(10);
        vbox->Add(btns, 0, wxALIGN_CENTER | wxBOTTOM, 12);
        vbox->AddStretchSpacer();

        panel->SetSizer(vbox);
        Centre();

        // Pressing Enter in the text box confirms the dialog.
        m_input->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { EndModal(wxID_OK); });
    }

    // Returns the text the user typed, encoded as UTF-8 std::string.
    [[nodiscard]] std::string GetInput() const
    {
        return m_input->GetValue().utf8_string();
    }

private:
    wxTextCtrl* m_input = nullptr;
};

// ============================================================================
// Command: WX.GREETING
// ============================================================================

auto wxGreetingCmd =
    xll::Command("WX.GREETING")
    | xll::Procedure("ShowWxGreeting")
    | xll::Category("wxWidgets Examples")
    | xll::Description(
        "Shows a modal wxWidgets dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(wxGreetingCmd);

XLL_FUNCTION void XLLAPI ShowWxGreeting()
{
    if (!s_wxInitialized) return;

    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    GreetingDialog dlg(nullptr);
    NativeOwnerModal modal(static_cast<HWND>(dlg.GetHWND()), excelHwnd);

    if (dlg.ShowModal() == wxID_OK)
        xll::alert(xll::String("Hello, " + dlg.GetInput() + "!"));
}

// ============================================================================
// Non-modal frame (WX.STATUS)
//
// Demonstrates the non-modal case:
//   • NativeOwnerSetup (not NativeOwnerModal) — no EnableWindow blocking,
//     Excel remains fully interactive.
//   • Heap-allocated with new; wxWidgets calls Destroy() when the user
//     closes it, so no manual lifetime management is needed.
//   • The command function returns immediately after Show(); Excel's own
//     message loop dispatches events to the frame while it is open.
// ============================================================================

class StatusFrame : public wxFrame
{
public:
    StatusFrame()
        : wxFrame(nullptr, wxID_ANY, "XLL Status",
                  wxDefaultPosition, wxSize(320, 120))
    {
        auto* panel = new wxPanel(this);
        auto* vbox  = new wxBoxSizer(wxVERTICAL);

        m_label = new wxStaticText(panel, wxID_ANY,
                                   "This window is non-modal.\n"
                                   "Excel remains fully interactive.",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_CENTRE_HORIZONTAL);

        auto* btn = new wxButton(panel, wxID_ANY, "Update");
        btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            m_label->SetLabel("Button clicked!");
            m_label->GetContainingSizer()->Layout();
        });

        vbox->AddStretchSpacer();
        vbox->Add(m_label, 0, wxALIGN_CENTER | wxALL, 8);
        vbox->Add(btn,     0, wxALIGN_CENTER | wxBOTTOM, 10);
        vbox->AddStretchSpacer();

        panel->SetSizer(vbox);
    }

private:
    wxStaticText* m_label = nullptr;
};

auto wxStatusCmd =
    xll::Command("WX.STATUS")
    | xll::Procedure("ShowWxStatus")
    | xll::Category("wxWidgets Examples")
    | xll::Description(
        "Shows a non-modal wxWidgets frame owned by the Excel window. "
        "Excel remains fully interactive while the frame is open.");
XLL_REGISTER(wxStatusCmd);

XLL_FUNCTION void XLLAPI ShowWxStatus()
{
    if (!s_wxInitialized) return;

    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    // Heap-allocate: the frame must outlive this function.
    // wxWidgets deletes it automatically when the user closes it.
    auto* frame = new StatusFrame();

    // NativeOwnerSetup: sets owner + centres, but does NOT disable Excel.
    NativeOwnerSetup setup(static_cast<HWND>(frame->GetHWND()), excelHwnd);

    frame->Show();
    // Returns immediately — Excel's message loop keeps the frame alive.
}


