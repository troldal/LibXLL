// ---------------------------------------------------------------------------
// ImGuiDemoWindow.hpp — self-contained modal 800 x 1200 window that hosts
// the Dear ImGui built-in demo (ImGui::ShowDemoWindow).
//
// Designed for the xlCOM add-in: launched from a ribbon callback on Excel's
// STA thread.  Creates its own Win32 window, D3D11 device/swap chain, and
// per-instance ImGuiContext.  Runs a local PeekMessage loop until the user
// closes the window.
//
// show(owner) disables the Excel window for proper modal semantics, centres
// the dialog over the owner, and re-enables/foregrounds the owner on exit.
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

class ImGuiDemoWindow
{
public:
    /// Opens an 800 x 1200 modal window running the ImGui built-in demo.
    /// Blocks until the user closes the window (Win32 X or ImGui close button).
    static void show(HWND owner)
    {
        ImGuiDemoWindow dlg;
        if (!dlg.create()) return;

        // Modal semantics: disable owner, centre dialog, re-enable on exit.
        if (owner && IsWindow(owner))
        {
            SetWindowLongPtr(dlg.m_hwnd, GWLP_HWNDPARENT,
                             reinterpret_cast<LONG_PTR>(owner));
            RECT ow{}, wd{};
            GetWindowRect(owner,      &ow);
            GetWindowRect(dlg.m_hwnd, &wd);
            const int dw = wd.right  - wd.left;
            const int dh = wd.bottom - wd.top;
            SetWindowPos(dlg.m_hwnd, nullptr,
                         ow.left + (ow.right  - ow.left - dw) / 2,
                         ow.top  + (ow.bottom - ow.top  - dh) / 2,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            EnableWindow(owner, FALSE);
        }

        ShowWindow(dlg.m_hwnd, SW_SHOW);
        UpdateWindow(dlg.m_hwnd);

        dlg.runModal();

        // Re-enable owner.
        if (owner && IsWindow(owner))
        {
            EnableWindow(owner, TRUE);
            SetForegroundWindow(owner);
        }
    }

private:
    static constexpr UINT_PTR kTimerId = 0xCBA5;
    static constexpr wchar_t  kClass[] = L"xlCOMImGuiDemoWnd";

    HWND          m_hwnd          = nullptr;
    ImGuiContext* m_ctx           = nullptr;
    bool          m_done          = false;
    bool          m_show          = true;  // ShowDemoWindow open flag

    int           m_width              = 1600;
    int           m_height             = 1200;
    bool          m_resizePending      = false;
    bool          m_swapChainOccluded  = false;

    // D3D11 resources
    ID3D11Device*           m_device    = nullptr;
    ID3D11DeviceContext*    m_d3dCtx    = nullptr;
    IDXGISwapChain*         m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv       = nullptr;

    // -----------------------------------------------------------------------
    // HINSTANCE of the hosting DLL (for window class registration).
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
    // Window + D3D11 + ImGui creation
    // -----------------------------------------------------------------------
    [[nodiscard]] bool create()
    {
        ensureClass();

        m_hwnd = CreateWindowExW(
            0, kClass, L"Dear ImGui Demo",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
            nullptr, nullptr,
            dllHandle(), this);

        if (!m_hwnd) return false;

        applyWindowTheme(m_hwnd);

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

        // Apply Excel dark/light style based on Windows theme.
        if (isDarkMode())
            SetStyleExcelDark();
        else
            SetStyleExcelLight();

        // Scale for DPI.
        const float  dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(m_hwnd);
        ImGuiStyle&  style    = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);
        style.FontScaleDpi        = dpiScale;
        io.ConfigDpiScaleFonts    = true;

        // Load Segoe UI; fall back to embedded Inter font, then ImGui default.
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

    ~ImGuiDemoWindow()
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

    // -----------------------------------------------------------------------
    // Modal message loop
    // -----------------------------------------------------------------------
    void runModal()
    {
        MSG msg;
        while (!m_done)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) { m_done = true; break; }
            }
            if (!m_done) renderFrame();
        }
    }

    // -----------------------------------------------------------------------
    // Render one Dear ImGui frame
    // -----------------------------------------------------------------------
    void renderFrame()
    {
        // Re-entrance guard — although this modal window drives rendering via
        // its own PeekMessage loop rather than WM_TIMER, it still participates
        // in the shared flag.  If a task pane or modeless ImGui window happens
        // to be rendering at the same moment (e.g. its WM_TIMER fires inside
        // our PeekMessage / DispatchMessage call), the guard prevents a second
        // concurrent frame from starting on the same thread.
        //
        // If the flag is already set, another window's renderFrame() is on the
        // call stack right now.  Skip this call — we will retry on the next
        // iteration of the PeekMessage loop.
        if (ImGuiRenderLock::isLocked()) return;

        // Acquire the render lock — automatically released on any exit from
        // this function, including early returns and exception unwinds.
        ImGuiRenderLock lock;

        // Handle window being minimized or screen locked: test-present and
        // skip the frame until the window is visible again.
        if (m_swapChainOccluded)
        {
            if (m_swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
                return;
            m_swapChainOccluded = false;
        }

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
        if (!m_show) m_done = true;

        ImGui::Render();

        // Clear colour: #292929 dark, #FFFFFF light
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
        m_swapChainOccluded = (m_swapChain->Present(1, 0) == DXGI_STATUS_OCCLUDED);
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

        // Disable DXGI's Alt+Enter fullscreen toggle — we're hosted inside Excel.
        {
            IDXGIFactory* factory = nullptr;
            if (SUCCEEDED(m_swapChain->GetParent(IID_PPV_ARGS(&factory))))
            {
                factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
                factory->Release();
            }
        }

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

        // Context contamination guard — even though this modal window runs its
        // own PeekMessage loop, DispatchMessage inside that loop can invoke this
        // WndProc while another ImGui window's Present() is pumping the queue.
        // ImGuiContextGuard saves the current ImGui context on entry and restores
        // it on exit, ensuring the SetCurrentContext() call below does not
        // permanently alter the context seen by the caller.
        ImGuiContextGuard ctxGuard;

        auto* self = reinterpret_cast<ImGuiDemoWindow*>(
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
                self->m_done = true;
                return 0;

            default: break;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

