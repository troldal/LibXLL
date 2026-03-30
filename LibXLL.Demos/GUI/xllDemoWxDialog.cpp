// xllDemoWxDialog.cpp
//
// Demonstrates hosting wxWidgets windows inside an XLL add-in.
//
// Architecture
// ------------
// wxWidgets needs a dedicated "main" thread (wxThread::IsMain() is stamped at
// wxEntryStart time).  A lightweight UiThread manages that thread's lifecycle
// with a ready/failed handshake so that XLL commands can lazily start it.
//
// Cross-thread communication uses two asymmetric mechanisms:
//
//   Excel thread → UI thread : wxTheApp->CallAfter()   (built into wx)
//   UI thread → Excel thread : MessageWindow            (hidden HWND task queue)
//
// Only one MessageWindow is needed (on Excel's thread).  There is no
// dispatcher on the UI thread — wx already provides CallAfter for that.
//
// The StatusFrame (non-modal) and GreetingDialog (modal) are always created
// and manipulated on the UI thread.  The frame pointer is only accessed on
// the UI thread, so no mutex is needed.
//
// Modal dialogs use run_on_ui_thread() which posts a lambda via CallAfter,
// blocks the calling thread with a promise/future, and pumps Win32 messages
// to avoid cross-thread SendMessage deadlocks.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// wx headers must come before LibXLL / ExcelSDK headers — see the full
// explanation below the #define.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <future>
#include <iostream>
#include <string>

#include <wx/wx.h>

#include <Excel/Automation.hpp>
#include <Win32/MessageWindow.hpp>
#include <Win32/UiThread.hpp>

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
// run_on_ui_thread
//
// Posts a callable to the wx event loop via CallAfter and blocks the calling
// thread until it completes, pumping Win32 messages to avoid deadlock from
// cross-thread SendMessage calls (e.g. EnableWindow in NativeOwnerModal).
//
// Returns the value produced by the callable.
// ============================================================================

template <typename F>
auto run_on_ui_thread(F&& fn) -> std::invoke_result_t<F>
{
    using R = std::invoke_result_t<F>;
    std::promise<R> promise;
    auto future = promise.get_future();

    wxTheApp->CallAfter([&promise, &fn]() {
        try {
            if constexpr (std::is_void_v<R>) {
                fn();
                promise.set_value();
            } else {
                promise.set_value(fn());
            }
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    });

    while (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLINPUT);
    }

    return future.get();
}

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

        m_input->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { EndModal(wxID_OK); });
    }

    [[nodiscard]] std::string GetInput() const
    {
        return m_input->GetValue().utf8_string();
    }

private:
    wxTextCtrl* m_input = nullptr;
};

namespace {

// ============================================================================
// COM automation helpers
// ============================================================================

bool write_to_active_cell(const std::wstring& text)
{
    xll::excel::Dispatch app;
    if (FAILED(xll::excel::get_active_object(L"Excel.Application", app)))
        return false;

    xll::excel::Dispatch activeCell;
    if (FAILED(app.property_dispatch(L"ActiveCell", activeCell)))
        return false;

    return SUCCEEDED(activeCell.put(L"Value", text));
}

// ============================================================================
// Application singleton
//
// Owns the Excel-thread dispatcher (UI→Excel task queue), the UI thread
// lifecycle, and the StatusFrame pointer.
//
// Cross-thread communication:
//   Excel → UI : wxTheApp->CallAfter()   (built into wx, no wrapper needed)
//   UI → Excel : m_excelDispatcher        (MessageWindow on Excel's thread)
//
// m_frame is only accessed on the UI thread — no mutex required.
// ============================================================================

class StatusFrame;

class AddIn
{
public:
    static AddIn& instance()
    {
        static AddIn s;
        return s;
    }

    bool initialize()
    {
        if (!m_excelDispatcher.create()) return false;
        return m_uiThread.start([](xll::win32::UiThread& ut) {
            ui_thread_body(ut);
        });
    }

    void shutdown();

    [[nodiscard]] xll::win32::MessageWindow& excel_dispatcher()
    {
        return m_excelDispatcher;
    }

    // UI-thread only — no synchronization needed.
    void set_frame(StatusFrame* f) { m_frame = f; }
    [[nodiscard]] StatusFrame* frame() const { return m_frame; }

    void show_frame(HWND excelHwnd);

private:
    AddIn() = default;

    static void ui_thread_body(xll::win32::UiThread& uiThread);

    xll::win32::MessageWindow  m_excelDispatcher;
    xll::win32::UiThread       m_uiThread;
    StatusFrame*               m_frame = nullptr;   // UI-thread only
};

// ============================================================================
// wx bootstrap
// ============================================================================

class XllStatusApp : public wxApp
{
public:
    bool OnInit() override
    {
        // Prevent wx from exiting the event loop when the last top-level
        // window is destroyed.  Without this, a modal dialog (WX.GREETING)
        // opened before any persistent frame causes wxTheApp->OnRun() to
        // return as soon as the dialog is dismissed, tearing down the wx
        // runtime and leaving wxTheApp in an invalid state.
        SetExitOnFrameDelete(false);
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(XllStatusApp);

// ============================================================================
// Non-modal frame (WX.STATUS)
// ============================================================================

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
    }

    void BringUpNearExcel()
    {
        HWND excelHwnd = xll::get_hwnd();

        if (excelHwnd && ::IsWindow(excelHwnd)) {
            RECT rc{};
            if (::GetWindowRect(excelHwnd, &rc)) {
                SetPosition(wxPoint(rc.right + 60, rc.top + 60));
            }
        }

        Show();
        Raise();
    }

    void UpdateResult(bool ok)
    {
        m_label->SetLabel(ok
            ? "Greeting written to active cell."
            : "Failed to write to active cell.");
        Layout();
    }

private:
    void SendGreetingRequest()
    {
        const std::wstring greeting = L"Hello, " + m_name->GetValue().ToStdWstring() + L"!";

        // Post the COM write to Excel's thread via the MessageWindow.
        // The result callback uses CallAfter to return to the UI thread.
        if (!AddIn::instance().excel_dispatcher().post([text = greeting]() {
                const bool ok = write_to_active_cell(text);
                if (wxTheApp) {
                    wxTheApp->CallAfter([ok]() {
                        StatusFrame* f = AddIn::instance().frame();
                        if (f) f->UpdateResult(ok);
                    });
                }
            })) {
            m_label->SetLabel("Failed to post request to Excel thread.");
            Layout();
            return;
        }

        m_label->SetLabel("Writing greeting...");
        Layout();
    }

    wxStaticText* m_label     = nullptr;
    wxTextCtrl*   m_name      = nullptr;
    HWND          m_excelHwnd = nullptr;
};

// ============================================================================
// AddIn — out-of-line definitions (depend on complete StatusFrame)
// ============================================================================

void AddIn::shutdown()
{
    if (m_uiThread.is_running() && wxTheApp) {
        wxTheApp->CallAfter([this]() {
            if (m_frame) {
                m_frame->Destroy();
                m_frame = nullptr;
            }
            if (wxTheApp) wxTheApp->ExitMainLoop();
        });
    }

    m_uiThread.join();
    m_excelDispatcher.shutdown();
}

void AddIn::ui_thread_body(xll::win32::UiThread& uiThread)
{

    // wx must be initialized on this thread — wxEntryStart() stamps it
    // as the wx "main" thread, and OnRun() asserts wxThread::IsMain().
    int argc = 0;
    char** argv = nullptr;

    wxApp::SetInstance(new XllStatusApp());

    if (!wxEntryStart(argc, argv)) {
        uiThread.signal_failed();
        return;
    }

    if (!wxTheApp || !wxTheApp->CallOnInit()) {
        wxEntryCleanup();
        uiThread.signal_failed();
        return;
    }

    // Unblock ensure_ui_started() — wx is ready, CallAfter() is safe.
    uiThread.signal_ready();

    wxTheApp->OnRun();

    // Event loop exited (shutdown() called ExitMainLoop).
    AddIn::instance().set_frame(nullptr);

    if (wxTheApp) wxTheApp->OnExit();
    wxEntryCleanup();
}

void AddIn::show_frame(HWND excelHwnd)
{
    if (!wxTheApp) return;

    wxTheApp->CallAfter([this, excelHwnd]() {
        if (!m_frame) {
            m_frame = new StatusFrame(excelHwnd);
        }
        m_frame->BringUpNearExcel();
    });
}

} // anonymous namespace

// ============================================================================
// Lifecycle
// ============================================================================

auto onOpen =
    xll::OnOpen()
    | xll::Before([] {
        AddIn::instance().initialize();
    });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] {
        AddIn::instance().shutdown();
    });
XLL_REGISTER(onClose);

// ============================================================================
// Command: WX.GREETING  (modal dialog)
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
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    auto input = run_on_ui_thread([excelHwnd]() -> std::string {
        GreetingDialog dlg(nullptr);
        NativeOwnerModal modal(static_cast<HWND>(dlg.GetHWND()), excelHwnd);
        return (dlg.ShowModal() == wxID_OK) ? dlg.GetInput() : std::string{};
    });

    if (!input.empty()) {
        xll::alert(xll::String("Hello, " + input + "!"));
    }
}

// ============================================================================
// Command: WX.STATUS  (non-modal frame)
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

XLL_FUNCTION void XLLAPI ShowWxStatus()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    AddIn::instance().show_frame(excelHwnd);
}

