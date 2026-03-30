// xllDemoFltkDialog.cpp
//
// Demonstrates hosting FLTK windows inside an XLL add-in using a
// signals/slots architecture (palacaze/sigslot) for cross-thread
// communication.  This is the FLTK counterpart of xllDemoWxDialog.cpp.
//
// Architecture
// ------------
// FLTK is initialized on a dedicated thread.  Fl::lock() enables FLTK's
// multi-threading support, and Fl::awake() provides thread-safe cross-thread
// posting — the FLTK equivalent of wxTheApp->CallAfter().
//
// Cross-thread communication is implemented via sigslot signals with
// thread-marshaling centralized in the wiring.  Call sites just emit signals.
//
//   Excel thread → UI thread : signals wired through post_to_fltk()
//   UI thread → Excel thread : signals wired through MessageWindow::post()
//
// Signal groups:
//   msg::ToUi         — emitted on Excel thread, delivered on UI thread
//   msg::ToExcel      — emitted on UI thread, delivered on Excel thread
//   msg::ToUiResponse — emitted on Excel thread (in response), delivered on UI thread
//
// Modal dialogs bypass signals and use run_on_fltk_thread() which posts a
// callable via Fl::awake(), blocks the calling thread with a promise/future,
// and pumps Win32 messages to avoid cross-thread SendMessage deadlocks.
//
// Unlike wxWidgets, FLTK does not need a wxApp subclass, wxEntryStart, or
// wxEntryCleanup.  Calling Fl::lock() once on the UI thread is sufficient
// to enable multi-threaded operation.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// FLTK headers must come before LibXLL / ExcelSDK headers because
// xlcall.hpp contains a bare #include <windows.h> that, without
// WIN32_LEAN_AND_MEAN, drags in winsock.h and causes conflicts.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <atomic>
#include <future>
#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/platform.H>          // fl_xid()

#include <sigslot/signal.hpp>

#include <Excel/Automation.hpp>
#include <Win32/MessageWindow.hpp>
#include <Win32/UiThread.hpp>

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

// ============================================================================
// post_to_fltk
//
// Thread-safe helper: posts a std::function to the FLTK event loop via
// Fl::awake().  The callable runs on the FLTK thread during Fl::wait().
// This is the FLTK equivalent of wxTheApp->CallAfter().
// ============================================================================

inline void post_to_fltk(std::function<void()> fn)
{
    auto* task = new std::function<void()>(std::move(fn));
    Fl::awake([](void* data) {
        auto* t = static_cast<std::function<void()>*>(data);
        (*t)();
        delete t;
    }, task);
}

// ============================================================================
// run_on_fltk_thread
//
// Posts a callable to the FLTK event loop via Fl::awake() and blocks the
// calling thread until it completes, pumping Win32 messages to avoid
// deadlock from cross-thread SendMessage calls (e.g. EnableWindow in
// NativeOwnerModal).
//
// Returns the value produced by the callable.
// ============================================================================

template <typename F>
auto run_on_fltk_thread(F&& fn) -> std::invoke_result_t<F>
{
    using R = std::invoke_result_t<F>;
    std::promise<R> promise;
    auto future = promise.get_future();

    post_to_fltk([&promise, &fn]() {
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
    NativeOwnerSetup m_setup;
    HWND             m_owner;
};

// ============================================================================
// GreetingDialog  (modal — used by FLTK.GREETING)
// ============================================================================

class GreetingDialog : public Fl_Window
{
public:
    GreetingDialog()
        : Fl_Window(360, 120, "FLTK inside an XLL")
    {
        begin();
        new Fl_Box(10, 10, 120, 25, "Enter your name:");
        m_input  = new Fl_Input(130, 10, 210, 25);
        m_ok     = new Fl_Button(170, 70, 80, 30, "OK");
        m_cancel = new Fl_Button(260, 70, 80, 30, "Cancel");
        end();

        m_ok->callback(on_ok, this);
        m_cancel->callback(on_cancel, this);
        m_input->when(FL_WHEN_ENTER_KEY);
        m_input->callback(on_ok, this);
        set_modal();
    }

    [[nodiscard]] std::string run(HWND excelHwnd)
    {
        show();
        NativeOwnerModal modal(fl_xid(this), excelHwnd);
        while (shown()) Fl::wait();
        return m_confirmed ? m_input->value() : std::string{};
    }

private:
    static void on_ok(Fl_Widget*, void* data)
    {
        auto* self = static_cast<GreetingDialog*>(data);
        self->m_confirmed = true;
        self->hide();
    }

    static void on_cancel(Fl_Widget*, void* data)
    {
        auto* self = static_cast<GreetingDialog*>(data);
        self->m_confirmed = false;
        self->hide();
    }

    Fl_Input*  m_input   = nullptr;
    Fl_Button* m_ok      = nullptr;
    Fl_Button* m_cancel  = nullptr;
    bool       m_confirmed = false;
};

// ============================================================================
// UTF-8 → wide-string helper (for COM automation)
// ============================================================================

static std::wstring utf8_to_wide(const char* utf8)
{
    if (!utf8 || !*utf8) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.data(), len);
    return result;
}

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
// Non-modal frame (FLTK.STATUS)
//
// FLTK's default close behaviour calls hide(), which is exactly what we
// want — the frame is hidden but not destroyed, and can be shown again.
// The manual event loop (Fl::wait in a while-loop) keeps running even
// when no windows are visible.
//
// The frame communicates with Excel by emitting signals (no direct post).
// ============================================================================

class StatusFrame : public Fl_Window
{
public:
    explicit StatusFrame(HWND /*excelHwnd*/, msg::ToExcel& toExcel)
        : Fl_Window(340, 160, "XLL Status (FLTK)"),
          m_toExcel(toExcel)
    {
        begin();
        new Fl_Box(10, 15, 110, 25, "Your name:");
        m_name   = new Fl_Input(120, 15, 200, 25);
        m_name->value("World");
        m_label  = new Fl_Box(10, 55, 320, 40,
                              "Type a name and press the button\n"
                              "to greet the active Excel cell.");
        m_label->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
        auto* btn = new Fl_Button(95, 110, 150, 30, "Greet Active Cell");
        end();

        btn->callback(on_greet, this);
        m_name->when(FL_WHEN_ENTER_KEY);
        m_name->callback(on_greet, this);
    }

    void BringUpNearExcel()
    {
        HWND excelHwnd = xll::get_hwnd();
        if (excelHwnd && ::IsWindow(excelHwnd)) {
            RECT rc{};
            if (::GetWindowRect(excelHwnd, &rc)) {
                position(rc.right + 60, rc.top + 60);
            }
        }
        show();
    }

    void UpdateResult(bool ok)
    {
        copy_label_text(ok ? "Greeting written to active cell."
                           : "Failed to write to active cell.");
    }

private:
    void copy_label_text(const char* text)
    {
        m_label->copy_label(text);
        m_label->redraw();
    }

    static void on_greet(Fl_Widget*, void* data)
    {
        static_cast<StatusFrame*>(data)->SendGreetingRequest();
    }

    void SendGreetingRequest()
    {
        std::wstring greeting =
            L"Hello, " + utf8_to_wide(m_name->value()) + L"!";

        // Just emit the signal — the wiring takes care of thread marshaling.
        m_toExcel.write_to_cell(greeting);

        copy_label_text("Writing greeting...");
    }

    Fl_Input*     m_name  = nullptr;
    Fl_Box*       m_label = nullptr;
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

    [[nodiscard]] bool exit_requested() const noexcept { return m_exitLoop.load(); }

private:
    AddIn() = default;

    static void ui_thread_body(xll::win32::UiThread& uiThread)
    {
        // Fl::lock() enables FLTK's multi-threading support.  After this call,
        // other threads can use Fl::awake() to post work to this thread.
        Fl::lock();

        // Unblock start() — FLTK is ready, Fl::awake() is safe.
        uiThread.signal_ready();

        // Manual event loop: Fl::wait(timeout) processes events and awake
        // callbacks.  Unlike Fl::run(), this keeps looping even when no windows
        // are visible, allowing the frame to be re-shown later.
        auto& app = AddIn::instance();
        while (!app.exit_requested()) {
            Fl::wait(0.1);
        }

        // Clean up the frame on the FLTK thread before exiting.
        if (app.frame()) {
            delete app.frame();
            app.set_frame(nullptr);
        }

        Fl::unlock();
    }

    // -----------------------------------------------------------------
    // Wiring — centralized thread marshaling, done once during init
    // -----------------------------------------------------------------

    void wire_signals()
    {
        // Excel → UI: show_status
        m_toUi.show_status.connect([this](HWND excelHwnd) {
            post_to_fltk([this, excelHwnd]() {
                if (!m_frame) {
                    m_frame = new StatusFrame(excelHwnd, m_toExcel);
                }
                m_frame->BringUpNearExcel();
            });
        });

        // Excel → UI: shutdown
        m_toUi.shutdown.connect([this]() {
            m_exitLoop.store(true);
            // Wake up Fl::wait() so it sees the exit flag promptly.
            post_to_fltk([]() {});
        });

        // UI → Excel: write_to_cell
        m_toExcel.write_to_cell.connect([this](const std::wstring& text) {
            (void)m_excelDispatcher.post([this, text]() {
                bool ok = write_to_active_cell(text);
                // Response signal → marshaled back to UI thread.
                m_toUiResponse.cell_write_result(ok);
            });
        });

        // Excel → UI (response): cell_write_result
        m_toUiResponse.cell_write_result.connect([this](bool ok) {
            post_to_fltk([this, ok]() {
                if (m_frame) m_frame->UpdateResult(ok);
            });
        });
    }

    xll::win32::MessageWindow  m_excelDispatcher;
    xll::win32::UiThread       m_uiThread;
    StatusFrame*               m_frame = nullptr;        // UI-thread only
    std::atomic<bool>          m_exitLoop{ false };

    msg::ToUi          m_toUi;
    msg::ToExcel       m_toExcel;
    msg::ToUiResponse  m_toUiResponse;
};

} // anonymous namespace

// ============================================================================
// Lifecycle
// ============================================================================

auto fltkOnOpen =
    xll::OnOpen()
    | xll::Before([] {
        AddIn::instance().initialize();
    });
XLL_REGISTER(fltkOnOpen);

auto fltkOnClose =
    xll::OnClose()
    | xll::Before([] {
        AddIn::instance().shutdown();
    });
XLL_REGISTER(fltkOnClose);

// ============================================================================
// Command: FLTK.GREETING  (modal dialog — exception to the signal pattern)
// ============================================================================

auto fltkGreetingCmd =
    xll::Command("FLTK.GREETING")
    | xll::Procedure("ShowFltkGreeting")
    | xll::Category("FLTK Examples")
    | xll::Description(
        "Shows a modal FLTK dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(fltkGreetingCmd);

XLL_FUNCTION void XLLAPI ShowFltkGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    auto input = run_on_fltk_thread([excelHwnd]() -> std::string {
        GreetingDialog dlg;
        return dlg.run(excelHwnd);
    });

    if (!input.empty()) {
        xll::alert(xll::String("Hello, " + input + "!"));
    }
}

// ============================================================================
// Command: FLTK.STATUS  (non-modal frame — uses signals, no threading code)
// ============================================================================

auto fltkStatusCmd =
    xll::Command("FLTK.STATUS")
    | xll::Procedure("ShowFltkStatus")
    | xll::Category("FLTK Examples")
    | xll::Description(
        "Shows a non-modal FLTK frame on a dedicated UI thread. "
        "Excel remains interactive while the frame is open. "
        "If the frame is already open, it is brought to the front.");
XLL_REGISTER(fltkStatusCmd);

XLL_FUNCTION void XLLAPI ShowFltkStatus()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    // Just emit the signal — the wiring takes care of thread marshaling.
    AddIn::instance().to_ui().show_status(excelHwnd);
}


