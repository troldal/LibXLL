// ---------------------------------------------------------------------------
// ImGuiTaskPane.hpp — Dear ImGui (Win32 + DirectX 11) content implementation
//                     for TaskPaneControl, using type-erased WindowContent.
//
// This class template satisfies the Content concept required by
// TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Has static void shutdown()
//   - Destructor handles cleanup
//
// The template parameter T must satisfy the WindowContent concept defined in
// ImGuiWindowContent.hpp (i.e. it must have a renderContent() method that
// returns FrameAction).  Optional hooks — onClose(), onDpiChanged(float),
// onActivateApp(bool), postRender() — are detected at compile time and
// called when present.
//
// SHARED INFRASTRUCTURE
// ---------------------
// The D3D11 device, device context, and ImFontAtlas are shared with all
// ImGuiWindowBase-derived windows (ImGuiModalWindow, ImGuiModelessWindow)
// via static members in ImGuiWindowBase.  ImGuiTaskPane is declared as a
// friend of ImGuiWindowBase so that it can access the shared statics
// directly.
//
// RENDERING STRATEGY
// ------------------
// A DirectX 11 swap chain is created on the container HWND supplied by
// TaskPaneControl.  A Win32 timer (~60 fps) continuously calls renderFrame()
// so that ImGui animations and hover effects stay responsive regardless of
// how often Excel generates paint messages.
//
// The container HWND is subclassed (SetWindowLongPtrW / GWLP_WNDPROC) so
// that Win32 messages (mouse, keyboard, WM_PAINT, WM_SIZE, WM_TIMER) can
// be forwarded to the ImGui Win32 backend via
// ImGui_ImplWin32_WndProcHandler before any custom handling.
//
// IMGUI CONTEXT
// -------------
// A per-instance ImGuiContext is created in the constructor and activated
// with ImGui::SetCurrentContext() before every ImGui call.  The shared
// ImFontAtlas is passed to ImGui::CreateContext() so that fonts are loaded
// once and reused across all windows and task panes.
//
// THREADING
// ---------
// All operations happen on Excel's STA thread.  No secondary threads or
// additional message pumps are involved.
// ---------------------------------------------------------------------------

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

// ImGui_ImplWin32_WndProcHandler is intentionally placed inside a #if 0
// guard in imgui_impl_win32.h to avoid pulling <windows.h> into every
// translation unit that includes the backend header.  Declare it explicitly
// here, after <windows.h> is already in scope.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "SetStyle.hpp"
#include "Utils/IsDarkMode.hpp"
#include "ImGuiRenderGuard.hpp"
#include "ImGuiWindowContent.hpp"
#include "ImGuiWindowBase.hpp"
#include <iostream>
#include <memory>

template<WindowContent T>
class ImGuiTaskPane
{
public:
    // -----------------------------------------------------------------------
    // shutdown — process-wide teardown hook called from OnDisconnection.
    //
    // All cleanup is handled per-instance in the destructor.  This static
    // method exists for API symmetry with other content implementations
    // (WxTaskPane, QtTaskPane, FltkTaskPane, etc.).
    // -----------------------------------------------------------------------
    static void shutdown() { /* per-instance cleanup only */ }

    // -----------------------------------------------------------------------
    // Constructor — creates a per-window swap chain on the shared D3D11
    // device, initialises Dear ImGui with the Win32 and DX11 backends, and
    // subclasses parentHwnd to intercept Win32 messages.  A WM_TIMER fires
    // every ~16 ms (~60 fps) to drive continuous rendering.
    //
    // The type-erased content object (ContentModel<T>) is default-constructed
    // from T{} and stored as a unique_ptr<ContentConcept>.
    // -----------------------------------------------------------------------
    ImGuiTaskPane(HWND parentHwnd, int w, int h)
        : m_hwnd(parentHwnd)
        , m_width(w > 0 ? w : 1)
        , m_height(h > 0 ? h : 1)
    {
        if (!ensureDeviceAndSwapChain())
        {
            std::cerr << "[xlCOM] ImGuiTaskPane: D3D11 device/swap-chain creation failed\n";
            return;
        }

        // First window/pane: create the shared atlas and load fonts into it.
        if (ImGuiWindowBase::s_windowCount++ == 0)
            ImGuiWindowBase::initSharedAtlas();

        // Create a per-pane ImGui context with the shared atlas so that
        // fonts are loaded once and shared by every window and pane.
        IMGUI_CHECKVERSION();
        m_ctx = ImGui::CreateContext(ImGuiWindowBase::s_sharedAtlas);
        ImGui::SetCurrentContext(m_ctx);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags        |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename         = nullptr;
        io.ConfigDpiScaleFonts = true;

        // Apply the colour scheme matching the system dark/light preference.
        isDarkMode() ? SetStyleExcelDark() : SetStyleExcelLight();

        // Scale the style for the display DPI.
        const float dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(m_hwnd);
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);
        style.FontScaleDpi = dpiScale;

        // Initialise platform and renderer backends.
        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(ImGuiWindowBase::s_device, ImGuiWindowBase::s_d3dDevCtx);

        // Create the type-erased content object.
        m_content = std::make_unique<ContentModel<T>>(T{});

        // Subclass parentHwnd so that WM_PAINT, WM_SIZE, WM_TIMER, and all
        // input messages can be intercepted.  Store `this` in GWLP_USERDATA
        // so the static WndProc can route calls back to the instance.
        m_prevWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(&ImGuiTaskPane::subclassProc)));
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(this));

        // Drive rendering at ~60 fps via a WM_TIMER posted to the subclassed HWND.
        SetTimer(m_hwnd, kTimerId, 16, nullptr);

        std::cerr << "[xlCOM] ImGuiTaskPane created (" << m_width << "x" << m_height << ")\n";
    }

    // -----------------------------------------------------------------------
    // resize — called by TaskPaneControl::SetObjectRects() when the task pane
    // is resized.  Stores the new target dimensions and sets a flag so that
    // ResizeBuffers is called once inside the next renderFrame() tick rather
    // than on every SetObjectRects call during a live drag.  This prevents the
    // per-step buffer-clear flicker that would otherwise occur.
    // -----------------------------------------------------------------------
    void resize(int w, int h)
    {
        if (w <= 0 || h <= 0) return;
        m_width         = w;
        m_height        = h;
        m_resizePending = true;   // ResizeBuffers deferred to renderFrame()
    }

    // -----------------------------------------------------------------------
    // Destructor — shuts down ImGui backends, releases the per-window swap
    // chain, and restores the original WndProc on the container HWND.
    //
    // The timer is killed first so no WM_TIMER callbacks arrive after the
    // D3D11 or ImGui state has been torn down.
    //
    // When this is the last window/pane, the shared D3D11 device, device
    // context, and font atlas are released.
    // -----------------------------------------------------------------------
    ~ImGuiTaskPane()
    {
        // Stop the render timer before tearing down D3D11 and ImGui state.
        if (m_hwnd)
            KillTimer(m_hwnd, kTimerId);

        // Restore the original WndProc and clear the USERDATA slot.
        if (m_hwnd && m_prevWndProc)
        {
            SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(m_prevWndProc));
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
        }

        // Destroy the content before tearing down ImGui/D3D11.
        m_content.reset();

        // Shut down ImGui backends, then destroy the per-pane context.
        bool wasLastWindow = false;
        if (m_ctx)
        {
            ImGui::SetCurrentContext(m_ctx);
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(m_ctx);
            m_ctx = nullptr;

            wasLastWindow = (--ImGuiWindowBase::s_windowCount == 0);
        }

        // Release per-window D3D11 resources.
        cleanupRenderTarget();
        if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }

        // When this is the last window/pane, release the shared resources.
        if (wasLastWindow)
        {
            ImGuiWindowBase::s_sharedAtlas       = nullptr;
            ImGuiWindowBase::s_atlasFrameCounter = 0;

            if (ImGuiWindowBase::s_d3dDevCtx)
            {
                ImGuiWindowBase::s_d3dDevCtx->Release();
                ImGuiWindowBase::s_d3dDevCtx = nullptr;
            }
            if (ImGuiWindowBase::s_device)
            {
                ImGuiWindowBase::s_device->Release();
                ImGuiWindowBase::s_device = nullptr;
            }
        }
    }

    // Non-copyable, non-movable (raw COM pointer ownership).
    ImGuiTaskPane(const ImGuiTaskPane&)            = delete;
    ImGuiTaskPane& operator=(const ImGuiTaskPane&) = delete;
    ImGuiTaskPane(ImGuiTaskPane&&)                 = delete;
    ImGuiTaskPane& operator=(ImGuiTaskPane&&)      = delete;

private:
    // Arbitrary non-zero timer ID — chosen to avoid clashing with any timer
    // that the original WndProc of the container HWND may already use.
    static constexpr UINT_PTR kTimerId = 0xCBA1;

    HWND    m_hwnd        = nullptr;   // container HWND (not owned)
    WNDPROC m_prevWndProc = nullptr;   // saved original WndProc; restored in destructor
    int     m_width       = 1;
    int     m_height      = 1;

    IDXGISwapChain*         m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv       = nullptr;

    ImGuiContext*           m_ctx               = nullptr;
    bool                    m_resizePending     = false;
    bool                    m_swapChainOccluded = false;

    // Type-erased content -------------------------------------------------
    std::unique_ptr<ContentConcept> m_content;

    // -----------------------------------------------------------------------
    // ensureDeviceAndSwapChain — creates the shared D3D11 device (if it does
    // not yet exist) and a per-window DXGI swap chain bound to m_hwnd.
    // Falls back to the WARP software rasteriser if the hardware adapter
    // rejects the device.
    // -----------------------------------------------------------------------
    bool ensureDeviceAndSwapChain()
    {
        // Create the shared D3D11 device on the first call.
        if (!ImGuiWindowBase::s_device)
        {
            constexpr D3D_FEATURE_LEVEL kLevels[] = {
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_0,
            };
            D3D_FEATURE_LEVEL fl = {};

            HRESULT hr = D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                kLevels, 2, D3D11_SDK_VERSION,
                &ImGuiWindowBase::s_device, &fl, &ImGuiWindowBase::s_d3dDevCtx);

            if (hr == DXGI_ERROR_UNSUPPORTED)
                hr = D3D11CreateDevice(
                    nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                    kLevels, 2, D3D11_SDK_VERSION,
                    &ImGuiWindowBase::s_device, &fl, &ImGuiWindowBase::s_d3dDevCtx);

            if (FAILED(hr)) return false;
        }

        // Create a per-window swap chain on the shared device.
        DXGI_SWAP_CHAIN_DESC sd   = {};
        sd.BufferCount            = 2;
        sd.BufferDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate  = { 60, 1 };
        sd.Flags                  = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage            = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow           = m_hwnd;
        sd.SampleDesc             = { 1, 0 };
        sd.Windowed               = TRUE;
        sd.SwapEffect             = DXGI_SWAP_EFFECT_DISCARD;

        IDXGIDevice*  dxgiDevice  = nullptr;
        IDXGIAdapter* dxgiAdapter = nullptr;
        IDXGIFactory* dxgiFactory = nullptr;

        HRESULT hr = ImGuiWindowBase::s_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        if (SUCCEEDED(hr)) hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
        if (SUCCEEDED(hr)) hr = dxgiFactory->CreateSwapChain(ImGuiWindowBase::s_device, &sd, &m_swapChain);

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

    // -----------------------------------------------------------------------
    // createRenderTarget / cleanupRenderTarget — manage the
    // ID3D11RenderTargetView wrapping the swap chain's back buffer.
    // cleanupRenderTarget must be called before IDXGISwapChain::ResizeBuffers;
    // createRenderTarget must be called after.
    // -----------------------------------------------------------------------
    void createRenderTarget()
    {
        ID3D11Texture2D* pBack = nullptr;
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
        if (pBack)
        {
            ImGuiWindowBase::s_device->CreateRenderTargetView(pBack, nullptr, &m_rtv);
            pBack->Release();
        }
    }

    void cleanupRenderTarget()
    {
        if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    }

    // -----------------------------------------------------------------------
    // renderFrame — builds and presents one Dear ImGui frame.
    //
    // Called from the WM_TIMER handler (~60 fps) for continuous animation
    // and from the WM_PAINT handler for forced synchronous redraws.
    // Delegates all UI building to the type-erased content object.
    // -----------------------------------------------------------------------
    void renderFrame()
    {
        // Re-entrance guard — DXGI Present(1, 0) with VSync blocks for up to
        // ~16 ms waiting for the vertical blank.  During that block the Win32
        // message pump can dispatch pending messages, including a WM_TIMER for
        // another ImGui window.  If that timer fires and calls renderFrame() on
        // the other window while we are still inside our own frame, two frames
        // would be built concurrently on the same thread with shared global ImGui
        // state, causing draw-list corruption and assertion failures.
        //
        // If the flag is already set, another window's renderFrame() is on the
        // call stack right now.  Skip this tick — we render on the next one
        // (~16 ms later), which is imperceptible.
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

        // Apply any pending swap-chain resize before rendering.  Deferring
        // ResizeBuffers here means it fires at most once per timer tick
        // (~16 ms) regardless of how many SetObjectRects calls arrived since
        // the last frame, eliminating the per-step buffer-clear flicker.
        if (m_resizePending && m_swapChain)
        {
            m_resizePending = false;
            cleanupRenderTarget();
            m_swapChain->ResizeBuffers(0,
                                       static_cast<UINT>(m_width),
                                       static_cast<UINT>(m_height),
                                       DXGI_FORMAT_UNKNOWN, 0);
            createRenderTarget();
        }

        if (!ImGuiWindowBase::s_device || !m_rtv || !m_ctx || !m_content) return;

        ImGui::SetCurrentContext(m_ctx);

        // The shared atlas is not owned by any context, so ImGui will not
        // call ImFontAtlasUpdateNewFrame() automatically.  We must do it
        // ourselves before NewFrame().
        if (ImGuiWindowBase::s_sharedAtlas)
        {
            const bool has_textures =
                (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0;
            ImFontAtlasUpdateNewFrame(ImGuiWindowBase::s_sharedAtlas,
                                      ++ImGuiWindowBase::s_atlasFrameCounter,
                                      has_textures);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        // Delegate all UI building to the type-erased content object.
        m_content->renderContent();

        ImGui::Render();

        // Clear colour matches the current theme.
        const bool  dark     = isDarkMode();
        const float kClear[] = {
            dark ? 0.1608f : 1.0f,
            dark ? 0.1608f : 1.0f,
            dark ? 0.1608f : 1.0f,
            1.0f
        };
        ImGuiWindowBase::s_d3dDevCtx->OMSetRenderTargets(1, &m_rtv, nullptr);
        ImGuiWindowBase::s_d3dDevCtx->ClearRenderTargetView(m_rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_swapChainOccluded = (m_swapChain->Present(1, 0) == DXGI_STATUS_OCCLUDED);

        // Let content run deferred post-render actions (e.g. modal dialogs)
        // while the last frame is fully presented.
        m_content->postRender();
    }

    // -----------------------------------------------------------------------
    // subclassProc — static WndProc installed on the container HWND via
    // SetWindowLongPtrW.
    //
    // Message handling order:
    //   1. Forward every message to ImGui_ImplWin32_WndProcHandler first so
    //      that ImGui can track mouse position, button state, and key input.
    //      If the handler returns non-zero it consumed the message; return
    //      immediately.
    //   2. Handle messages that require custom D3D11 / ImGui actions:
    //        WM_TIMER  — render a frame; validate the update region so no
    //                    redundant WM_PAINT is queued.
    //        WM_PAINT  — render a frame inside BeginPaint / EndPaint to
    //                    satisfy the paint validator.
    //        WM_SIZE   — resize the swap chain to the new client dimensions.
    //   3. Forward all other messages to the saved original WndProc.
    // -----------------------------------------------------------------------
    static LRESULT CALLBACK subclassProc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
    {
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

        auto* self = reinterpret_cast<ImGuiTaskPane*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        // Forward to ImGui's Win32 backend before any custom handling.
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
                // Suppress the default white background erase.  DX11 paints
                // the entire surface every frame, so erasing with the window
                // brush colour first would cause a white flash whenever the
                // pane grows (new pixels exposed before the next Present).
                return 1;

            case WM_TIMER:
                if (wParam == kTimerId)
                {
                    self->renderFrame();
                    // Validate the update region so a WM_PAINT is not
                    // queued immediately after each timer tick.
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
                        self->resize(w, h);
                }
                break;

            default:
                break;
            }
        }

        // Forward to the original WndProc.
        if (self && self->m_prevWndProc)
            return CallWindowProcW(self->m_prevWndProc, hwnd, msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

