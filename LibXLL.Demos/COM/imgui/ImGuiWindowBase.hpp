// ---------------------------------------------------------------------------
// ImGuiWindowBase.hpp — shared D3D11 + ImGui infrastructure for all
//                        type-erased ImGui windows.
//
// This is an internal base class.  The public API is ImGuiModalWindow and
// ImGuiModelessWindow.
//
// DESIGN NOTES
// ------------
// All owned windows share a single registered Win32 class (kClass) and a
// single static WndProc.  Polymorphic behaviour (close, activate) is
// dispatched through virtual methods on the base class pointer stored in
// GWLP_USERDATA.
//
// A shared ImFontAtlas is reference-counted across all live windows so that
// fonts are loaded once and shared by every per-window ImGuiContext.
//
// A static registry (s_registry) tracks all live windows for future use
// (e.g. broadcasting DPI-change events).
// ---------------------------------------------------------------------------

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <algorithm>
#include <memory>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "SetStyle.hpp"
#include "Utils/IsDarkMode.hpp"
#include "ImGuiRenderGuard.hpp"
#include "ImGuiWindowContent.hpp"

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(foo);

// ---------------------------------------------------------------------------
class ImGuiWindowBase
{
    template<WindowContent> friend class ImGuiTaskPane;

public:
    ImGuiWindowBase(const ImGuiWindowBase&)            = delete;
    ImGuiWindowBase& operator=(const ImGuiWindowBase&) = delete;

protected:
    static constexpr UINT_PTR kTimerId = 0xCBA5;
    static constexpr wchar_t  kClass[] = L"xlLibImGuiWindow";

    // Per-window state -------------------------------------------------------
    HWND          m_hwnd             = nullptr;
    ImGuiContext* m_ctx              = nullptr;
    int           m_width            = 800;
    int           m_height           = 600;
    bool          m_resizePending    = false;
    bool          m_swapChainOccluded = false;

    // D3D11 resources (per-window) -------------------------------------------
    IDXGISwapChain*         m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv       = nullptr;

    // Type-erased content ----------------------------------------------------
    std::unique_ptr<ContentConcept> m_content;

    // Shared across all instances --------------------------------------------
    inline static ID3D11Device*             s_device    = nullptr;
    inline static ID3D11DeviceContext*      s_d3dDevCtx = nullptr;
    inline static ImFontAtlas*              s_sharedAtlas = nullptr;
    inline static int                       s_windowCount = 0;
    inline static int                       s_atlasFrameCounter = 0;
    inline static std::vector<ImGuiWindowBase*> s_registry;

    // -----------------------------------------------------------------------
    ImGuiWindowBase() = default;
    virtual ~ImGuiWindowBase() { shutdownBase(); }

    // -----------------------------------------------------------------------
    // Virtual interface — derived classes customise close and activate
    // behaviour; everything else is identical across window types.
    // -----------------------------------------------------------------------
    virtual void onWindowClose()            = 0;
    virtual void onWindowActivateApp(bool)  {}   // no-op default

    // -----------------------------------------------------------------------
    // initBase — creates the Win32 window, D3D11 device, and ImGui context.
    // Must be called from the derived-class factory before returning.
    //
    //   parent   — Win32 parent/owner HWND passed to CreateWindowExW.
    //   exStyle  — extended window style (e.g. WS_EX_TOOLWINDOW).
    //   title    — window title bar text.
    //   w, h     — initial client width and height in pixels.
    // -----------------------------------------------------------------------
    [[nodiscard]] bool initBase(HWND           parent,
                                DWORD          exStyle,
                                const wchar_t* title,
                                int            w,
                                int            h)
    {
        m_width  = w;
        m_height = h;

        ensureClass();

        m_hwnd = CreateWindowExW(
            exStyle, kClass, title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, w, h,
            parent, nullptr, dllHandle(), this);

        if (!m_hwnd) return false;

        applyWindowTheme(m_hwnd);

        if (!createDevice())
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            return false;
        }

        // First window: create the shared atlas and load fonts into it.
        if (s_windowCount++ == 0)
            initSharedAtlas();

        IMGUI_CHECKVERSION();
        m_ctx = ImGui::CreateContext(s_sharedAtlas);
        ImGui::SetCurrentContext(m_ctx);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags        |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags        |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename         = nullptr;
        io.ConfigDpiScaleFonts = true;

        isDarkMode() ? SetStyleExcelDark() : SetStyleExcelLight();

        const float dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(m_hwnd);
        ImGuiStyle& style    = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);
        style.FontScaleDpi = dpiScale;

        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(s_device, s_d3dDevCtx);

        SetTimer(m_hwnd, kTimerId, 16, nullptr);

        s_registry.push_back(this);
        return true;
    }

    // -----------------------------------------------------------------------
    // shutdownBase — kills the timer, shuts down ImGui and D3D11, destroys
    // the Win32 window, and removes this instance from the registry.
    // Safe to call on a partially-initialised or moved-from object
    // (all operations are guarded by null checks).
    // -----------------------------------------------------------------------
    void shutdownBase()
    {
        if (m_hwnd) KillTimer(m_hwnd, kTimerId);

        bool wasLastWindow = false;
        if (m_ctx)
        {
            ImGui::SetCurrentContext(m_ctx);
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(m_ctx);
            m_ctx = nullptr;

            wasLastWindow = (--s_windowCount == 0);
        }

        cleanupRenderTarget();
        if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }

        if (wasLastWindow)
        {
            // ImGui::DestroyContext → Shutdown() already calls
            // IM_DELETE on the atlas when its RefCount reaches 0,
            // so we must NOT delete it again.  Just forget our pointer
            // and reset the per-atlas frame counter for the next batch.
            s_sharedAtlas       = nullptr;
            s_atlasFrameCounter = 0;

            if (s_d3dDevCtx) { s_d3dDevCtx->Release(); s_d3dDevCtx = nullptr; }
            if (s_device)    { s_device->Release();     s_device    = nullptr; }
        }

        if (m_hwnd)
        {
            // Clear GWLP_USERDATA before DestroyWindow so that WM_DESTROY
            // dispatched to the WndProc sees a null self pointer and falls
            // through to DefWindowProcW safely.
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        // Remove from the shared registry (erase–remove idiom).
        auto& r = s_registry;
        r.erase(std::remove(r.begin(), r.end(), this), r.end());
    }

    // -----------------------------------------------------------------------
    // renderFrame — builds and presents one Dear ImGui frame.
    // Called from the WM_TIMER handler (~60 fps) and from WM_PAINT for
    // forced synchronous redraws.
    // -----------------------------------------------------------------------
    void renderFrame()
    {
        // Re-entrance guard: DXGI Present(1,0) can pump the message queue
        // while blocking for VSync, potentially dispatching a WM_TIMER for
        // another ImGui window.  Skip the nested call — it will be retried
        // on the next timer tick (~16 ms later).
        if (ImGuiRenderLock::isLocked()) return;
        ImGuiRenderLock lock;

        // Skip frames while the window is occluded (minimised / lock screen).
        if (m_swapChainOccluded)
        {
            if (m_swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
                return;
            m_swapChainOccluded = false;
        }

        // Apply any pending swap-chain resize before rendering.
        if (m_resizePending && m_swapChain)
        {
            m_resizePending = false;
            cleanupRenderTarget();
            m_swapChain->ResizeBuffers(0,
                static_cast<UINT>(m_width), static_cast<UINT>(m_height),
                DXGI_FORMAT_UNKNOWN, 0);
            createRenderTarget();
        }

        if (!m_ctx || !m_rtv || !m_content) return;

        ImGui::SetCurrentContext(m_ctx);

        // The shared atlas is not owned by any context, so ImGui will not
        // call ImFontAtlasUpdateNewFrame() automatically.  We must do it
        // ourselves before NewFrame() — see imgui.cpp comment (1).
        if (s_sharedAtlas)
        {
            const bool has_textures =
                (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0;
            ImFontAtlasUpdateNewFrame(s_sharedAtlas, ++s_atlasFrameCounter, has_textures);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const FrameAction action = m_content->renderContent();

        // Notify content before finalising the frame so it can react to the
        // close signal while Dear ImGui's frame is still open.
        if (action == FrameAction::RequestClose)
            m_content->onClose();

        ImGui::Render();

        const bool  dark     = isDarkMode();
        const float kClear[] = {
            dark ? 0.1608f : 1.0f,
            dark ? 0.1608f : 1.0f,
            dark ? 0.1608f : 1.0f,
            1.0f
        };
        s_d3dDevCtx->OMSetRenderTargets(1, &m_rtv, nullptr);
        s_d3dDevCtx->ClearRenderTargetView(m_rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_swapChainOccluded = (m_swapChain->Present(1, 0) == DXGI_STATUS_OCCLUDED);

        // Let content run deferred post-render actions (e.g. modal dialogs)
        // while the last frame is fully presented.
        if (m_content) m_content->postRender();

        // Act on the close request after the frame is fully presented so
        // the last rendered frame is visible before the window closes/hides.
        if (action == FrameAction::RequestClose)
            onWindowClose();
    }

private:
    // -----------------------------------------------------------------------
    // dllHandle — returns the HINSTANCE of the DLL that contains this code,
    // used when registering the Win32 window class.
    // -----------------------------------------------------------------------
    static HINSTANCE dllHandle()
    {
        HMODULE mod = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&dllHandle), &mod);
        return mod ? mod : GetModuleHandleW(nullptr);
    }

    // -----------------------------------------------------------------------
    // applyWindowTheme — sets the DWM title bar to match the Windows
    // dark / light mode preference.
    // -----------------------------------------------------------------------
    static void applyWindowTheme(HWND hwnd)
    {
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
        BOOL dark = isDarkMode() ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                              &dark, sizeof(dark));
    }

    // -----------------------------------------------------------------------
    // ensureClass — registers the shared Win32 window class once.
    // RegisterClassExW is idempotent: it returns ERROR_CLASS_ALREADY_EXISTS
    // on subsequent calls, which we intentionally ignore.
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

    // -----------------------------------------------------------------------
    // initSharedAtlas — loads fonts into the shared ImFontAtlas.
    // Called once when the first window is created.  Tries Segoe UI first;
    // falls back to the CMakeRC-embedded Inter Variable Font.
    // -----------------------------------------------------------------------
    static void initSharedAtlas()
    {
        s_sharedAtlas = IM_NEW(ImFontAtlas);

        constexpr const char* kSegoeUI = "C:\\Windows\\Fonts\\segoeui.ttf";
        const bool segoeExists =
            GetFileAttributesA(kSegoeUI) != INVALID_FILE_ATTRIBUTES;

        if (!segoeExists || !s_sharedAtlas->AddFontFromFileTTF(kSegoeUI, 18.0f))
        {
            auto     fs   = cmrc::foo::get_filesystem();
            auto     file = fs.open("Resources/Fonts/Inter-VariableFont.ttf");
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            s_sharedAtlas->AddFontFromMemoryTTF(
                const_cast<void*>(static_cast<const void*>(file.begin())),
                static_cast<int>(file.size()),
                16.0f, &cfg);
        }
    }

    // -----------------------------------------------------------------------
    // D3D11 helpers
    // -----------------------------------------------------------------------
    bool createDevice()
    {
        // Create the shared D3D11 device on the first call.
        if (!s_device)
        {
            constexpr D3D_FEATURE_LEVEL kLevels[] = {
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_0,
            };
            D3D_FEATURE_LEVEL fl = {};

            HRESULT hr = D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                kLevels, 2, D3D11_SDK_VERSION,
                &s_device, &fl, &s_d3dDevCtx);

            if (hr == DXGI_ERROR_UNSUPPORTED)
                hr = D3D11CreateDevice(
                    nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                    kLevels, 2, D3D11_SDK_VERSION,
                    &s_device, &fl, &s_d3dDevCtx);

            if (FAILED(hr)) return false;
        }

        // Create a per-window swap chain on the shared device.
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

        IDXGIDevice*  dxgiDevice  = nullptr;
        IDXGIAdapter* dxgiAdapter = nullptr;
        IDXGIFactory* dxgiFactory = nullptr;

        HRESULT hr = s_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        if (SUCCEEDED(hr)) hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
        if (SUCCEEDED(hr)) hr = dxgiFactory->CreateSwapChain(s_device, &sd, &m_swapChain);

        // Disable DXGI's Alt+Enter fullscreen toggle — we're hosted in Excel.
        if (SUCCEEDED(hr))
            dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

        if (dxgiFactory) dxgiFactory->Release();
        if (dxgiAdapter) dxgiAdapter->Release();
        if (dxgiDevice)  dxgiDevice->Release();

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
            s_device->CreateRenderTargetView(back, nullptr, &m_rtv);
            back->Release();
        }
    }

    void cleanupRenderTarget()
    {
        if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    }

    // -----------------------------------------------------------------------
    // WndProc — shared by all ImGuiWindowBase-derived windows.
    //
    // Message handling order:
    //   1. WM_NCCREATE: store the ImGuiWindowBase* in GWLP_USERDATA.
    //   2. ImGuiContextGuard: save/restore the global ImGui context so that
    //      messages dispatched inside DXGI Present() do not leak a context
    //      switch to the caller's frame.
    //   3. Forward to ImGui_ImplWin32_WndProcHandler for input processing.
    //   4. Custom handling for timer, paint, size, close, activate, DPI.
    //   5. DefWindowProcW for everything else.
    // -----------------------------------------------------------------------
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

        ImGuiContextGuard ctxGuard;

        auto* self = reinterpret_cast<ImGuiWindowBase*>(
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
                // Suppress default erase: D3D11 paints the full surface every
                // frame, so erasing with the window brush first would flash.
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
                // Notify content, then let the window type decide what to do
                // (modal exits its loop; modeless hides).
                if (self->m_content) self->m_content->onClose();
                self->onWindowClose();
                return 0;

            case WM_ACTIVATEAPP:
                if (self->m_content) self->m_content->onActivateApp(wParam != 0);
                self->onWindowActivateApp(wParam != 0);
                return 0;

            case WM_DPICHANGED:
            {
                // Resize to the rectangle suggested by Windows.
                const float newScale =
                    static_cast<float>(HIWORD(wParam)) / 96.0f;
                const RECT* r = reinterpret_cast<const RECT*>(lParam);
                SetWindowPos(hwnd, nullptr,
                             r->left, r->top,
                             r->right - r->left, r->bottom - r->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);

                // Re-apply the style from scratch (ScaleAllSizes is
                // cumulative, so we must start from the unscaled base).
                if (self->m_ctx)
                {
                    ImGui::SetCurrentContext(self->m_ctx);
                    isDarkMode() ? SetStyleExcelDark() : SetStyleExcelLight();
                    ImGui::GetStyle().ScaleAllSizes(newScale);
                    ImGui::GetStyle().FontScaleDpi = newScale;
                }
                if (self->m_content) self->m_content->onDpiChanged(newScale);
                return 0;
            }

            default: break;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};
