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
//
// class NativeOwnerSetup
// {
// public:
//     NativeOwnerSetup(HWND windowHwnd, HWND ownerHwnd)
//     {
//         SetWindowLongPtr(windowHwnd, GWLP_HWNDPARENT,
//                            reinterpret_cast<LONG_PTR>(ownerHwnd));
//
//         RECT ow{}, wd{};
//         GetWindowRect(ownerHwnd,  &ow);
//         GetWindowRect(windowHwnd, &wd);
//         const int dw = wd.right  - wd.left;
//         const int dh = wd.bottom - wd.top;
//         SetWindowPos(windowHwnd, nullptr,
//                        ow.left + (ow.right  - ow.left - dw) / 2,
//                        ow.top  + (ow.bottom - ow.top  - dh) / 2,
//                        0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
//     }
//     // No destructor logic — the window is unaffected when this goes out of scope.
// };
//
// // ============================================================================
// // NativeOwnerModal
// //
// // Extends NativeOwnerSetup with RAII modal semantics: disables the owner
// // window for exclusive input while the dialog is open, then restores it.
// // ============================================================================
//
// class NativeOwnerModal
// {
// public:
//     NativeOwnerModal(HWND dialogHwnd, HWND ownerHwnd)
//         : m_setup(dialogHwnd, ownerHwnd), m_owner(ownerHwnd)
//     {
//         EnableWindow(ownerHwnd, FALSE);
//     }
//
//     ~NativeOwnerModal()
//     {
//         EnableWindow(m_owner, TRUE);
//         SetForegroundWindow(m_owner);
//     }
//
//     NativeOwnerModal(const NativeOwnerModal&)            = delete;
//     NativeOwnerModal& operator=(const NativeOwnerModal&) = delete;
//
// private:
//     NativeOwnerSetup m_setup;   // owner relationship + centering
//     HWND             m_owner;
// };
//
// // ============================================================================
// // DLL-level wxWidgets initialisation state
// // ============================================================================
//
// static bool s_wxInitialized = false;
//
// // A minimal wxApp subclass for hosting wx GUI elements from a DLL.
// //
// // wxInitialize() creates wxDummyConsoleApp, which only inherits from
// // wxAppConsole — NOT from wxApp.  In a GUI build, wxTheApp is typed as
// // wxApp*, so any virtual call through it (e.g. wxApp::MSWGetDefaultLayout
// // in toplevel.cpp calling wxTheApp->GetLayoutDirection()) is a call through
// // an invalid pointer → segfault.
// //
// // The fix: wxInitialize() only creates wxDummyConsoleApp when
// // wxApp::GetInstance() returns nullptr.  Setting our own wxApp-derived
// // instance first causes wxInitialize() to skip that step and use ours.
// class XllApp : public wxApp
// {
// public:
//     bool OnInit() override { return true; }
// };
//
// // ============================================================================
// // Add-in lifecycle
// // ============================================================================
//
// xll::AddInManagerInfo dllName([] { return xll::String("wxDialog Demo"); });

// auto onOpen =
//     xll::OnOpen()
//     | xll::Before([] {
//         // if (!s_wxInitialized) {
//         //     if (!wxApp::GetInstance())
//         //         wxApp::SetInstance(new XllApp());
//         //     if (wxInitialize())
//         //         s_wxInitialized = true;
//         // }
//     });
// XLL_REGISTER(onOpen);
//
// auto onClose =
//     xll::OnClose()
//     | xll::Before([] {
//         // if (s_wxInitialized) {
//         //     wxUninitialize();
//         //     s_wxInitialized = false;
//         // }
//     });
// XLL_REGISTER(onClose);

// // ============================================================================
// // Modal dialog (WX.GREETING)
// // ============================================================================
//
// class GreetingDialog : public wxDialog
// {
// public:
//     explicit GreetingDialog(wxWindow* parent)
//         : wxDialog(parent, wxID_ANY, "wxWidgets inside an XLL",
//                    wxDefaultPosition, wxSize(360, 150))
//     {
//         auto* panel  = new wxPanel(this);
//         auto* vbox   = new wxBoxSizer(wxVERTICAL);
//         auto* hbox   = new wxBoxSizer(wxHORIZONTAL);
//         auto* btns   = new wxBoxSizer(wxHORIZONTAL);
//
//         auto* label  = new wxStaticText(panel, wxID_ANY, "Enter your name:");
//         m_input      = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
//                                       wxDefaultPosition, wxSize(200, -1),
//                                       wxTE_PROCESS_ENTER);
//
//         hbox->Add(label,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
//         hbox->Add(m_input, 1, wxEXPAND);
//
//         auto* btnOk     = new wxButton(panel, wxID_OK,     "OK");
//         auto* btnCancel = new wxButton(panel, wxID_CANCEL, "Cancel");
//         btns->Add(btnOk,     0, wxRIGHT, 6);
//         btns->Add(btnCancel, 0);
//
//         vbox->AddStretchSpacer();
//         vbox->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
//         vbox->AddSpacer(10);
//         vbox->Add(btns, 0, wxALIGN_CENTER | wxBOTTOM, 12);
//         vbox->AddStretchSpacer();
//
//         panel->SetSizer(vbox);
//         Centre();
//
//         // Pressing Enter in the text box confirms the dialog.
//         m_input->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { EndModal(wxID_OK); });
//     }
//
//     // Returns the text the user typed, encoded as UTF-8 std::string.
//     [[nodiscard]] std::string GetInput() const
//     {
//         return m_input->GetValue().utf8_string();
//     }
//
// private:
//     wxTextCtrl* m_input = nullptr;
// };
//
// // ============================================================================
// // Command: WX.GREETING
// // ============================================================================
//
// auto wxGreetingCmd =
//     xll::Command("WX.GREETING")
//     | xll::Procedure("ShowWxGreeting")
//     | xll::Category("wxWidgets Examples")
//     | xll::Description(
//         "Shows a modal wxWidgets dialog parented to the Excel window, "
//         "then greets the user with xll::alert.");
// XLL_REGISTER(wxGreetingCmd);
//
// XLL_FUNCTION void XLLAPI ShowWxGreeting()
// {
//     if (!s_wxInitialized) return;
//
//     HWND excelHwnd = xll::get_hwnd();
//     if (!excelHwnd) return;
//
//     GreetingDialog dlg(nullptr);
//     NativeOwnerModal modal(static_cast<HWND>(dlg.GetHWND()), excelHwnd);
//
//     if (dlg.ShowModal() == wxID_OK)
//         xll::alert(xll::String("Hello, " + dlg.GetInput() + "!"));
//
//     // convert_formula demo: convert =A1+B1 from A1 notation to R1C1.
//     const auto converted =
//         xll::convert_formula<xll::From<xll::A1>>(xll::String("=A1+B1"), xll::RefStyle::Absolute);
//     if (converted)
//         std::cout << "convert_formula: =A1+B1 -> " << *converted << "\n";
//     else
//         std::cout << "convert_formula: conversion failed\n";
// }
//
// // ============================================================================
// // COM automation helpers
// //
// // Since the XLL runs inside Excel's process on Excel's main thread, COM is
// // already initialised.  We attach to the running instance via GetActiveObject
// // and drive it through IDispatch — no CoInitialize/CoUninitialize needed.
// // ============================================================================
//
// namespace {
//
// // Retrieve a property from an IDispatch object by name.
// inline HRESULT com_get(IDispatch* pDisp, LPCOLESTR name, VARIANT& result)
// {
//     DISPID id;
//     LPOLESTR pName = const_cast<LPOLESTR>(name);
//     if (HRESULT hr = pDisp->GetIDsOfNames(IID_NULL, &pName, 1,
//                                            LOCALE_USER_DEFAULT, &id);
//         FAILED(hr))
//         return hr;
//
//     DISPPARAMS dp = { nullptr, nullptr, 0, 0 };
//     VariantInit(&result);
//     return pDisp->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
//                           DISPATCH_PROPERTYGET, &dp, &result,
//                           nullptr, nullptr);
// }
//
// // Set a BSTR property on an IDispatch object by name.
// // The caller retains ownership of the BSTR; this function does not free it.
// inline HRESULT com_put_bstr(IDispatch* pDisp, LPCOLESTR name, BSTR value)
// {
//     DISPID id;
//     LPOLESTR pName = const_cast<LPOLESTR>(name);
//     if (HRESULT hr = pDisp->GetIDsOfNames(IID_NULL, &pName, 1,
//                                            LOCALE_USER_DEFAULT, &id);
//         FAILED(hr))
//         return hr;
//
//     VARIANT val;
//     VariantInit(&val);
//     val.vt      = VT_BSTR;
//     val.bstrVal = value;   // borrowed reference — caller owns it
//
//     DISPID namedArg = DISPID_PROPERTYPUT;
//     DISPPARAMS dp   = { &val, &namedArg, 1, 1 };
//     return pDisp->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
//                           DISPATCH_PROPERTYPUT, &dp,
//                           nullptr, nullptr, nullptr);
// }
//
// // Write a string to the currently active Excel cell via COM automation.
// // Returns true on success.
// bool write_to_active_cell(const std::wstring& text)
// {
//     CLSID clsid;
//     if (FAILED(CLSIDFromProgID(L"Excel.Application", &clsid)))
//         return false;
//
//     IUnknown* pUnk = nullptr;
//     if (FAILED(GetActiveObject(clsid, nullptr, &pUnk)))
//         return false;
//
//     IDispatch* pApp = nullptr;
//     HRESULT hr = pUnk->QueryInterface(IID_IDispatch,
//                                        reinterpret_cast<void**>(&pApp));
//     pUnk->Release();
//     if (FAILED(hr)) return false;
//
//     VARIANT vCell;
//     hr = com_get(pApp, L"ActiveCell", vCell);
//     pApp->Release();
//     if (FAILED(hr) || vCell.vt != VT_DISPATCH) {
//         VariantClear(&vCell);
//         return false;
//     }
//
//     BSTR bstr = SysAllocString(text.c_str());
//     hr = com_put_bstr(vCell.pdispVal, L"Value", bstr);
//     SysFreeString(bstr);
//     VariantClear(&vCell);   // also releases vCell.pdispVal
//     return SUCCEEDED(hr);
// }
//
// } // namespace
//
// // ============================================================================
// // Non-modal frame (WX.STATUS)
// //
// // The "Greet Active Cell" button writes "Hello, <name>!" to the currently
// // selected Excel cell using COM automation (IDispatch / GetActiveObject).
// //
// // Keyboard isolation — WH_GETMESSAGE hook
// // ----------------------------------------
// // After ShowWxStatus() returns, Excel owns the thread's message loop and
// // intercepts keyboard messages before they reach our controls, routing them
// // to the active cell instead.
// //
// // A WH_GETMESSAGE hook is installed on the main thread for the lifetime of
// // the frame.  Because hooks are chained in LIFO order, ours runs before any
// // hook Excel has installed.  For every keyboard message (WM_KEYFIRST ..
// // WM_KEYLAST) whose target HWND belongs to our frame the hook:
// //   1. Calls TranslateMessage — generates WM_CHAR from WM_KEYDOWN as normal.
// //   2. Calls DispatchMessage  — delivers the message to the intended control.
// //   3. Sets msg.message = WM_NULL — the rest of the hook chain (including
// //      Excel's) and Excel's own TranslateMessage/DispatchMessage calls become
// //      harmless no-ops.
// // ============================================================================
//
// // File-scope state shared between the hook proc and StatusFrame.
// // wxFrame* avoids a forward-declaration problem: the hook proc is defined
// // before StatusFrame, but only calls wxWindow::GetHWND() — a wxFrame method.
// static wxFrame* s_statusFrame = nullptr;
// static HHOOK    s_getMsgHook  = nullptr;
//
// // WH_GETMESSAGE hook — installed on Excel's main thread.
// // Must be a plain function (HOOKPROC = __stdcall function pointer).
// static LRESULT CALLBACK StatusGetMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
// {
//     if (nCode == HC_ACTION && wParam == PM_REMOVE && s_statusFrame) {
//         MSG* pMsg = reinterpret_cast<MSG*>(lParam);
//
//         // Only intercept keyboard messages with a valid target window.
//         if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST
//             && pMsg->hwnd != nullptr)
//         {
//             const HWND frameHwnd = static_cast<HWND>(s_statusFrame->GetHWND());
//             if (pMsg->hwnd == frameHwnd || ::IsChild(frameHwnd, pMsg->hwnd)) {
//                 // Translate (WM_KEYDOWN → posts WM_CHAR) then dispatch
//                 // directly to the intended control.
//                 ::TranslateMessage(pMsg);
//                 ::DispatchMessage(pMsg);
//
//                 // Nullify before calling the next hook so that Excel's
//                 // TranslateMessage and DispatchMessage are both no-ops.
//                 pMsg->message = WM_NULL;
//             }
//         }
//     }
//     return ::CallNextHookEx(s_getMsgHook, nCode, wParam, lParam);
// }
//
// class StatusFrame : public wxFrame
// {
// public:
//     StatusFrame()
//         : wxFrame(nullptr, wxID_ANY, "XLL Status",
//                   wxDefaultPosition, wxSize(340, 170))
//     {
//         // Register with the hook proc before any child window is created.
//         s_statusFrame = this;
//         s_getMsgHook  = ::SetWindowsHookEx(WH_GETMESSAGE,
//                                             StatusGetMsgHook,
//                                             nullptr,
//                                             ::GetCurrentThreadId());
//
//         auto* panel = new wxPanel(this);
//         auto* vbox  = new wxBoxSizer(wxVERTICAL);
//         auto* hbox  = new wxBoxSizer(wxHORIZONTAL);
//
//         auto* nameLabel = new wxStaticText(panel, wxID_ANY, "Your name:");
//         m_name = new wxTextCtrl(panel, wxID_ANY, "World",
//                                 wxDefaultPosition, wxSize(160, -1),
//                                 wxTE_PROCESS_ENTER);
//         hbox->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
//         hbox->Add(m_name,    1, wxEXPAND);
//
//         m_label = new wxStaticText(panel, wxID_ANY,
//                                    "Type a name and press the button\n"
//                                    "to greet the active Excel cell.",
//                                    wxDefaultPosition, wxDefaultSize,
//                                    wxALIGN_CENTRE_HORIZONTAL);
//
//         auto* btn = new wxButton(panel, wxID_ANY, "Greet Active Cell");
//         btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
//             const std::wstring greeting =
//                 L"Hello, " + m_name->GetValue().ToStdWstring() + L"!";
//             if (write_to_active_cell(greeting))
//                 m_label->SetLabel("Greeting written to active cell.");
//             else
//                 m_label->SetLabel("Failed to write to active cell.");
//             m_label->GetContainingSizer()->Layout();
//         });
//
//         // Pressing Enter in the text box also triggers the button.
//         m_name->Bind(wxEVT_TEXT_ENTER, [btn](wxCommandEvent&) {
//             wxCommandEvent evt(wxEVT_BUTTON, btn->GetId());
//             btn->GetEventHandler()->ProcessEvent(evt);
//         });
//
//         vbox->AddStretchSpacer();
//         vbox->Add(hbox,    0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
//         vbox->Add(m_label, 0, wxALIGN_CENTER | wxALL, 8);
//         vbox->Add(btn,     0, wxALIGN_CENTER | wxBOTTOM, 10);
//         vbox->AddStretchSpacer();
//
//         panel->SetSizer(vbox);
//     }
//
//     ~StatusFrame() override
//     {
//         // Nullify first so the hook proc cannot use 'this' while we are
//         // mid-destruction (e.g. if a message is processed during teardown).
//         s_statusFrame = nullptr;
//         if (s_getMsgHook) {
//             ::UnhookWindowsHookEx(s_getMsgHook);
//             s_getMsgHook = nullptr;
//         }
//     }
//
// private:
//     wxStaticText* m_label = nullptr;
//     wxTextCtrl*   m_name  = nullptr;
// };
//
// auto wxStatusCmd =
//     xll::Command("WX.STATUS")
//     | xll::Procedure("ShowWxStatus")
//     | xll::Category("wxWidgets Examples")
//     | xll::Description(
//         "Shows a non-modal wxWidgets frame owned by the Excel window. "
//         "Excel remains fully interactive while the frame is open. "
//         "If the frame is already open, it is brought to the front.");
// XLL_REGISTER(wxStatusCmd);
//
// XLL_FUNCTION void XLLAPI ShowWxStatus()
// {
//     if (!s_wxInitialized) return;
//
//     HWND excelHwnd = xll::get_hwnd();
//     if (!excelHwnd) return;
//
//     // Lazily created once; persists (hidden when closed) for the XLL lifetime.
//     // The constructor installs the WH_GETMESSAGE hook; the destructor removes it.
//     if (!s_statusFrame) {
//         new StatusFrame();   // sets s_statusFrame and installs hook
//
//         // Hide on close rather than destroy — keeps the hook alive and avoids
//         // re-creating the window on subsequent WX.STATUS invocations.
//         s_statusFrame->Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent&) {
//             s_statusFrame->Hide();
//         });
//     }
//
//     // (Re-)centre on Excel every time the frame is shown in case Excel moved.
//     NativeOwnerSetup setup(static_cast<HWND>(s_statusFrame->GetHWND()),
//                             excelHwnd);
//
//     s_statusFrame->Show();
//     s_statusFrame->Raise();
//     // Returns immediately — Excel's message loop keeps the frame alive.
//
// }


#include <windows.h>
#include <oleauto.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

#include <wx/wx.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/msgqueue.h>


std::atomic<bool> s_shuttingDown{ false };


// ============================================================================
// COM automation helpers
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
    val.bstrVal = value;

    DISPID namedArg = DISPID_PROPERTYPUT;
    DISPPARAMS dp   = { &val, &namedArg, 1, 1 };
    return pDisp->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                         DISPATCH_PROPERTYPUT, &dp,
                         nullptr, nullptr, nullptr);
}

// Write a string to the currently active Excel cell via COM automation.
// Must be called on Excel's main thread in this design.
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
    VariantClear(&vCell);
    return SUCCEEDED(hr);
}

} // namespace

// ============================================================================
// Excel-thread bridge window
//
// A message-only Win32 window lives on Excel's thread. The wx UI thread posts
// requests to it whenever the button is clicked. The bridge then performs the
// Excel-sensitive work on the Excel thread.
// ============================================================================

namespace {

constexpr UINT WM_APP_WRITE_GREETING_RESULT = WM_APP + 0x540;
constexpr UINT WM_APP_WRITE_GREETING        = WM_APP + 0x541;

struct WriteGreetingRequest
{
    std::wstring text;
    HWND         replyHwnd = nullptr; // wx frame HWND
};

HWND              s_excelBridgeHwnd   = nullptr;
DWORD             s_excelThreadId     = 0;
std::once_flag    s_bridgeInitOnce;

LRESULT CALLBACK ExcelBridgeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_APP_WRITE_GREETING:
    {
        std::unique_ptr<WriteGreetingRequest> req(
            reinterpret_cast<WriteGreetingRequest*>(lParam));

        bool ok = false;
        if (req) {
            ok = write_to_active_cell(req->text);
            if (req->replyHwnd) {
                ::PostMessage(req->replyHwnd,
                              WM_APP_WRITE_GREETING_RESULT,
                              static_cast<WPARAM>(ok ? 1 : 0),
                              0);
            }
        }
        return 0;
    }

    default:
        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

bool EnsureExcelBridgeWindow()
{
    std::call_once(s_bridgeInitOnce, []() {
        s_excelThreadId = ::GetCurrentThreadId();

        WNDCLASSW wc = {};
        wc.lpfnWndProc   = ExcelBridgeWndProc;
        wc.hInstance     = ::GetModuleHandleW(nullptr);
        wc.lpszClassName = L"XllExcelBridgeWindow";

        ::RegisterClassW(&wc);

        s_excelBridgeHwnd = ::CreateWindowExW(
            0,
            wc.lpszClassName,
            L"",
            0,
            0, 0, 0, 0,
            HWND_MESSAGE, // message-only window
            nullptr,
            wc.hInstance,
            nullptr
        );
    });

    return s_excelBridgeHwnd != nullptr;
}

} // namespace

// ============================================================================
// wx UI thread
// ============================================================================

namespace {

class StatusFrame;

// Keep all wx-owned state on the wx UI thread side as much as possible.
std::thread              s_wxUiThread;
std::atomic<bool>        s_wxUiThreadRunning{ false };
std::atomic<bool>        s_wxUiThreadReady{ false };
std::mutex               s_wxUiMutex;
std::condition_variable  s_wxUiCv;

StatusFrame*             s_statusFrame = nullptr;
DWORD                    s_wxUiThreadId = 0;

class XllStatusApp : public wxApp
{
public:
    bool OnInit() override
    {
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(XllStatusApp);

class StatusFrame : public wxFrame
{
public:
    StatusFrame(HWND excelHwnd)
        : wxFrame(nullptr, wxID_ANY, "XLL Status",
                  wxDefaultPosition, wxSize(340, 170)),
          m_excelHwnd(excelHwnd)
    {
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
            SendGreetingRequest();
        });

        m_name->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
            SendGreetingRequest();
        });

        vbox->AddStretchSpacer();
        vbox->Add(hbox,    0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        vbox->Add(m_label, 0, wxALIGN_CENTER | wxALL, 8);
        vbox->Add(btn,     0, wxALIGN_CENTER | wxBOTTOM, 10);
        vbox->AddStretchSpacer();

        panel->SetSizer(vbox);

        // Hide instead of destroy.
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& evt) {
            Hide();
            evt.Veto();
        });

        // Handle async result posted back from Excel thread.
        Bind(wxEVT_THREAD, &StatusFrame::OnWriteResultEvent, this);

#ifdef _WIN32
        // Handle raw Win32 message sent back from Excel-thread bridge.
        HWND hwnd = static_cast<HWND>(GetHWND());
        m_oldWndProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                reinterpret_cast<LONG_PTR>(&StatusFrame::StaticWndProc)));
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // Optional: owner relationship to Excel for z-order/taskbar behavior.
        // This does NOT make Excel pump our messages; it just sets ownership.
        // if (m_excelHwnd) {
        //     ::SetWindowLongPtrW(hwnd, GWLP_HWNDPARENT,
        //                         reinterpret_cast<LONG_PTR>(m_excelHwnd));
        // }
#endif
    }

    ~StatusFrame() override
    {
#ifdef _WIN32
        HWND hwnd = static_cast<HWND>(GetHWND());
        if (hwnd && m_oldWndProc) {
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                reinterpret_cast<LONG_PTR>(m_oldWndProc));
        }
#endif
    }

    void BringUpNearExcel()
    {
        HWND excelHwnd = xll::get_hwnd();

        if (excelHwnd && ::IsWindow(excelHwnd)) {
            RECT rc{};
            if (::GetWindowRect(excelHwnd, &rc)) {
                SetPosition(wxPoint(rc.left + 60, rc.top + 60));
            }
        }

        Show();
        Raise();
    }

private:
    void SendGreetingRequest()
    {
        if (!s_excelBridgeHwnd) {
            m_label->SetLabel("Excel bridge window is not available.");
            Layout();
            return;
        }

        auto req = std::make_unique<WriteGreetingRequest>();
        req->text      = L"Hello, " + m_name->GetValue().ToStdWstring() + L"!";
        req->replyHwnd = static_cast<HWND>(GetHWND());

        if (!::PostMessage(s_excelBridgeHwnd,
                           WM_APP_WRITE_GREETING,
                           0,
                           reinterpret_cast<LPARAM>(req.get())))
        {
            m_label->SetLabel("Failed to post request to Excel thread.");
            Layout();
            return;
        }

        req.release(); // ownership transferred to Excel bridge proc
        m_label->SetLabel("Writing greeting...");
        Layout();
    }

    void OnWriteResultEvent(wxThreadEvent& evt)
    {
        const bool ok = evt.GetInt() != 0;
        m_label->SetLabel(ok
            ? "Greeting written to active cell."
            : "Failed to write to active cell.");
        Layout();
    }

#ifdef _WIN32
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto* self = reinterpret_cast<StatusFrame*>(
            ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (self && msg == WM_APP_WRITE_GREETING_RESULT) {
            auto* evt = new wxThreadEvent(wxEVT_THREAD);
            evt->SetInt(static_cast<int>(wParam));
            wxQueueEvent(self, evt);
            return 0;
        }

        if (self && self->m_oldWndProc) {
            return ::CallWindowProcW(self->m_oldWndProc, hwnd, msg, wParam, lParam);
        }

        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
#endif

private:
    wxStaticText* m_label = nullptr;
    wxTextCtrl*   m_name  = nullptr;
    HWND          m_excelHwnd = nullptr;

#ifdef _WIN32
    WNDPROC       m_oldWndProc = nullptr;
#endif
};

void WxUiThreadMain(HWND excelHwnd)
{
    s_wxUiThreadRunning = true;
    s_wxUiThreadId      = ::GetCurrentThreadId();

    // COM for this thread is optional unless you add COM work here later.
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    int argc = 0;
    char** argv = nullptr;

    wxApp::SetInstance(new XllStatusApp());

    if (!wxEntryStart(argc, argv)) {
        {
            std::lock_guard<std::mutex> lock(s_wxUiMutex);
            s_wxUiThreadReady = true;
            s_statusFrame = nullptr;
        }
        s_wxUiCv.notify_all();
        ::CoUninitialize();
        s_wxUiThreadRunning = false;
        return;
    }

    if (!wxTheApp || !wxTheApp->CallOnInit()) {
        wxEntryCleanup();
        {
            std::lock_guard<std::mutex> lock(s_wxUiMutex);
            s_wxUiThreadReady = true;
            s_statusFrame = nullptr;
        }
        s_wxUiCv.notify_all();
        ::CoUninitialize();
        s_wxUiThreadRunning = false;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(s_wxUiMutex);
        s_statusFrame = new StatusFrame(excelHwnd);
        s_wxUiThreadReady = true;
    }
    s_wxUiCv.notify_all();

    wxTheApp->OnRun();

    {
        std::lock_guard<std::mutex> lock(s_wxUiMutex);
        s_statusFrame = nullptr;
    }

    if (wxTheApp) {
        wxTheApp->OnExit();
    }
    wxEntryCleanup();

    ::CoUninitialize();
    s_wxUiThreadRunning = false;
}

bool EnsureWxUiThreadStarted(HWND excelHwnd)
{
    if (s_wxUiThreadRunning) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(s_wxUiMutex);
        s_wxUiThreadReady = false;
    }

    s_wxUiThread = std::thread([excelHwnd]() {
        WxUiThreadMain(excelHwnd);
    });

    {
        std::unique_lock<std::mutex> lock(s_wxUiMutex);
        s_wxUiCv.wait(lock, [] { return s_wxUiThreadReady.load(); });
    }

    return s_statusFrame != nullptr;
}

    void ShowStatusFrameOnWxThread()
{
    StatusFrame* frame = nullptr;
    {
        std::lock_guard<std::mutex> lock(s_wxUiMutex);
        frame = s_statusFrame;
    }

    if (frame && wxTheApp) {
        wxTheApp->CallAfter([frame]() {
            frame->BringUpNearExcel();
        });
    }
}

} // namespace

// ============================================================================
// XLL registration
// ============================================================================

auto wxStatusCmd =
    xll::Command("WX.STATUS")
    | xll::Procedure("ShowWxStatus")
    | xll::Category("wxWidgets Examples")
    | xll::Description(
        "Shows a non-modal wxWidgets frame on a dedicated UI thread. "
        "Excel remains interactive while the frame is open. "
        "If the frame is already open, it is brought to the front.");
XLL_REGISTER(wxStatusCmd);

// ============================================================================
// Entry point
// ============================================================================

void ShutdownStatusUi()
{
    s_shuttingDown = true;

    // Ask wx thread to stop.
    if (s_wxUiThreadRunning && wxTheApp) {
        wxTheApp->CallAfter([]() {
            if (s_statusFrame) {
                // Destroy on the wx thread.
                s_statusFrame->Destroy();
                s_statusFrame = nullptr;
            }

            if (wxTheApp) {
                wxTheApp->ExitMainLoop();
            }
        });
    }

    if (s_wxUiThread.joinable()) {
        s_wxUiThread.join();
    }

    s_wxUiThreadRunning = false;
    s_wxUiThreadReady   = false;
    s_wxUiThreadId      = 0;

    if (s_excelBridgeHwnd) {
        ::DestroyWindow(s_excelBridgeHwnd);
        s_excelBridgeHwnd = nullptr;
    }

    s_excelThreadId = 0;
}

auto onOpen =
    xll::OnOpen()
    | xll::Before([] {
        s_shuttingDown = false;
        EnsureExcelBridgeWindow();
    });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] {
        ShutdownStatusUi();
    });
XLL_REGISTER(onClose);


XLL_FUNCTION void XLLAPI ShowWxStatus()
{
    if (s_shuttingDown) {
        return;
    }

    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) {
        return;
    }

    if (!EnsureExcelBridgeWindow()) {
        return;
    }

    if (!EnsureWxUiThreadStarted(excelHwnd)) {
        return;
    }

    ShowStatusFrameOnWxThread();
}