// xllDemoImDialog.cpp
//
// Demonstrates hosting Dear ImGui (Win32 + DirectX 11) windows inside an
// XLL add-in, implementing the same functionality as xllDemoWxDialog.cpp.
//
// Architecture
// ------------
// IM.GREETING — modal dialog, runs on the Excel thread.  Has its own local
//               Win32 message loop; blocks the caller until OK or Cancel.
//
// IM.STATUS   — non-modal window, runs on a dedicated UI thread (same pattern
//               as StatusFrame in xllDemoWxDialog.cpp).  Excel remains fully
//               interactive.  Cross-thread communication uses sigslot signals
//               with thread-marshaling centralised in AddIn::wire_signals():
//
//   Excel thread → UI thread : signals wired through m_uiDispatcher.post()
//   UI thread → Excel thread : signals wired through m_excelDispatcher.post()
//
// Signal groups (identical names to xllDemoWxDialog.cpp):
//   msg::ToUi         — emitted on Excel thread, delivered on UI thread
//   msg::ToExcel      — emitted on UI thread,   delivered on Excel thread
//   msg::ToUiResponse — emitted on Excel thread (response), delivered on UI thread

#define WIN32_LEAN_AND_MEAN
#include "ImGuiGreetingDialog.hpp"

#include <cstring>

#include <sigslot/signal.hpp>

#include <Excel/Automation.hpp>
#include <Win32/MessageWindow.hpp>
#include <Win32/UiThread.hpp>

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>        // pulls in xll::get_hwnd(), xll::alert(), xll::String

thread_local ImGuiContext*   GImGui = NULL;

namespace {

// ============================================================================
// Signal groups (identical layout to xllDemoWxDialog.cpp)
//
// Emitted on Excel thread, delivered on UI thread.
struct ToUiSignals {
    sigslot::signal<HWND>  show_status;
    sigslot::signal<>      shutdown;
};

// Emitted on UI thread, delivered on Excel thread.
struct ToExcelSignals {
    sigslot::signal<std::wstring>  write_to_cell;
};

// Emitted on Excel thread (in response), delivered on UI thread.
struct ToUiResponseSignals {
    sigslot::signal<bool>  cell_write_result;
};

namespace msg {
    using ToUi         = ToUiSignals;
    using ToExcel      = ToExcelSignals;
    using ToUiResponse = ToUiResponseSignals;
}

// ============================================================================
// COM helper — write a string value to the active Excel cell.
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
// ImGuiStatusWindow — non-modal window (IM.STATUS)
//
// Mirrors StatusFrame from xllDemoWxDialog.cpp.
//
// Runs on the dedicated UI thread managed by AddIn.  NOT a singleton; AddIn
// owns the instance (m_window, UI-thread only).  Cross-thread communication
// uses msg::ToExcel signals wired in AddIn::wire_signals().
// ============================================================================

class ImGuiStatusWindow
{
public:
    /// Constructor — called on the UI thread.
    explicit ImGuiStatusWindow(msg::ToExcel& toExcel)
        : m_toExcel(toExcel)
    {
        std::memcpy(m_nameBuf.data(), "World", 6);
    }

    ~ImGuiStatusWindow() { destroy(); }

    ImGuiStatusWindow(const ImGuiStatusWindow&)            = delete;
    ImGuiStatusWindow& operator=(const ImGuiStatusWindow&) = delete;

    void showOrRaise(HWND excelHwnd)
    {
        if (!m_hwnd && !create()) return;

        if (excelHwnd && IsWindow(excelHwnd))
        {
            RECT rc{};
            if (GetWindowRect(excelHwnd, &rc))
                SetWindowPos(m_hwnd, nullptr,
                             rc.left + 60, rc.top + 60,
                             0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }

    /// Called on the UI thread (via AddIn's response signal) after a cell
    /// write completes on the Excel thread.
    void updateResult(bool ok)
    {
        m_status = ok ? "Greeting written to active cell."
                      : "Failed to write to active cell.";
    }

    void destroy()
    {
        if (!m_hwnd) return;

        KillTimer(m_hwnd, kTimerId);

        if (m_ctx)
        {
            ImGui::SetCurrentContext(m_ctx);
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(m_ctx);
            m_ctx = nullptr;
        }

        m_d3d.destroy();
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

private:
    static constexpr UINT_PTR kTimerId = 0xCBA3;
    static constexpr wchar_t  kClass[] = L"ImGuiStatusWnd";

    msg::ToExcel& m_toExcel;   // signal group — emit write_to_cell from UI thread

    HWND          m_hwnd          = nullptr;
    D3D11Block    m_d3d;
    ImGuiContext* m_ctx           = nullptr;
    int           m_width         = 510;
    int           m_height        = 255;
    bool          m_resizePending = false;

    std::array<char, 256> m_nameBuf{};
    std::string           m_status{
        "Type a name and press the button\n"
        "to greet the active Excel cell." };
    bool         m_greetPending   = false;
    std::wstring m_pendingGreeting;

    [[nodiscard]] bool create()
    {
        ensureClass();

        m_hwnd = CreateWindowExW(
            0, kClass, L"XLL Status",
            WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
            nullptr, nullptr,
            dll_handle(), this);

        if (!m_hwnd) return false;

        ApplyWindowTheme(m_hwnd);

        if (!m_d3d.create(m_hwnd))
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            return false;
        }

        IMGUI_CHECKVERSION();
        m_ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_ctx);

        ImGuiIO& io  = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename  = nullptr;

        applyStyle(m_hwnd);

        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(m_d3d.device, m_d3d.context);

        SetTimer(m_hwnd, kTimerId, 16, nullptr);
        return true;
    }

    void renderFrame()
    {
        if (m_resizePending && m_d3d.swapChain)
        {
            m_resizePending = false;
            m_d3d.resize(static_cast<UINT>(m_width), static_cast<UINT>(m_height));
        }

        if (!m_ctx || !m_d3d.rtv) return;

        ImGui::SetCurrentContext(m_ctx);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        constexpr ImGuiWindowFlags kFlags =
            ImGuiWindowFlags_NoTitleBar          |
            ImGuiWindowFlags_NoResize            |
            ImGuiWindowFlags_NoMove              |
            ImGuiWindowFlags_NoScrollbar         |
            ImGuiWindowFlags_NoCollapse          |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##Status", nullptr, kFlags);

        const ImVec2 avail   = ImGui::GetContentRegionAvail();
        const float  lineH   = ImGui::GetFrameHeight();
        const float  spacing = ImGui::GetStyle().ItemSpacing.y;
        const float  statusH = ImGui::CalcTextSize(m_status.c_str(), nullptr, false, avail.x).y;
        const float  totalH  = lineH + spacing + statusH + spacing + lineH;

        ImGui::SetCursorPosY((avail.y - totalH) * 0.5f);

        ImGui::Text("Your name:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(avail.x - ImGui::GetCursorPosX());
        const bool enter = ImGui::InputText(
            "##Name", m_nameBuf.data(), m_nameBuf.size(),
            ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::TextWrapped("%s", m_status.c_str());

        const float btnW =
            ImGui::CalcTextSize("Greet Active Cell").x
            + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetCursorPosX((avail.x - btnW) * 0.5f);

        if (HighlightedExcelButton("Greet Active Cell") || enter)
        {
            const std::string name(m_nameBuf.data());
            m_pendingGreeting = L"Hello, "
                + std::wstring(name.begin(), name.end()) + L"!";
            m_greetPending = true;
            m_status = "Writing greeting...";
        }

        ImGui::End();
        ImGui::Render();

        constexpr float kClear[] = { 0.1216f, 0.1216f, 0.1216f, 1.0f };
        m_d3d.context->OMSetRenderTargets(1, &m_d3d.rtv, nullptr);
        m_d3d.context->ClearRenderTargetView(m_d3d.rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_d3d.swapChain->Present(1, 0);

        // Deferred signal emit — after Present() to avoid re-entering the
        // render path.  The wired slot posts write_to_active_cell() to the
        // Excel thread via m_excelDispatcher; the result returns via
        // AddIn::wire_signals() → m_toUiResponse → updateResult().
        if (m_greetPending)
        {
            m_greetPending = false;
            m_toExcel.write_to_cell(m_pendingGreeting);
        }
    }

    static void ensureClass()
    {
        WNDCLASSEXW wc   = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &WndProc;
        wc.hInstance     = dll_handle();
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kClass;
        RegisterClassExW(&wc);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        auto* self = reinterpret_cast<ImGuiStatusWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (self && self->m_ctx)
        {
            ImGui::SetCurrentContext(self->m_ctx);
            if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
                return TRUE;
        }

        if (self)
        {
            switch (msg)
            {
            case WM_ERASEBKGND: return 1;

            case WM_TIMER:
                if (wParam == kTimerId)
                {
                    self->renderFrame();
                    ValidateRect(hwnd, nullptr);
                    return 0;
                }
                break;

            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                self->renderFrame();
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_SIZE:
                if (self->m_d3d.swapChain && wParam != SIZE_MINIMIZED)
                {
                    const int w = static_cast<int>(LOWORD(lParam));
                    const int h = static_cast<int>(HIWORD(lParam));
                    if (w > 0 && h > 0)
                    {
                        self->m_width         = w;
                        self->m_height        = h;
                        self->m_resizePending = true;
                    }
                }
                return 0;

            case WM_CLOSE:
                // Hide rather than destroy — the window can be shown again.
                ShowWindow(hwnd, SW_HIDE);
                return 0;

            default: break;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

// ============================================================================
// AddIn singleton
//
// Owns the signal groups, the Excel-thread dispatcher (MessageWindow), the
// UI thread lifecycle, and the ImGuiStatusWindow pointer.
//
// Signal wiring is done once in wire_signals(), called from initialize()
// after the UI thread is ready.
//
// m_window is only accessed on the UI thread — no mutex required.
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
        // Emit shutdown → marshaled to UI thread → posts WM_QUIT → loop exits.
        m_toUi.shutdown();
        m_uiThread.join();
        m_excelDispatcher.shutdown();
    }

    msg::ToUi& to_ui() { return m_toUi; }

    // UI-thread only — called from ui_thread_body after the loop exits.
    void set_window(ImGuiStatusWindow* w) { m_window = w; }

private:
    AddIn() = default;

    // -----------------------------------------------------------------
    // UI thread body — creates the UI-thread MessageWindow, signals
    // readiness, then runs a standard Win32 message loop.
    // -----------------------------------------------------------------

    static void ui_thread_body(xll::win32::UiThread& uiThread)
    {
        // Create the hidden dispatcher window on THIS thread so that
        // m_uiDispatcher.post() routes work here.
        if (!AddIn::instance().m_uiDispatcher.create())
        {
            uiThread.signal_failed();
            return;
        }

        uiThread.signal_ready();

        // Standard Win32 message loop.  Processes:
        //   WM_TIMER   — ImGuiStatusWindow rendering (~60 fps)
        //   WM_APP+    — m_uiDispatcher work items (signal delivery, shutdown)
        MSG msg;
        while (::GetMessage(&msg, nullptr, 0, 0) > 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        // Message loop exited (PostQuitMessage called by shutdown handler).
        AddIn::instance().set_window(nullptr);
        AddIn::instance().m_uiDispatcher.shutdown();
    }

    // -----------------------------------------------------------------
    // Wiring — centralised thread marshaling, done once during init.
    // -----------------------------------------------------------------

    void wire_signals()
    {
        // Excel → UI: show_status
        m_toUi.show_status.connect([this](HWND excelHwnd) {
            auto _ = m_uiDispatcher.post([this, excelHwnd]() {
                if (!m_window)
                    m_window = new ImGuiStatusWindow(m_toExcel);
                m_window->showOrRaise(excelHwnd);
            });
        });

        // Excel → UI: shutdown
        m_toUi.shutdown.connect([this]() {
            if (m_uiThread.is_running()) {
                auto _ = m_uiDispatcher.post([this]() {
                    if (m_window) {
                        m_window->destroy();
                        delete m_window;
                        m_window = nullptr;
                    }
                    ::PostQuitMessage(0);
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
            auto _ = m_uiDispatcher.post([this, ok]() {
                if (m_window) m_window->updateResult(ok);
            });
        });
    }

    xll::win32::MessageWindow  m_excelDispatcher;
    xll::win32::MessageWindow  m_uiDispatcher;    // created on the UI thread
    xll::win32::UiThread       m_uiThread;
    ImGuiStatusWindow*         m_window = nullptr; // UI-thread only

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
// Command: IM.GREETING  (modal dialog)
// ============================================================================

auto imGreetingCmd =
    xll::Command("IM.GREETING")
    | xll::Procedure("ShowImGreeting")
    | xll::Category("ImGui Examples")
    | xll::Description(
        "Shows a modal Dear ImGui dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(imGreetingCmd);

XLL_FUNCTION void XLLAPI ShowImGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    const auto input = ImGuiGreetingDialog::show(excelHwnd);
    if (!input.empty())
        xll::alert(xll::String("Hello, " + input + "!"));
}

// ============================================================================
// Command: IM.STATUS  (non-modal window — dedicated UI thread)
// ============================================================================

auto imStatusCmd =
    xll::Command("IM.STATUS")
    | xll::Procedure("ShowImStatus")
    | xll::Category("ImGui Examples")
    | xll::Description(
        "Shows a non-modal Dear ImGui window on a dedicated UI thread. "
        "Excel remains interactive while the window is open. "
        "If the window is already open, it is brought to the front.");
XLL_REGISTER(imStatusCmd);

XLL_FUNCTION void XLLAPI ShowImStatus()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    AddIn::instance().to_ui().show_status(excelHwnd);
}
