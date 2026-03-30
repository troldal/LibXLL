// xllDemoWxDialog.cpp
//
// Demonstrates hosting wxWidgets windows inside an XLL add-in using a
// signals/slots architecture (palacaze/sigslot) for cross-thread
// communication.
//
// Architecture
// ------------
// wxWidgets needs a dedicated "main" thread (wxThread::IsMain() is stamped at
// wxEntryStart time).  A lightweight UiThread manages that thread's lifecycle
// with a ready/failed handshake.
//
// Cross-thread communication is implemented via sigslot signals with
// thread-marshaling centralized in the wiring.  Call sites just emit signals.
//
//   Excel thread → UI thread : signals wired through wxTheApp->CallAfter()
//   UI thread → Excel thread : signals wired through MessageWindow::post()
//
// Signal groups:
//   msg::ToUi         — emitted on Excel thread, delivered on UI thread
//   msg::ToExcel      — emitted on UI thread, delivered on Excel thread
//   msg::ToUiResponse — emitted on Excel thread (in response), delivered on UI thread
//
// Modal dialogs bypass signals and use run_on_ui_thread() which posts a
// callable via CallAfter, blocks the Excel thread with a promise/future,
// and pumps Win32 messages to avoid cross-thread SendMessage deadlocks.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// wx headers must come before LibXLL / ExcelSDK headers — see the full
// explanation below the #define.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <future>
#include <string>

#include <wx/wx.h>

#include <sigslot/signal.hpp>

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
// Signal groups
//
// Signals are grouped by direction.  Each signal is emitted on one thread
// and delivered on the other via thread-marshaling wired during xlAutoOpen.
// ============================================================================

namespace msg {

    // Emitted on Excel thread, delivered on UI thread.
    struct ToUi {
        sigslot::signal<HWND>  show_status;      // fire-and-forget
        sigslot::signal<>      shutdown;
    };

    // Emitted on UI thread, delivered on Excel thread.
    struct ToExcel {
        sigslot::signal<std::wstring>  write_to_cell;
    };

    // Emitted on Excel thread, delivered on UI thread (responses).
    struct ToUiResponse {
        sigslot::signal<bool>  cell_write_result;
    };

} // namespace msg

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
//
// The frame is created on the UI thread and only accessed there.
// It communicates with Excel by emitting signals (no direct post/CallAfter).
// ============================================================================

class StatusFrame : public wxFrame
{
public:
    StatusFrame(HWND excelHwnd, msg::ToExcel& toExcel)
        : wxFrame(nullptr, wxID_ANY, "XLL Status",
                  wxDefaultPosition, wxSize(340, 170)),
          m_excelHwnd(excelHwnd),
          m_toExcel(toExcel)
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
        std::wstring greeting = L"Hello, " + m_name->GetValue().ToStdWstring() + L"!";

        // Just emit the signal — the wiring takes care of thread marshaling.
        m_toExcel.write_to_cell(greeting);

        m_label->SetLabel("Writing greeting...");
        Layout();
    }

    wxStaticText* m_label     = nullptr;
    wxTextCtrl*   m_name      = nullptr;
    HWND          m_excelHwnd = nullptr;
    msg::ToExcel& m_toExcel;
};

// ============================================================================
// Application singleton
//
// Owns the signal groups, the Excel-thread dispatcher (MessageWindow), the
// UI thread lifecycle, and the StatusFrame pointer.
//
// Signal wiring is done once in wire_signals(), called from initialize()
// after the UI thread is ready.
//
// m_frame is only accessed on the UI thread — no mutex required.
// ============================================================================

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
        if (!m_uiThread.start([](xll::win32::UiThread& ut) {
            ui_thread_body(ut);
        })) return false;

        wire_signals();
        return true;
    }

    void shutdown()
    {
        // Emit shutdown → marshaled to UI thread → exits event loop.
        m_toUi.shutdown();

        m_uiThread.join();
        m_excelDispatcher.shutdown();
    }

    // Signal groups — accessible for command handlers.
    msg::ToUi& to_ui() { return m_toUi; }

    // UI-thread only — no synchronization needed.
    void set_frame(StatusFrame* f) { m_frame = f; }
    [[nodiscard]] StatusFrame* frame() const { return m_frame; }

private:
    AddIn() = default;

    static void ui_thread_body(xll::win32::UiThread& uiThread)
    {
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

        uiThread.signal_ready();

        wxTheApp->OnRun();

        // Event loop exited (shutdown signal called ExitMainLoop).
        AddIn::instance().set_frame(nullptr);

        if (wxTheApp) wxTheApp->OnExit();
        wxEntryCleanup();
    }

    // -----------------------------------------------------------------
    // Wiring — centralized thread marshaling, done once during init
    // -----------------------------------------------------------------

    void wire_signals()
    {
        // Excel → UI: show_status
        m_toUi.show_status.connect([this](HWND excelHwnd) {
            wxTheApp->CallAfter([this, excelHwnd]() {
                if (!m_frame) {
                    m_frame = new StatusFrame(excelHwnd, m_toExcel);
                }
                m_frame->BringUpNearExcel();
            });
        });

        // Excel → UI: shutdown
        m_toUi.shutdown.connect([this]() {
            if (m_uiThread.is_running() && wxTheApp) {
                wxTheApp->CallAfter([this]() {
                    if (m_frame) {
                        m_frame->Destroy();
                        m_frame = nullptr;
                    }
                    if (wxTheApp) wxTheApp->ExitMainLoop();
                });
            }
        });

        // UI → Excel: write_to_cell
        m_toExcel.write_to_cell.connect([this](const std::wstring& text) {
            auto _ = m_excelDispatcher.post([this, text]() {
                bool ok = write_to_active_cell(text);
                // Response signal → marshaled back to UI thread.
                m_toUiResponse.cell_write_result(ok);
            });
        });

        // Excel → UI (response): cell_write_result
        m_toUiResponse.cell_write_result.connect([this](bool ok) {
            if (wxTheApp) {
                wxTheApp->CallAfter([this, ok]() {
                    if (m_frame) m_frame->UpdateResult(ok);
                });
            }
        });
    }

    xll::win32::MessageWindow  m_excelDispatcher;
    xll::win32::UiThread       m_uiThread;
    StatusFrame*               m_frame = nullptr;   // UI-thread only

    msg::ToUi          m_toUi;
    msg::ToExcel       m_toExcel;
    msg::ToUiResponse  m_toUiResponse;
};

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
// Command: WX.GREETING  (modal dialog — exception to the signal pattern)
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
// Command: WX.STATUS  (non-modal frame — uses signals, no threading code)
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

    // Just emit the signal — the wiring takes care of thread marshaling.
    AddIn::instance().to_ui().show_status(excelHwnd);
}

