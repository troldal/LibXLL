// ---------------------------------------------------------------------------
// ImGuiDemoWindow2.hpp — modeless 800 x 1200 window that hosts the Dear ImGui
// built-in demo (ImGui::ShowDemoWindow).
//
// Unlike ImGuiDemoWindow (modal, blocking local message loop), this version
// creates the window and returns immediately.  Rendering is driven by a
// WM_TIMER dispatched through Excel's own STA message pump — Excel stays
// fully interactive while the demo window is open.
//
// Lifecycle:
//   create(owner)  — allocates and shows the window; returns a raw pointer
//                     that the caller stores in a static/global.
//   showOrRaise()  — brings a hidden window back to the foreground.
//   destroy()      — tears down D3D11 + ImGui and destroys the Win32 window.
//   WM_CLOSE       — hides the window (toggle pattern); caller can re-show
//                     with showOrRaise() or destroy() on shutdown.
// ---------------------------------------------------------------------------

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "SetStyle.hpp"
#include "Utils/IsDarkMode.hpp"
#include "ImGuiRenderGuard.hpp"

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(foo);

class ImGuiDemoWindow2
{
public:
    // -----------------------------------------------------------------------
    // Factory — creates the window, shows it, and returns immediately.
    // The caller owns the returned pointer and must eventually call destroy()
    // or delete it.  Returns nullptr on failure.
    // -----------------------------------------------------------------------
    [[nodiscard]] static ImGuiDemoWindow2* create(HWND owner)
    {
        auto* self = new (std::nothrow) ImGuiDemoWindow2;
        if (!self) return nullptr;

        if (!self->init(owner))
        {
            delete self;
            return nullptr;
        }

        ShowWindow(self->m_hwnd, SW_SHOW);
        UpdateWindow(self->m_hwnd);
        return self;
    }

    // -----------------------------------------------------------------------
    // showOrRaise — if the window is hidden (user closed with X), show it
    // again and bring it to the foreground.
    // -----------------------------------------------------------------------
    void showOrRaise()
    {
        if (!m_hwnd) return;

        // Reset the ImGui demo window open flag so ShowDemoWindow renders
        // again after a previous hide triggered by the close button.
        m_show = true;

        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }

    // -----------------------------------------------------------------------
    // destroy — full teardown.  Safe to call multiple times.
    // -----------------------------------------------------------------------
    void destroy()
    {
        if (m_hwnd) KillTimer(m_hwnd, kTimerId);

        if (m_ctx)
        {
            ImGui::SetCurrentContext(m_ctx);
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(m_ctx);
            m_ctx = nullptr;
        }

        cleanupRenderTarget();
        if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
        if (m_d3dCtx)    { m_d3dCtx->Release();    m_d3dCtx    = nullptr; }
        if (m_device)    { m_device->Release();     m_device    = nullptr; }

        if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
    }

    ~ImGuiDemoWindow2() { destroy(); }

    ImGuiDemoWindow2(const ImGuiDemoWindow2&)            = delete;
    ImGuiDemoWindow2& operator=(const ImGuiDemoWindow2&) = delete;

private:
    ImGuiDemoWindow2() = default;

    static constexpr UINT_PTR kTimerId = 0xCBA6;
    static constexpr wchar_t  kClass[] = L"xlCOMImGuiDemoWnd2";

    HWND          m_hwnd          = nullptr;
    ImGuiContext* m_ctx           = nullptr;
    bool          m_show          = true;   // ShowDemoWindow open flag

    int           m_width         = 800;
    int           m_height        = 1200;
    bool          m_resizePending = false;
    bool          m_wasVisible    = false;  // for hide/restore on WM_ACTIVATEAPP

    // D3D11 resources
    ID3D11Device*           m_device    = nullptr;
    ID3D11DeviceContext*    m_d3dCtx    = nullptr;
    IDXGISwapChain*         m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv       = nullptr;

    // -----------------------------------------------------------------------
    // HINSTANCE of the hosting DLL.
    // -----------------------------------------------------------------------
    static HINSTANCE dllHandle()
    {
        HMODULE mod = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&dllHandle),
            &mod);
        return mod ? mod : GetModuleHandleW(nullptr);
    }

    // -----------------------------------------------------------------------
    // DWM dark-mode title bar.
    // -----------------------------------------------------------------------
    static void applyWindowTheme(HWND hwnd)
    {
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
        BOOL dark = isDarkMode() ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    }

    // -----------------------------------------------------------------------
    // init — creates the Win32 window, D3D11 device, and ImGui context.
    // -----------------------------------------------------------------------
    [[nodiscard]] bool init(HWND owner)
    {
        ensureClass();

        m_hwnd = CreateWindowExW(
            WS_EX_TOOLWINDOW, kClass, L"Dear ImGui Demo (modeless)",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
            owner,   // owned by Excel — floats above it without being system-wide topmost
            nullptr,
            dllHandle(), this);

        if (!m_hwnd) return false;

        applyWindowTheme(m_hwnd);

        // Position near the owner if available.
        if (owner && IsWindow(owner))
        {
            RECT rc{};
            if (GetWindowRect(owner, &rc))
                SetWindowPos(m_hwnd, nullptr,
                             rc.left + 60, rc.top + 60,
                             0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (!createDevice())
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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename  = nullptr;

        if (isDarkMode())
            SetStyleExcelDark();
        else
            SetStyleExcelLight();

        const float  dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(m_hwnd);
        ImGuiStyle&  style    = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);
        style.FontScaleDpi = dpiScale;

        {
            constexpr const char* kSegoeUI = "C:\\Windows\\Fonts\\segoeui.ttf";
            const bool segoeExists = (GetFileAttributesA(kSegoeUI) != INVALID_FILE_ATTRIBUTES);
            ImFont* font = segoeExists ? io.Fonts->AddFontFromFileTTF(kSegoeUI, 18.0f) : nullptr;
            if (!font)
            {
                auto fs   = cmrc::foo::get_filesystem();
                auto file = fs.open("Resources/Fonts/Inter-VariableFont.ttf");
                ImFontConfig cfg;
                cfg.FontDataOwnedByAtlas = false;
                io.Fonts->AddFontFromMemoryTTF(
                    const_cast<void*>(static_cast<const void*>(file.begin())),
                    static_cast<int>(file.size()),
                    16.0f,
                    &cfg);
            }
        }

        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(m_device, m_d3dCtx);

        SetTimer(m_hwnd, kTimerId, 16, nullptr);
        return true;
    }

    // -----------------------------------------------------------------------
    // renderFrame — called from WM_TIMER / WM_PAINT.
    // -----------------------------------------------------------------------
    void renderFrame()
    {
        // Re-entrance guard — DXGI Present(1, 0) with VSync blocks for up to
        // ~16 ms waiting for the vertical blank.  During that block the Win32
        // message pump can dispatch pending messages, including a WM_TIMER for
        // another ImGui window (e.g. ImGuiTaskPane).  If that timer fires and
        // calls renderFrame() on the other window while we are still inside our
        // own frame, two frames would be built concurrently on the same thread
        // with shared global ImGui state, causing draw-list corruption and
        // assertion failures.
        //
        // If the flag is already set, another window's renderFrame() is on the
        // call stack right now.  Skip this tick — we render on the next one
        // (~16 ms later), which is imperceptible.
        if (ImGuiRenderLock::isLocked()) return;

        // Acquire the render lock — automatically released on any exit from
        // this function, including early returns and exception unwinds.
        ImGuiRenderLock lock;

        if (m_resizePending && m_swapChain)
        {
            m_resizePending = false;
            cleanupRenderTarget();
            m_swapChain->ResizeBuffers(0,
                static_cast<UINT>(m_width), static_cast<UINT>(m_height),
                DXGI_FORMAT_UNKNOWN, 0);
            createRenderTarget();
        }

        if (!m_ctx || !m_rtv) return;

        ImGui::SetCurrentContext(m_ctx);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        ImGui::ShowDemoWindow(&m_show);

        // If user clicked the ImGui demo window's own close button, hide the
        // Win32 window instead of tearing down — matches WM_CLOSE behaviour.
        if (!m_show)
        {
            ImGui::Render();     // must complete the frame before hiding
            m_d3dCtx->OMSetRenderTargets(1, &m_rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            m_swapChain->Present(1, 0);
            ShowWindow(m_hwnd, SW_HIDE);
            return;
        }

        ImGui::Render();

        const bool dark = isDarkMode();
        const float kClear[] = {
            dark ? 0.1608f : 1.0f,
            dark ? 0.1608f : 1.0f,
            dark ? 0.1608f : 1.0f,
            1.0f
        };
        m_d3dCtx->OMSetRenderTargets(1, &m_rtv, nullptr);
        m_d3dCtx->ClearRenderTargetView(m_rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_swapChain->Present(1, 0);
    }

    // -----------------------------------------------------------------------
    // D3D11 device + swap chain
    // -----------------------------------------------------------------------
    bool createDevice()
    {
        DXGI_SWAP_CHAIN_DESC sd   = {};
        sd.BufferCount            = 2;
        sd.BufferDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate = { 60, 1 };
        sd.Flags                  = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage            = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow           = m_hwnd;
        sd.SampleDesc             = { 1, 0 };
        sd.Windowed               = TRUE;
        sd.SwapEffect             = DXGI_SWAP_EFFECT_DISCARD;

        constexpr D3D_FEATURE_LEVEL kLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };
        D3D_FEATURE_LEVEL fl = {};

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            kLevels, 2, D3D11_SDK_VERSION,
            &sd, &m_swapChain, &m_device, &fl, &m_d3dCtx);

        if (hr == DXGI_ERROR_UNSUPPORTED)
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                kLevels, 2, D3D11_SDK_VERSION,
                &sd, &m_swapChain, &m_device, &fl, &m_d3dCtx);

        if (FAILED(hr)) return false;
        createRenderTarget();
        return true;
    }

    void createRenderTarget()
    {
        ID3D11Texture2D* back = nullptr;
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&back));
        if (back)
        {
            m_device->CreateRenderTargetView(back, nullptr, &m_rtv);
            back->Release();
        }
    }

    void cleanupRenderTarget()
    {
        if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    }

    // -----------------------------------------------------------------------
    // Win32 window class + WndProc
    // -----------------------------------------------------------------------
    static void ensureClass()
    {
        WNDCLASSEXW wc   = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &WndProc;
        wc.hInstance     = dllHandle();
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

        // Context contamination guard — DXGI Present(1, 0) in another window's
        // renderFrame() can pump the Win32 message queue while blocking for VSync.
        // If a WM_TIMER or other message is dispatched to this WndProc during that
        // block, the SetCurrentContext() call below would change the global ImGui
        // context.  Without a save/restore, when Present() returns in the other
        // window the global context would point at our window instead of theirs,
        // causing the other window to render with the wrong HWND, backends, and
        // draw lists.
        //
        // ImGuiContextGuard saves ImGui::GetCurrentContext() here and restores it
        // in its destructor, so this WndProc is side-effect-free with respect to
        // the global context — regardless of how it was invoked.
        ImGuiContextGuard ctxGuard;

        auto* self = reinterpret_cast<ImGuiDemoWindow2*>(
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
            case WM_ERASEBKGND:
                return 1;

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
                if (self->m_swapChain && wParam != SIZE_MINIMIZED)
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
                // Hide rather than destroy — the window can be re-shown.
                ShowWindow(hwnd, SW_HIDE);
                return 0;

            case WM_ACTIVATEAPP:
                // Hide on app deactivation, restore on activation.
                if (wParam == FALSE)
                {
                    ShowWindow(hwnd, SW_HIDE);
                    self->m_wasVisible = true;
                }
                else if (self->m_wasVisible)
                {
                    ShowWindow(hwnd, SW_SHOW);
                    self->m_wasVisible = false;
                }
                return 0;

            default: break;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

