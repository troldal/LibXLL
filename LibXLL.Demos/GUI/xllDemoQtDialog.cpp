// xllDemoQtDialog.cpp
//
// Demonstrates hosting Qt widgets inside an XLL add-in using a
// signals/slots architecture (palacaze/sigslot) for cross-thread
// communication.  This is the Qt counterpart of xllDemoWxDialog.cpp.
//
// Architecture
// ------------
// Qt's QApplication must live on the thread that creates it (Qt's "GUI
// thread").  A lightweight UiThread manages that thread's lifecycle with a
// ready/failed handshake.
//
// Cross-thread communication is implemented via sigslot signals with
// thread-marshaling centralized in the wiring.  Call sites just emit signals.
//
//   Excel thread → UI thread : signals wired through QMetaObject::invokeMethod
//   UI thread → Excel thread : signals wired through MessageWindow::post()
//
// Signal groups:
//   msg::ToUi         — emitted on Excel thread, delivered on UI thread
//   msg::ToExcel      — emitted on UI thread, delivered on Excel thread
//   msg::ToUiResponse — emitted on Excel thread (in response), delivered on UI thread
//
// Modal dialogs bypass signals and use run_on_qt_thread() which posts a
// callable via QMetaObject::invokeMethod with QueuedConnection, blocks the
// Excel thread with a promise/future, and pumps Win32 messages to avoid
// cross-thread SendMessage deadlocks.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// Qt headers must come before LibXLL / ExcelSDK headers because
// xlcall.hpp contains a bare #include <windows.h> that, without
// WIN32_LEAN_AND_MEAN, drags in winsock.h and causes conflicts.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <atomic>
#include <future>
#include <string>

#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <sigslot/signal.hpp>

#include <Excel/Automation.hpp>
#include <Win32/MessageWindow.hpp>
#include <Win32/UiThread.hpp>

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

// ============================================================================
// post_to_qt
//
// Thread-safe helper: posts a std::function to the Qt event loop via
// QMetaObject::invokeMethod with QueuedConnection.  The callable runs on
// the Qt GUI thread.  This is the Qt equivalent of wxTheApp->CallAfter().
// ============================================================================

inline void post_to_qt(std::function<void()> fn)
{
    // QTimer::singleShot with 0ms and a context object (qApp) ensures the
    // lambda runs on the context object's thread — the Qt GUI thread.
    QTimer::singleShot(0, qApp, std::move(fn));
}

// ============================================================================
// run_on_qt_thread
//
// Posts a callable to the Qt event loop and blocks the calling thread until
// it completes, pumping Win32 messages to avoid deadlock from cross-thread
// SendMessage calls (e.g. EnableWindow in NativeOwnerModal).
//
// Returns the value produced by the callable.
// ============================================================================

template <typename F>
auto run_on_qt_thread(F&& fn) -> std::invoke_result_t<F>
{
    using R = std::invoke_result_t<F>;
    std::promise<R> promise;
    auto future = promise.get_future();

    post_to_qt([&promise, &fn]() {
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
//   Qt : reinterpret_cast<HWND>(widget->winId())
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
    NativeOwnerSetup m_setup;
    HWND             m_owner;
};

// ============================================================================
// Modal dialog (QT.GREETING)
// ============================================================================

class GreetingDialog : public QDialog
{
public:
    explicit GreetingDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Qt inside an XLL");
        resize(360, 150);

        auto* vbox = new QVBoxLayout(this);
        auto* hbox = new QHBoxLayout();
        auto* btns = new QHBoxLayout();

        auto* label = new QLabel("Enter your name:");
        m_input     = new QLineEdit();
        m_input->setMinimumWidth(200);

        hbox->addWidget(label);
        hbox->addWidget(m_input, 1);

        auto* btnOk     = new QPushButton("OK");
        auto* btnCancel = new QPushButton("Cancel");
        btns->addStretch();
        btns->addWidget(btnOk);
        btns->addWidget(btnCancel);
        btns->addStretch();

        vbox->addStretch();
        vbox->addLayout(hbox);
        vbox->addSpacing(10);
        vbox->addLayout(btns);
        vbox->addStretch();

        connect(btnOk,     &QPushButton::clicked, this, &QDialog::accept);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_input, &QLineEdit::returnPressed, this, &QDialog::accept);
    }

    [[nodiscard]] std::string GetInput() const
    {
        return m_input->text().toStdString();
    }

private:
    QLineEdit* m_input = nullptr;
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
// Non-modal frame (QT.STATUS)
//
// The frame is created on the UI thread and only accessed there.
// It communicates with Excel by emitting signals (no direct post).
// ============================================================================

class StatusFrame : public QWidget
{
public:
    StatusFrame(HWND /*excelHwnd*/, msg::ToExcel& toExcel)
        : QWidget(nullptr),
          m_toExcel(toExcel)
    {
        setWindowTitle("XLL Status (Qt)");
        resize(340, 170);

        auto* vbox = new QVBoxLayout(this);
        auto* hbox = new QHBoxLayout();

        auto* nameLabel = new QLabel("Your name:");
        m_name = new QLineEdit("World");
        m_name->setMinimumWidth(160);

        hbox->addWidget(nameLabel);
        hbox->addWidget(m_name, 1);

        m_label = new QLabel("Type a name and press the button\n"
                             "to greet the active Excel cell.");
        m_label->setAlignment(Qt::AlignCenter);

        auto* btn = new QPushButton("Greet Active Cell");

        vbox->addStretch();
        vbox->addLayout(hbox);
        vbox->addWidget(m_label);
        vbox->addWidget(btn, 0, Qt::AlignCenter);
        vbox->addStretch();

        connect(btn, &QPushButton::clicked, this, &StatusFrame::SendGreetingRequest);
        connect(m_name, &QLineEdit::returnPressed, this, &StatusFrame::SendGreetingRequest);
    }

    void BringUpNearExcel(HWND excelHwnd)
    {
        if (excelHwnd && ::IsWindow(excelHwnd)) {
            RECT rc{};
            if (::GetWindowRect(excelHwnd, &rc)) {
                move(rc.left + 60, rc.top + 60);
            }
        }

        show();
        raise();
        activateWindow();
    }

    void UpdateResult(bool ok)
    {
        m_label->setText(ok
            ? "Greeting written to active cell."
            : "Failed to write to active cell.");
    }

protected:
    // Hide instead of destroy when the user closes the window.
    void closeEvent(QCloseEvent* event) override
    {
        hide();
        event->ignore();
    }

private:
    void SendGreetingRequest()
    {
        std::wstring greeting =
            L"Hello, " + m_name->text().toStdWString() + L"!";

        // Just emit the signal — the wiring takes care of thread marshaling.
        m_toExcel.write_to_cell(greeting);

        m_label->setText("Writing greeting...");
    }

    QLabel*       m_label     = nullptr;
    QLineEdit*    m_name      = nullptr;
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
        // QApplication must be created on the thread that will run the event
        // loop — this becomes Qt's "GUI thread".
        int argc = 0;
        char* argv[] = { nullptr };
        QApplication app(argc, argv);

        // Prevent Qt from quitting when the last window is closed.
        // Without this, closing the StatusFrame (or dismissing a modal
        // dialog before any persistent window exists) causes
        // QApplication::exec() to return, tearing down the Qt runtime.
        app.setQuitOnLastWindowClosed(false);

        // Qt is ready — QApplication exists and the event loop is about to
        // start.  Unblock the caller of UiThread::start().
        uiThread.signal_ready();

        // Run the Qt event loop.  This blocks until QApplication::quit() is
        // called (from the shutdown signal handler).
        app.exec();

        // Event loop exited — clean up the frame on the Qt thread.
        auto& addIn = AddIn::instance();
        if (addIn.frame()) {
            delete addIn.frame();
            addIn.set_frame(nullptr);
        }
    }

    // -----------------------------------------------------------------
    // Wiring — centralized thread marshaling, done once during init
    // -----------------------------------------------------------------

    void wire_signals()
    {
        // Excel → UI: show_status
        m_toUi.show_status.connect([this](HWND excelHwnd) {
            post_to_qt([this, excelHwnd]() {
                if (!m_frame) {
                    m_frame = new StatusFrame(excelHwnd, m_toExcel);
                }
                m_frame->BringUpNearExcel(excelHwnd);
            });
        });

        // Excel → UI: shutdown
        m_toUi.shutdown.connect([this]() {
            if (m_uiThread.is_running()) {
                post_to_qt([this]() {
                    if (m_frame) {
                        delete m_frame;
                        m_frame = nullptr;
                    }
                    QApplication::quit();
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
            post_to_qt([this, ok]() {
                if (m_frame) m_frame->UpdateResult(ok);
            });
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

auto qtOnOpen =
    xll::OnOpen()
    | xll::Before([] {
        AddIn::instance().initialize();
    });
XLL_REGISTER(qtOnOpen);

auto qtOnClose =
    xll::OnClose()
    | xll::Before([] {
        AddIn::instance().shutdown();
    });
XLL_REGISTER(qtOnClose);

// ============================================================================
// Command: QT.GREETING  (modal dialog — exception to the signal pattern)
// ============================================================================

auto qtGreetingCmd =
    xll::Command("QT.GREETING")
    | xll::Procedure("ShowQtGreeting")
    | xll::Category("Qt Examples")
    | xll::Description(
        "Shows a modal Qt dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(qtGreetingCmd);

XLL_FUNCTION void XLLAPI ShowQtGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    auto input = run_on_qt_thread([excelHwnd]() -> std::string {
        GreetingDialog dlg;
        dlg.show();
        NativeOwnerModal modal(
            reinterpret_cast<HWND>(dlg.winId()), excelHwnd);
        dlg.exec();
        return (dlg.result() == QDialog::Accepted)
            ? dlg.GetInput() : std::string{};
    });

    if (!input.empty()) {
        xll::alert(xll::String("Hello, " + input + "!"));
    }
}

// ============================================================================
// Command: QT.STATUS  (non-modal frame — uses signals, no threading code)
// ============================================================================

auto qtStatusCmd =
    xll::Command("QT.STATUS")
    | xll::Procedure("ShowQtStatus")
    | xll::Category("Qt Examples")
    | xll::Description(
        "Shows a non-modal Qt frame on a dedicated UI thread. "
        "Excel remains interactive while the frame is open. "
        "If the frame is already open, it is brought to the front.");
XLL_REGISTER(qtStatusCmd);

XLL_FUNCTION void XLLAPI ShowQtStatus()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    // Just emit the signal — the wiring takes care of thread marshaling.
    AddIn::instance().to_ui().show_status(excelHwnd);
}





