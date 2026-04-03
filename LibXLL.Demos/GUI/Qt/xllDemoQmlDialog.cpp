// xllDemoQmlDialog.cpp
//
// Demonstrates hosting QML-defined GUI inside an XLL add-in using a
// signals/slots architecture (palacaze/sigslot) for cross-thread
// communication.  This is the QML counterpart of xllDemoQtDialog.cpp.
//
// The GUI is defined in QML (embedded as raw string literals) and rendered
// with the FluentWinUI3 style for a modern Windows 11 look.
//
// Architecture
// ------------
// Same as xllDemoQtDialog.cpp:
//
// - A dedicated UI thread runs QGuiApplication + QQmlEngine.
// - Cross-thread communication via sigslot signals with marshaling.
// - A QmlBridge QObject is exposed to QML as a "bridge" context property
//   for C++ ↔ QML communication.
// - Modal dialogs use run_on_qt_thread() to bypass signals.
//
// Key difference from xllDemoQtDialog.cpp: all UI is defined in QML, not
// in C++ widgets.  The QML sources are embedded as raw string literals and
// loaded via QQmlComponent::setData().
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
#include <memory>
#include <string>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(qml_demo);

#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QWindow>

#include <sigslot/signal.hpp>

#include <Excel/Automation.hpp>
#include <Win32/MessageWindow.hpp>
#include <Win32/UiThread.hpp>

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

// ============================================================================
// QML sources — loaded from embedded CMakeRC resources at runtime.
// ============================================================================

static QByteArray loadQml(const char* path)
{
    auto fs   = cmrc::qml_demo::get_filesystem();
    auto file = fs.open(path);
    return QByteArray(file.begin(), static_cast<qsizetype>(file.size()));
}

// ============================================================================
// post_to_qt
//
// Thread-safe helper: posts a std::function to the Qt event loop.
// The callable runs on the Qt GUI thread.
// ============================================================================

inline void post_to_qt(std::function<void()> fn)
{
    QTimer::singleShot(0, qApp, std::move(fn));
}

// ============================================================================
// run_on_qt_thread
//
// Posts a callable to the Qt event loop and blocks the calling thread until
// it completes, pumping Win32 messages to avoid deadlock from cross-thread
// SendMessage calls (e.g. EnableWindow in NativeOwnerModal).
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
// QmlBridge
//
// QObject exposed to QML as the "bridge" context property.
//
// QML reads bridge.statusText (bound to a Label) and calls
// bridge.writeToCell(text) when the user clicks the button.
//
// The C++ AddIn connects the writeToCellRequested Qt signal to the
// sigslot write_to_cell signal, and calls onCellWriteResult() when the
// Excel thread reports back.
//
// Note: Q_OBJECT must not be in an anonymous namespace (MOC limitation).
// ============================================================================

class QmlBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText WRITE setStatusText
               NOTIFY statusTextChanged)

public:
    explicit QmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    [[nodiscard]] QString statusText() const { return m_statusText; }

    void setStatusText(const QString& text)
    {
        if (m_statusText != text) {
            m_statusText = text;
            emit statusTextChanged();
        }
    }

signals:
    void statusTextChanged();
    void writeToCellRequested(const QString& greeting);

public slots:
    void writeToCell(const QString& greeting)
    {
        emit writeToCellRequested(greeting);
        setStatusText(QStringLiteral("Writing greeting..."));
    }

    void onCellWriteResult(bool ok)
    {
        setStatusText(ok
            ? QStringLiteral("Greeting written to active cell.")
            : QStringLiteral("Failed to write to active cell."));
    }

private:
    QString m_statusText{
        QStringLiteral("Type a name and press the button\n"
                       "to greet the active Excel cell.")};
};

// ============================================================================

namespace {

// ============================================================================
// Signal groups
// ============================================================================

namespace msg {

    struct ToUi {
        sigslot::signal<HWND>  show_status;
        sigslot::signal<>      shutdown;
    };

    struct ToExcel {
        sigslot::signal<std::wstring>  write_to_cell;
    };

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
// Application singleton
//
// Same role as in xllDemoQtDialog.cpp: owns the signal groups, the
// Excel-thread dispatcher (MessageWindow), the UI thread lifecycle,
// and pointers to the QML engine / bridge / status window.
//
// The QML engine, bridge, and status window live on the UI thread
// and are only accessed there — no mutex required.
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
        m_toUi.shutdown();
        m_uiThread.join();
        m_excelDispatcher.shutdown();
    }

    msg::ToUi& to_ui() { return m_toUi; }

    // UI-thread accessors (set from ui_thread_body, read from wire_signals
    // after the UiThread ready-handshake guarantees visibility).
    void set_engine(QQmlEngine* e)           { m_engine = e; }
    [[nodiscard]] QQmlEngine* engine() const { return m_engine; }

    void set_bridge(QmlBridge* b)              { m_bridge = b; }
    [[nodiscard]] QmlBridge* bridge() const    { return m_bridge; }

    void set_status_window(QObject* w)              { m_statusWindow = w; }
    [[nodiscard]] QObject* status_window() const    { return m_statusWindow; }

private:
    AddIn() = default;

    static void ui_thread_body(xll::win32::UiThread& uiThread)
    {
        int argc = 0;
        char* argv[] = { nullptr };
        QGuiApplication app(argc, argv);

        // FluentWinUI3: modern Windows 11 look (Qt 6.8+).
        // Must be set before any QML component is loaded.
        QQuickStyle::setStyle("FluentWinUI3");

        app.setQuitOnLastWindowClosed(false);

        QQmlEngine engine;
        QmlBridge  bridge;
        engine.rootContext()->setContextProperty("bridge", &bridge);

        auto& addIn = AddIn::instance();
        addIn.set_engine(&engine);
        addIn.set_bridge(&bridge);

        uiThread.signal_ready();
        app.exec();

        // Cleanup — defensive; the shutdown handler should have done this.
        if (addIn.status_window()) {
            delete addIn.status_window();
            addIn.set_status_window(nullptr);
        }

        addIn.set_engine(nullptr);
        addIn.set_bridge(nullptr);
    }

    // -----------------------------------------------------------------
    // Wiring — centralized thread marshaling, done once during init
    // -----------------------------------------------------------------

    void wire_signals()
    {
        // Excel → UI: show_status
        m_toUi.show_status.connect([this](HWND excelHwnd) {
            post_to_qt([this, excelHwnd]() {
                if (!m_statusWindow) {
                    QQmlComponent component(m_engine);
                    component.setData(loadQml("QML/StatusWindow.qml"), QUrl());
                    if (component.isError()) {
                        for (const auto& e : component.errors())
                            qWarning() << e.toString();
                        return;
                    }
                    m_statusWindow = component.create();
                }

                auto* window = qobject_cast<QWindow*>(m_statusWindow);
                if (window) {
                    if (excelHwnd && ::IsWindow(excelHwnd)) {
                        RECT rc{};
                        if (::GetWindowRect(excelHwnd, &rc))
                            window->setPosition(rc.left + 60, rc.top + 60);
                    }
                    window->show();
                    window->raise();
                    window->requestActivate();
                }
            });
        });

        // Excel → UI: shutdown
        m_toUi.shutdown.connect([this]() {
            if (m_uiThread.is_running()) {
                post_to_qt([this]() {
                    if (m_statusWindow) {
                        delete m_statusWindow;
                        m_statusWindow = nullptr;
                    }
                    QGuiApplication::quit();
                });
            }
        });

        // Bridge Qt signal → sigslot (both fire on UI thread)
        QObject::connect(m_bridge, &QmlBridge::writeToCellRequested,
            m_bridge, [this](const QString& greeting) {
                m_toExcel.write_to_cell(greeting.toStdWString());
            });

        // UI → Excel: write_to_cell
        m_toExcel.write_to_cell.connect([this](const std::wstring& text) {
            auto _ = m_excelDispatcher.post([this, text]() {
                bool ok = write_to_active_cell(text);
                m_toUiResponse.cell_write_result(ok);
            });
        });

        // Excel → UI (response): cell_write_result
        m_toUiResponse.cell_write_result.connect([this](bool ok) {
            post_to_qt([this, ok]() {
                if (m_bridge) m_bridge->onCellWriteResult(ok);
            });
        });
    }

    xll::win32::MessageWindow  m_excelDispatcher;
    xll::win32::UiThread       m_uiThread;

    QQmlEngine*  m_engine       = nullptr;   // UI-thread only
    QmlBridge*   m_bridge       = nullptr;   // UI-thread only
    QObject*     m_statusWindow = nullptr;   // UI-thread only

    msg::ToUi          m_toUi;
    msg::ToExcel       m_toExcel;
    msg::ToUiResponse  m_toUiResponse;
};

} // anonymous namespace

// ============================================================================
// Lifecycle
// ============================================================================

auto qmlOnOpen =
    xll::OnOpen()
    | xll::Before([] {
        AddIn::instance().initialize();
    });
XLL_REGISTER(qmlOnOpen);

auto qmlOnClose =
    xll::OnClose()
    | xll::Before([] {
        AddIn::instance().shutdown();
    });
XLL_REGISTER(qmlOnClose);

// ============================================================================
// Command: QML.GREETING  (modal dialog — exception to the signal pattern)
// ============================================================================

auto qmlGreetingCmd =
    xll::Command("QML.GREETING")
    | xll::Procedure("ShowQmlGreeting")
    | xll::Category("QML Examples")
    | xll::Description(
        "Shows a modal QML dialog (FluentWinUI3 style) parented to the "
        "Excel window, then greets the user with xll::alert.");
XLL_REGISTER(qmlGreetingCmd);

XLL_FUNCTION void XLLAPI ShowQmlGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    auto input = run_on_qt_thread([excelHwnd]() -> std::string {
        auto* engine = AddIn::instance().engine();
        if (!engine) return {};

        QQmlComponent component(engine);
        component.setData(loadQml("QML/GreetingDialog.qml"), QUrl());
        if (component.isError()) return {};

        std::unique_ptr<QObject> obj(component.create());
        auto* window = qobject_cast<QWindow*>(obj.get());
        if (!window) return {};

        window->show();
        NativeOwnerModal modal(
            reinterpret_cast<HWND>(window->winId()), excelHwnd);

        // Run a nested event loop until the QML window closes itself.
        QEventLoop loop;
        QObject::connect(window, &QWindow::visibleChanged,
            &loop, [&loop](bool visible) {
                if (!visible) loop.quit();
            });
        loop.exec();

        bool accepted = obj->property("accepted").toBool();
        return accepted
            ? obj->property("inputText").toString().toStdString()
            : std::string{};
    });

    if (!input.empty()) {
        xll::alert(xll::String("Hello, " + input + "!"));
    }
}

// ============================================================================
// Command: QML.STATUS  (non-modal frame — uses signals, no threading code)
// ============================================================================

auto qmlStatusCmd =
    xll::Command("QML.STATUS")
    | xll::Procedure("ShowQmlStatus")
    | xll::Category("QML Examples")
    | xll::Description(
        "Shows a non-modal QML window (FluentWinUI3 style) on a dedicated "
        "UI thread.  Excel remains interactive while the window is open. "
        "If the window is already open, it is brought to the front.");
XLL_REGISTER(qmlStatusCmd);

XLL_FUNCTION void XLLAPI ShowQmlStatus()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    AddIn::instance().to_ui().show_status(excelHwnd);
}

// ============================================================================
// MOC-generated meta-object code for QmlBridge (Q_OBJECT in a .cpp file).
// CMake AUTOMOC detects this include and generates the file automatically.
// ============================================================================

#include "xllDemoQmlDialog.moc"

