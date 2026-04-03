// ---------------------------------------------------------------------------
// QtQuickTaskPane.hpp — Qt Quick (QML) content implementation for TaskPaneControl.
//
// This class satisfies the Content concept required by TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Destructor handles framework-appropriate cleanup
//
// It also provides two static helpers for process-wide framework management:
//
//   - QtQuickTaskPane::initialize() — lazy one-time QGuiApplication bootstrap
//   - QtQuickTaskPane::shutdown()   — QGuiApplication teardown at DLL unload
//
// QT QUICK EMBEDDING STRATEGY
// ----------------------------
// Unlike the Qt Widgets variant (QtTaskPane.hpp) which creates a QWidget and
// reparents it via QWindow::setParent(), the Qt Quick variant uses a
// QQuickView — a QWindow subclass that hosts a QML scene graph.
//
// Steps:
//   1. Wrap the ActiveX container HWND as a foreign QWindow via
//      QWindow::fromWinId().
//   2. Create a QQuickView and set its parent to the foreign QWindow.
//      Because QQuickView IS-A QWindow, QWindow::setParent() works directly
//      — no intermediate QWidget or winId() call is needed.
//   3. Load inline QML via QQuickView::setSource() from a data URL.
//   4. Resize the QQuickView to fill the container.
//
// The QML scene graph renders via the RHI backend (Direct3D 11 on Windows by
// default in Qt 6).  Because we are hosted inside Excel's STA thread, the
// scene graph must run in the "basic" render-loop mode (single-threaded),
// which is the correct choice when no QGuiApplication::exec() loop is running.
// We force this via the QSG_RENDER_LOOP=basic environment variable.
//
// DPI SCALING
// -----------
// Qt 6 enables automatic high-DPI scaling by default.  Because the host
// application (Excel) manages DPI and passes pixel coordinates through
// SetObjectRects / MoveWindow, Qt must NOT apply its own scaling on top.
// We disable it via qputenv("QT_ENABLE_HIGHDPI_SCALING", "0") before
// creating QGuiApplication.
//
// MOC / AUTOMOC
// -------------
// The QML scene needs to call back into C++ (to show a native message box).
// This is done via a small QObject-derived helper (QtQuickTaskPaneHelper)
// exposed as a context property.  The Q_OBJECT macro requires MOC processing;
// because this class is defined in a header, CMake's AUTOMOC automatically
// generates and compiles the moc output (moc_QtQuickTaskPane.cpp) when
// AUTOMOC is enabled on the target.
//
// THREADING
// ---------
// All Qt Quick operations happen on Excel's STA thread (the GUI thread).
// No extra threads or message pumps are involved.  QGuiApplication::exec()
// is never called — Excel's own Win32 message pump dispatches messages to
// the Qt child HWNDs and drives the QML render loop.
// ---------------------------------------------------------------------------

#pragma once

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QWindow>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <iostream>
#include <vector>

// ---------------------------------------------------------------------------
// QtQuickTaskPaneHelper — QObject exposed to QML as a context property.
//
// Provides a single Q_INVOKABLE method that shows a native MessageBox.
// Defined at namespace scope (not nested inside QtQuickTaskPane) because
// MOC does not support Q_OBJECT in nested classes.
// ---------------------------------------------------------------------------

class QtQuickTaskPaneHelper : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE void showMessage()  // NOLINT
    {
        MessageBoxW(nullptr,
                    L"Hello from the Task Pane!",
                    L"Task Pane",
                    MB_OK | MB_ICONINFORMATION);
    }
};

// ---------------------------------------------------------------------------
// QtQuickTaskPane
// ---------------------------------------------------------------------------

class QtQuickTaskPane
{
public:
    // -----------------------------------------------------------------------
    // initialize()
    //
    // Lazily creates a QGuiApplication the first time it is called.
    // Safe to call multiple times — subsequent calls are no-ops.
    //
    // QGuiApplication (not QApplication) is sufficient for Qt Quick because
    // Qt Quick does not use QWidget.  Using QGuiApplication avoids pulling
    // in the Qt Widgets module.
    //
    // Qt 6's automatic high-DPI scaling and the threaded scene-graph render
    // loop are both disabled before construction so that:
    //   - pixel coordinates from the host (SetObjectRects) are used as-is;
    //   - the scene graph renders on the STA thread (no secondary GL thread).
    //
    // Returns true on success.
    // -----------------------------------------------------------------------
    static bool initialize()
    {
        if (qApp) return true;            // already initialised

        // Disable Qt's automatic high-DPI scaling.  The host application
        // (Excel) manages DPI and passes pixel coordinates via
        // SetObjectRects / MoveWindow.  If Qt applies its own scaling on
        // top, the widget geometry becomes too large and appears as a stray
        // window.
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

        // Force the "basic" (single-threaded) render loop.  The default
        // "threaded" mode creates a secondary thread that calls
        // makeCurrent / swapBuffers, which deadlocks when there is no
        // running QGuiApplication::exec() event loop.
        qputenv("QSG_RENDER_LOOP", "basic");

        // QGuiApplication requires argc by reference; use a static so it
        // outlives the constructor call.
        static int    argc = 0;
        static char*  argv[] = { nullptr };
        new QGuiApplication(argc, argv);   // sets qApp global
        return qApp != nullptr;
    }

    // -----------------------------------------------------------------------
    // shutdown()
    //
    // Tears down the QGuiApplication.  Should be called from OnDisconnection
    // or DllMain on DLL_PROCESS_DETACH.
    //
    // WHY WE DRAIN LIVE INSTANCES FIRST
    // -----------------------------------
    // Excel may defer IOleObject::Close / IOleInPlaceObject::InPlaceDeactivate
    // until after OnDisconnection returns.  In that case the QQuickView (and
    // its D3D11 render context) is still alive when we reach delete qApp.
    // QGuiApplicationPrivate's destructor iterates Qt's internal window list
    // and tears down the D3D11 platform integration; finding a live QWindow
    // with an active render context at that point causes a memmove crash
    // inside QWindowsContext / QWindowsDirect3DContext cleanup.
    //
    // Fix: call destroyQtObjects() on every surviving instance before
    // touching qApp.  destroyQtObjects() hides the view, releases all GPU
    // resources, unparents it, deletes all Qt objects, and nulls the
    // pointers.  When TaskPaneControl's destructor eventually fires, the
    // second call to destroyQtObjects() is a complete no-op.
    // -----------------------------------------------------------------------
    static void shutdown()
    {
        if (!qApp) return;

        // Snapshot the list: destroyQtObjects() will erase from it via the
        // normal destructor path if the TaskPaneControl is destroyed during
        // the loop (unlikely on the STA thread, but be safe).
        const auto snapshot = s_liveInstances;
        for (QtQuickTaskPane* inst : snapshot)
            inst->destroyQtObjects();
        s_liveInstances.clear();

        // Flush any remaining deferred deletes queued during cleanup above.
        qApp->processEvents();
        delete qApp;                       // clears the qApp global
    }

    // -----------------------------------------------------------------------
    // Constructor — creates a Qt Quick scene inside parentHwnd.
    //
    // Steps:
    //   1. Register this instance in s_liveInstances so shutdown() can find
    //      it if Excel defers InPlaceDeactivate past OnDisconnection.
    //   2. Ensure QGuiApplication is initialised.
    //   3. Wrap the ActiveX container HWND as a foreign QWindow.
    //   4. Create a QQuickView and parent it to the foreign QWindow.
    //   5. Expose a C++ helper object as a context property so QML can
    //      call back into native code (MessageBox).
    //   6. Load inline QML that defines a centred Button.
    //   7. Set geometry to fill the container and show the view.
    //
    // Layout:
    //   parentHwnd  (Win32, owned by TaskPaneControl)
    //     └─ m_view  (QQuickView, parented via QWindow::setParent)
    //          └─ QML scene
    //               └─ Rectangle (anchors.fill: parent)
    //                    └─ Button ("Click me!", anchors.centerIn: parent)
    //
    // The QML Button's onClicked handler calls nativeHelper.showMessage(),
    // which is a Q_INVOKABLE method on the QtQuickTaskPaneHelper context
    // property that shows a native Win32 MessageBox.
    // -----------------------------------------------------------------------
    QtQuickTaskPane(HWND parentHwnd, int w, int h)
        : m_parent(parentHwnd)
    {
        s_liveInstances.push_back(this);

        if (!initialize())
        {
            std::cerr << "[xlCOM]   Qt Quick initialisation failed\n";
            return;
        }

        // 1. Wrap the ActiveX container HWND as a foreign QWindow.
        m_parentWindow = QWindow::fromWinId(reinterpret_cast<WId>(parentHwnd));

        // 2. Create the QQuickView and parent it to the foreign window.
        m_view = new QQuickView();
        m_view->setParent(m_parentWindow);

        // 3. Fill the container.
        m_view->setGeometry(0, 0, w, h);

        // 4. Resize the root QML item automatically when the view resizes.
        m_view->setResizeMode(QQuickView::SizeRootObjectToView);

        // 5. Expose the native helper to QML.
        m_helper = new QtQuickTaskPaneHelper();
        m_view->rootContext()->setContextProperty(
            QStringLiteral("nativeHelper"), m_helper);

        // 6. Load inline QML.
        //    The QML defines a Rectangle that fills the view, with a
        //    QtQuick.Controls Button centred in it.  Clicking the button
        //    calls nativeHelper.showMessage() — the C++ helper exposed in
        //    step 5.
        //
        //    palette.window is used as the background colour so the pane
        //    matches the system theme (light / dark).
        static const QUrl qmlSource = []() {
            static const QByteArray qml = R"QML(
                import QtQuick
                import QtQuick.Controls

                Rectangle {
                    anchors.fill: parent
                    color: palette.window

                    Button {
                        text: "Click me!"
                        anchors.centerIn: parent
                        highlighted: true
                        onClicked: nativeHelper.showMessage()
                    }
                }
            )QML";
            QUrl url;
            url.setScheme(QStringLiteral("data"));
            url.setPath(QStringLiteral("text/plain;base64,")
                        + QString::fromLatin1(qml.toBase64()));
            return url;
        }();

        m_view->setSource(qmlSource);

        if (m_view->status() == QQuickView::Error)
        {
            for (const auto& error : m_view->errors())
                std::cerr << "[xlCOM]   QML error: "
                          << error.toString().toStdString() << '\n';
            return;
        }

        m_view->show();

        std::cerr << "[xlCOM]   Qt Quick content created\n";
    }

    // -----------------------------------------------------------------------
    // resize — called by TaskPaneControl::SetObjectRects() when the task pane
    // is resized.  Uses QWindow::setGeometry() so the scene graph and root
    // QML object know about the new size.
    // -----------------------------------------------------------------------
    void resize(int w, int h)
    {
        if (m_view)
            m_view->setGeometry(0, 0, w, h);
    }

    // -----------------------------------------------------------------------
    // Destructor — unregisters the instance and delegates all Qt-specific
    // cleanup to destroyQtObjects().
    //
    // Normal path (Excel calls InPlaceDeactivate before shutdown()):
    //   destroyQtObjects() runs while qApp is alive; all Qt objects are
    //   destroyed in the correct order before shutdown() calls delete qApp.
    //
    // Emergency path (Excel defers InPlaceDeactivate past OnDisconnection):
    //   shutdown() has already called destroyQtObjects() on this instance,
    //   setting all Qt pointers to null.  The call here is a complete no-op.
    //
    // The container HWND is destroyed by TaskPaneControl::deactivate() after
    // this destructor returns — the Qt child window is already gone by then.
    // -----------------------------------------------------------------------
    ~QtQuickTaskPane()
    {
        // Remove from the live list before cleanup so a re-entrant
        // shutdown() call cannot process this instance a second time.
        s_liveInstances.erase(
            std::remove(s_liveInstances.begin(), s_liveInstances.end(), this),
            s_liveInstances.end());

        destroyQtObjects();
    }

    // Non-copyable, non-movable (Qt object ownership).
    QtQuickTaskPane(const QtQuickTaskPane&)            = delete;
    QtQuickTaskPane& operator=(const QtQuickTaskPane&) = delete;
    QtQuickTaskPane(QtQuickTaskPane&&)                 = delete;
    QtQuickTaskPane& operator=(QtQuickTaskPane&&)      = delete;

private:
    HWND        m_parent       = nullptr;   // the container HWND (not owned)

    // Foreign QWindow wrapping m_parent.  Non-owning — deleting it does NOT
    // destroy the underlying HWND.
    QWindow*    m_parentWindow = nullptr;

    // The QQuickView that renders the QML scene.  Parented to m_parentWindow.
    QQuickView* m_view         = nullptr;

    // Helper object exposed to QML for calling native MessageBox.
    QtQuickTaskPaneHelper* m_helper = nullptr;

    // -----------------------------------------------------------------------
    // destroyQtObjects — releases all Qt-owned resources for this instance.
    //
    // Idempotent: every Qt pointer is set to null on exit.  A second call
    // (destructor after shutdown() already ran) sees only null pointers and
    // returns immediately.
    //
    // WHY hide() / releaseResources() ARE NOT CALLED
    // ------------------------------------------------
    // Excel's HWND shutdown cascade destroys m_view's native HWND before
    // OnDisconnection returns and posts WM_DESTROY / WM_NCDESTROY to the
    // message queue.  releaseResources() drives the Qt event loop internally.
    // If it drains those queued messages, Qt's WM_DESTROY handler begins
    // tearing down the D3D11 render context at the same time that
    // releaseResources() is tearing it down — double-free → RtlIsZeroMemory
    // crash in the DXGI/D3D11 cleanup path.
    //
    // The safe alternative is to drain pending messages with processEvents()
    // BEFORE entering any Qt destructor, then rely on QQuickWindow's own
    // destructor to call QSGDefaultRenderContext::invalidate() — which is the
    // normal, well-tested D3D11 teardown path.
    //
    // DELETION ORDER
    // --------------
    //   1. processEvents() — drain WM_DESTROY / WM_NCDESTROY before any
    //                        Qt destructor runs.
    //   2. delete m_view   — QQuickWindow::~QQuickWindow releases the D3D11
    //                        render context (swap chain, device, buffers).
    //                        Safe when the underlying HWND is already dead:
    //                        QWindowsWindow::destroy() calls DestroyWindow()
    //                        which returns FALSE for an invalid handle, and
    //                        the COM Release() calls don't touch the HWND.
    //                        Deleting the child before the QObject parent
    //                        also prevents m_parentWindow's dtor from
    //                        auto-deleting m_view a second time.
    //   3. delete m_helper — plain QObject, no native resources.
    //   4. delete m_parentWindow — foreign-window wrapper; does NOT destroy
    //                        the host HWND (Qt does not own it).
    //   5. processEvents() — flush any deleteLater() objects queued by the
    //                        destructors above.
    // -----------------------------------------------------------------------
    void destroyQtObjects()
    {
        // Drain pending messages (WM_DESTROY etc.) before Qt destructors run.
        if (qApp)
            qApp->processEvents();

        delete m_view;         m_view         = nullptr;
        delete m_helper;       m_helper       = nullptr;
        delete m_parentWindow; m_parentWindow = nullptr;

        if (qApp)
            qApp->processEvents();
    }

    // All QtQuickTaskPane instances currently alive.  Populated in the
    // constructor, erased in the destructor.  shutdown() uses this list to
    // call destroyQtObjects() on any instance that Excel has not yet
    // deactivated, ensuring no live QWindow is left when delete qApp runs.
    static inline std::vector<QtQuickTaskPane*> s_liveInstances;
};
