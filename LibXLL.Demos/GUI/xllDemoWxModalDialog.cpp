// xllDemoWxModalDialog.cpp
//
// Minimal demo: a single XLL command that shows a modal wxWidgets dialog.
//
// When you only need modal dialogs (no persistent non-modal frames), the
// code is dramatically simpler than xllDemoWxDialog.cpp:
//
//   - wxInitialize() on xlAutoOpen, wxUninitialize() on xlAutoClose.
//     Both run on Excel's main thread, which becomes the wx "main" thread.
//   - The command creates the dialog, calls ShowModal(), reads the result
//     — all on the same thread, no cross-thread marshaling required.
//   - No dedicated UI thread, no dispatchers, no futures/promises,
//     no message-pumping wait loop.
//
// The only non-trivial part is NativeOwnerModal: because we create the
// dialog with a nullptr wx parent (to avoid wxWidgets parent-chain issues),
// we use Win32 to manually set the owner, centre the dialog over Excel,
// and disable/re-enable the Excel window for proper modal semantics.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// wx headers must come before LibXLL / ExcelSDK headers — see the full
// explanation in xllDemoWxDialog.cpp.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <string>

#include <wx/wx.h>

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

// ============================================================================
// NativeOwnerModal
//
// RAII helper: sets a Win32 owner relationship, centres the dialog over the
// owner, and disables the owner for the lifetime of this object.
// ============================================================================

class NativeOwnerModal
{
public:
    NativeOwnerModal(HWND dialogHwnd, HWND ownerHwnd)
        : m_owner(ownerHwnd)
    {
        // Make the dialog visually owned by Excel.
        SetWindowLongPtr(dialogHwnd, GWLP_HWNDPARENT,
                         reinterpret_cast<LONG_PTR>(ownerHwnd));

        // Centre the dialog over the owner.
        RECT ow{}, wd{};
        GetWindowRect(ownerHwnd,  &ow);
        GetWindowRect(dialogHwnd, &wd);
        const int dw = wd.right  - wd.left;
        const int dh = wd.bottom - wd.top;
        SetWindowPos(dialogHwnd, nullptr,
                     ow.left + (ow.right  - ow.left - dw) / 2,
                     ow.top  + (ow.bottom - ow.top  - dh) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        // Disable the owner — standard modal semantics.
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
    HWND m_owner;
};

// ============================================================================
// GreetingDialog
// ============================================================================

class GreetingDialog : public wxDialog
{
public:
    explicit GreetingDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "wxWidgets inside an XLL",
                   wxDefaultPosition, wxSize(360, 150))
    {
        auto* panel = new wxPanel(this);
        auto* vbox  = new wxBoxSizer(wxVERTICAL);
        auto* hbox  = new wxBoxSizer(wxHORIZONTAL);
        auto* btns  = new wxBoxSizer(wxHORIZONTAL);

        auto* label = new wxStaticText(panel, wxID_ANY, "Enter your name:");
        m_input     = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
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

        m_input->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
            EndModal(wxID_OK);
        });
    }

    [[nodiscard]] std::string GetInput() const
    {
        return m_input->GetValue().utf8_string();
    }

private:
    wxTextCtrl* m_input = nullptr;
};

// ============================================================================
// Minimal wxApp — makes wxInitialize() create a GUI app (not a console stub).
// Without this, wxDialog::Create segfaults in wxApp::MSWGetDefaultLayout()
// because the default wxDummyConsoleApp has no GUI support.
// ============================================================================

class ModalApp : public wxApp
{
public:
    bool OnInit() override { return true; }
};

wxIMPLEMENT_APP_NO_MAIN(ModalApp);

// ============================================================================
// Lifecycle: wxInitialize / wxUninitialize on Excel's main thread
// ============================================================================

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { wxInitialize(); });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] { wxUninitialize(); });
XLL_REGISTER(onClose);

// ============================================================================
// Command: WX.MODAL.GREETING
// ============================================================================

auto wxModalGreetingCmd =
    xll::Command("WX.MODAL.GREETING")
    | xll::Procedure("ShowWxModalGreeting")
    | xll::Category("wxWidgets Examples")
    | xll::Description(
        "Shows a modal wxWidgets dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(wxModalGreetingCmd);

XLL_FUNCTION void XLLAPI ShowWxModalGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    GreetingDialog dlg(nullptr);
    NativeOwnerModal modal(static_cast<HWND>(dlg.GetHWND()), excelHwnd);

    if (dlg.ShowModal() == wxID_OK) {
        const std::string name = dlg.GetInput();
        if (!name.empty()) {
            xll::alert(xll::String("Hello, " + name + "!"));
        }
    }
}


