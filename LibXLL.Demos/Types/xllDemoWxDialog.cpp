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
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// xlcall.hpp (pulled in by every xlFunctions header) contains a bare
// #include <windows.h>; without WIN32_LEAN_AND_MEAN that drags in the old
// winsock.h, which then conflicts with the winsock2.h that wx and oleauto
// require.  Defining it here — before every other include — prevents that.
//
// wx headers must still come before LibXLL / ExcelSDK headers because
// wx/msw/wrapwin.h also does setup work beyond just WIN32_LEAN_AND_MEAN
// (e.g. it pulls in winsock2.h itself before the rest of windows.h).
// Since xlFunctions headers are LibXLL headers they must follow wx.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <iostream>

#include <wx/wx.h>
#include <oleauto.h>    // GetActiveObject, SysAllocString, IDispatch

// LibXLL / ExcelSDK headers — safe here because <windows.h> was already
// pulled in (with WIN32_LEAN_AND_MEAN) by wx above.
#include "xlFunctions/ActiveCell.hpp"
#include "xlFunctions/AppTitle.hpp"
#include "xlFunctions/Documents.hpp"
#include "xlFunctions/FormulaConvert.hpp"
#include "xlFunctions/RefText.hpp"
#include "xlFunctions/SheetId.hpp"
#include "xlFunctions/Stack.hpp"

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

    // convert_formula demo: convert =A1+B1 from A1 notation to R1C1.
    const auto converted =
        xll::convert_formula<xll::From<xll::A1>>(xll::String("=A1+B1"), xll::RefStyle::Absolute);
    if (converted)
        std::cout << "convert_formula: =A1+B1 -> " << *converted << "\n";
    else
        std::cout << "convert_formula: conversion failed\n";
}

// ============================================================================
// COM automation helpers
//
// Since the XLL runs inside Excel's process on Excel's main thread, COM is
// already initialised.  We attach to the running instance via GetActiveObject
// and drive it through IDispatch — no CoInitialize/CoUninitialize needed.
// ============================================================================

namespace {

// Retrieve a property from an IDispatch object by name.
inline HRESULT com_get(IDispatch* pDisp, LPCOLESTR name, VARIANT& result)
{
    DISPID id;
    LPOLESTR pName = const_cast<LPOLESTR>(name);
    if (HRESULT hr = pDisp->GetIDsOfNames(IID_NULL, &pName, 1,
                                           LOCALE_USER_DEFAULT, &id);
        FAILED(hr))
        return hr;

    DISPPARAMS dp = { nullptr, nullptr, 0, 0 };
    VariantInit(&result);
    return pDisp->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                          DISPATCH_PROPERTYGET, &dp, &result,
                          nullptr, nullptr);
}

// Set a BSTR property on an IDispatch object by name.
// The caller retains ownership of the BSTR; this function does not free it.
inline HRESULT com_put_bstr(IDispatch* pDisp, LPCOLESTR name, BSTR value)
{
    DISPID id;
    LPOLESTR pName = const_cast<LPOLESTR>(name);
    if (HRESULT hr = pDisp->GetIDsOfNames(IID_NULL, &pName, 1,
                                           LOCALE_USER_DEFAULT, &id);
        FAILED(hr))
        return hr;

    VARIANT val;
    VariantInit(&val);
    val.vt      = VT_BSTR;
    val.bstrVal = value;   // borrowed reference — caller owns it

    DISPID namedArg = DISPID_PROPERTYPUT;
    DISPPARAMS dp   = { &val, &namedArg, 1, 1 };
    return pDisp->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                          DISPATCH_PROPERTYPUT, &dp,
                          nullptr, nullptr, nullptr);
}

// Write a string to the currently active Excel cell via COM automation.
// Returns true on success.
bool write_to_active_cell(const std::wstring& text)
{
    CLSID clsid;
    if (FAILED(CLSIDFromProgID(L"Excel.Application", &clsid)))
        return false;

    IUnknown* pUnk = nullptr;
    if (FAILED(GetActiveObject(clsid, nullptr, &pUnk)))
        return false;

    IDispatch* pApp = nullptr;
    HRESULT hr = pUnk->QueryInterface(IID_IDispatch,
                                       reinterpret_cast<void**>(&pApp));
    pUnk->Release();
    if (FAILED(hr)) return false;

    VARIANT vCell;
    hr = com_get(pApp, L"ActiveCell", vCell);
    pApp->Release();
    if (FAILED(hr) || vCell.vt != VT_DISPATCH) {
        VariantClear(&vCell);
        return false;
    }

    BSTR bstr = SysAllocString(text.c_str());
    hr = com_put_bstr(vCell.pdispVal, L"Value", bstr);
    SysFreeString(bstr);
    VariantClear(&vCell);   // also releases vCell.pdispVal
    return SUCCEEDED(hr);
}

} // namespace

// ============================================================================
// Non-modal frame (WX.STATUS)
//
// The "Greet Active Cell" button writes "Hello, <name>!" to the currently
// selected Excel cell using COM automation (IDispatch / GetActiveObject).
//
// Keyboard isolation — WH_GETMESSAGE hook
// ----------------------------------------
// After ShowWxStatus() returns, Excel owns the thread's message loop and
// intercepts keyboard messages before they reach our controls, routing them
// to the active cell instead.
//
// A WH_GETMESSAGE hook is installed on the main thread for the lifetime of
// the frame.  Because hooks are chained in LIFO order, ours runs before any
// hook Excel has installed.  For every keyboard message (WM_KEYFIRST ..
// WM_KEYLAST) whose target HWND belongs to our frame the hook:
//   1. Calls TranslateMessage — generates WM_CHAR from WM_KEYDOWN as normal.
//   2. Calls DispatchMessage  — delivers the message to the intended control.
//   3. Sets msg.message = WM_NULL — the rest of the hook chain (including
//      Excel's) and Excel's own TranslateMessage/DispatchMessage calls become
//      harmless no-ops.
// ============================================================================

// File-scope state shared between the hook proc and StatusFrame.
// wxFrame* avoids a forward-declaration problem: the hook proc is defined
// before StatusFrame, but only calls wxWindow::GetHWND() — a wxFrame method.
static wxFrame* s_statusFrame = nullptr;
static HHOOK    s_getMsgHook  = nullptr;

// WH_GETMESSAGE hook — installed on Excel's main thread.
// Must be a plain function (HOOKPROC = __stdcall function pointer).
static LRESULT CALLBACK StatusGetMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == PM_REMOVE && s_statusFrame) {
        MSG* pMsg = reinterpret_cast<MSG*>(lParam);

        // Only intercept keyboard messages with a valid target window.
        if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST
            && pMsg->hwnd != nullptr)
        {
            const HWND frameHwnd = static_cast<HWND>(s_statusFrame->GetHWND());
            if (pMsg->hwnd == frameHwnd || ::IsChild(frameHwnd, pMsg->hwnd)) {
                // Translate (WM_KEYDOWN → posts WM_CHAR) then dispatch
                // directly to the intended control.
                ::TranslateMessage(pMsg);
                ::DispatchMessage(pMsg);

                // Nullify before calling the next hook so that Excel's
                // TranslateMessage and DispatchMessage are both no-ops.
                pMsg->message = WM_NULL;
            }
        }
    }
    return ::CallNextHookEx(s_getMsgHook, nCode, wParam, lParam);
}

class StatusFrame : public wxFrame
{
public:
    StatusFrame()
        : wxFrame(nullptr, wxID_ANY, "XLL Status",
                  wxDefaultPosition, wxSize(340, 170))
    {
        // Register with the hook proc before any child window is created.
        s_statusFrame = this;
        s_getMsgHook  = ::SetWindowsHookEx(WH_GETMESSAGE,
                                            StatusGetMsgHook,
                                            nullptr,
                                            ::GetCurrentThreadId());

        auto* panel = new wxPanel(this);
        auto* vbox  = new wxBoxSizer(wxVERTICAL);
        auto* hbox  = new wxBoxSizer(wxHORIZONTAL);

        auto* nameLabel = new wxStaticText(panel, wxID_ANY, "Your name:");
        m_name = new wxTextCtrl(panel, wxID_ANY, "World",
                                wxDefaultPosition, wxSize(160, -1),
                                wxTE_PROCESS_ENTER);
        hbox->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        hbox->Add(m_name,    1, wxEXPAND);

        m_label = new wxStaticText(panel, wxID_ANY,
                                   "Type a name and press the button\n"
                                   "to greet the active Excel cell.",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_CENTRE_HORIZONTAL);

        auto* btn = new wxButton(panel, wxID_ANY, "Greet Active Cell");
        btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            const std::wstring greeting =
                L"Hello, " + m_name->GetValue().ToStdWstring() + L"!";
            if (write_to_active_cell(greeting))
                m_label->SetLabel("Greeting written to active cell.");
            else
                m_label->SetLabel("Failed to write to active cell.");
            m_label->GetContainingSizer()->Layout();
        });

        // Pressing Enter in the text box also triggers the button.
        m_name->Bind(wxEVT_TEXT_ENTER, [btn](wxCommandEvent&) {
            wxCommandEvent evt(wxEVT_BUTTON, btn->GetId());
            btn->GetEventHandler()->ProcessEvent(evt);
        });

        vbox->AddStretchSpacer();
        vbox->Add(hbox,    0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        vbox->Add(m_label, 0, wxALIGN_CENTER | wxALL, 8);
        vbox->Add(btn,     0, wxALIGN_CENTER | wxBOTTOM, 10);
        vbox->AddStretchSpacer();

        panel->SetSizer(vbox);
    }

    ~StatusFrame() override
    {
        // Nullify first so the hook proc cannot use 'this' while we are
        // mid-destruction (e.g. if a message is processed during teardown).
        s_statusFrame = nullptr;
        if (s_getMsgHook) {
            ::UnhookWindowsHookEx(s_getMsgHook);
            s_getMsgHook = nullptr;
        }
    }

private:
    wxStaticText* m_label = nullptr;
    wxTextCtrl*   m_name  = nullptr;
};

auto wxStatusCmd =
    xll::Command("WX.STATUS")
    | xll::Procedure("ShowWxStatus")
    | xll::Category("wxWidgets Examples")
    | xll::Description(
        "Shows a non-modal wxWidgets frame owned by the Excel window. "
        "Excel remains fully interactive while the frame is open. "
        "If the frame is already open, it is brought to the front.");
XLL_REGISTER(wxStatusCmd);

XLL_FUNCTION void XLLAPI ShowWxStatus()
{
    if (!s_wxInitialized) return;

    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    // Lazily created once; persists (hidden when closed) for the XLL lifetime.
    // The constructor installs the WH_GETMESSAGE hook; the destructor removes it.
    if (!s_statusFrame) {
        new StatusFrame();   // sets s_statusFrame and installs hook

        // Hide on close rather than destroy — keeps the hook alive and avoids
        // re-creating the window on subsequent WX.STATUS invocations.
        s_statusFrame->Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent&) {
            s_statusFrame->Hide();
        });
    }

    // (Re-)centre on Excel every time the frame is shown in case Excel moved.
    NativeOwnerSetup setup(static_cast<HWND>(s_statusFrame->GetHWND()),
                            excelHwnd);

    s_statusFrame->Show();
    s_statusFrame->Raise();
    // Returns immediately — Excel's message loop keeps the frame alive.

}


