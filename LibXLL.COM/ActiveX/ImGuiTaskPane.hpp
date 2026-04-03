// ---------------------------------------------------------------------------
// ImGuiTaskPane.hpp — Dear ImGui (Win32 + DirectX 11) content implementation
//                     for TaskPaneControl.
//
// This class satisfies the Content concept required by TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Has static void shutdown()
//   - Destructor handles cleanup
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
// with ImGui::SetCurrentContext() before every ImGui call.  This allows
// multiple panes (if any) to coexist without sharing context state.
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

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

// ImGui_ImplWin32_WndProcHandler is intentionally placed inside a #if 0
// guard in imgui_impl_win32.h to avoid pulling <windows.h> into every
// translation unit that includes the backend header.  Declare it explicitly
// here, after <windows.h> is already in scope.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <cmrc/cmrc.hpp>
#include "SetStyle.hpp"
#include <iostream>

CMRC_DECLARE(foo);

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
    // Constructor — creates the D3D11 device and swap chain on parentHwnd,
    // initialises Dear ImGui with the Win32 and DX11 backends, and subclasses
    // parentHwnd to intercept Win32 messages.  A WM_TIMER fires every ~16 ms
    // (~60 fps) to drive continuous rendering.
    // -----------------------------------------------------------------------
    ImGuiTaskPane(HWND parentHwnd, int w, int h)
        : m_hwnd(parentHwnd)
        , m_width(w > 0 ? w : 1)
        , m_height(h > 0 ? h : 1)
    {
        if (!createDevice())
        {
            std::cerr << "[xlCOM] ImGuiTaskPane: D3D11 device creation failed\n";
            return;
        }

        // Create a per-pane ImGui context so that multiple panes can coexist.
        IMGUI_CHECKVERSION();
        m_imguiCtx = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_imguiCtx);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Apply the Rest Dark colour scheme.
        SetStyleFluentWinUIDark();

        // Scale the style for the display DPI.
        const float dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(m_hwnd);
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);
        style.FontScaleDpi = dpiScale;

        // Load Inter Variable Font from the CMakeRC embedded resource.
        {
            auto fs   = cmrc::foo::get_filesystem();
            auto file = fs.open("Inter-VariableFont.ttf");
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;   // data lives in the static CMakeRC segment
            io.Fonts->AddFontFromMemoryTTF(
                const_cast<void*>(static_cast<const void*>(file.begin())),
                static_cast<int>(file.size()),
                16.0f,
                &cfg);
        }

        // Initialise platform and renderer backends.
        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(m_device, m_context);

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
    // Destructor — shuts down ImGui backends, releases D3D11 resources, and
    // restores the original WndProc on the container HWND.
    //
    // The timer is killed first so no WM_TIMER callbacks arrive after the
    // D3D11 or ImGui state has been torn down.
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

        // Shut down ImGui backends, then destroy the per-pane context.
        if (m_imguiCtx)
        {
            ImGui::SetCurrentContext(m_imguiCtx);
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(m_imguiCtx);
            m_imguiCtx = nullptr;
        }

        // Release D3D11 resources in reverse order of creation.
        cleanupRenderTarget();
        if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
        if (m_context)   { m_context->Release();   m_context   = nullptr; }
        if (m_device)    { m_device->Release();    m_device    = nullptr; }
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

    ID3D11Device*           m_device    = nullptr;
    ID3D11DeviceContext*    m_context   = nullptr;
    IDXGISwapChain*         m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv       = nullptr;

    ImGuiContext*           m_imguiCtx      = nullptr;
    bool                    m_pendingMsgBox = false;  // deferred dialog — see renderFrame()
    bool                    m_resizePending = false;  // true when resize() stored new dims
                                                      // but ResizeBuffers not yet called

    // -----------------------------------------------------------------------
    // createDevice — creates the D3D11 device and DXGI swap chain bound to
    // m_hwnd.  Falls back to the WARP software rasteriser if the hardware
    // adapter rejects the device (e.g. no discrete GPU, RDP session).
    // -----------------------------------------------------------------------
    bool createDevice()
    {
        DXGI_SWAP_CHAIN_DESC sd                          = {};
        sd.BufferCount                                   = 2;
        sd.BufferDesc.Format                             = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator              = 60;
        sd.BufferDesc.RefreshRate.Denominator            = 1;
        sd.Flags                                         = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage                                   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow                                  = m_hwnd;
        sd.SampleDesc.Count                              = 1;
        sd.SampleDesc.Quality                            = 0;
        sd.Windowed                                      = TRUE;
        sd.SwapEffect                                    = DXGI_SWAP_EFFECT_DISCARD;

        const D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };
        D3D_FEATURE_LEVEL featureLevel = {};

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            featureLevels, 2, D3D11_SDK_VERSION,
            &sd, &m_swapChain, &m_device, &featureLevel, &m_context);

        if (hr == DXGI_ERROR_UNSUPPORTED)  // fall back to software rasteriser
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                featureLevels, 2, D3D11_SDK_VERSION,
                &sd, &m_swapChain, &m_device, &featureLevel, &m_context);

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
            m_device->CreateRenderTargetView(pBack, nullptr, &m_rtv);
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
    // -----------------------------------------------------------------------
    void renderFrame()
    {
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

        if (!m_device || !m_rtv || !m_imguiCtx) return;

        ImGui::SetCurrentContext(m_imguiCtx);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ---- Fullscreen pane window -----------------------------------------
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        constexpr ImGuiWindowFlags kPaneFlags =
            ImGuiWindowFlags_NoTitleBar          |
            ImGuiWindowFlags_NoResize            |
            ImGuiWindowFlags_NoMove              |
            ImGuiWindowFlags_NoScrollbar         |
            ImGuiWindowFlags_NoCollapse          |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##MainPane", nullptr, kPaneFlags);

        //const bool tooSmall = (io.DisplaySize.x < 400.0f || io.DisplaySize.y < 600.0f);

        // Open the "too small" popup whenever the canvas shrinks below the
        // minimum, and keep it open while the condition holds.
        //if (tooSmall && !ImGui::IsPopupOpen("##TooSmall"))
        //    ImGui::OpenPopup("##TooSmall");

        //ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
        //                        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        // if (ImGui::BeginPopupModal("##TooSmall", nullptr,
        //                            ImGuiWindowFlags_AlwaysAutoResize |
        //                            ImGuiWindowFlags_NoDecoration))
        // {
        //     if (!tooSmall)
        //         ImGui::CloseCurrentPopup();   // canvas grew back — auto-dismiss
        //     else
        //         ImGui::Text("Canvas too small!");
        //     ImGui::EndPopup();
        // }

        // Normal content — only rendered while the canvas is large enough.
        //if (!tooSmall)
        //{
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            const float  btnW  = ImGui::CalcTextSize("Show Message").x
                               + ImGui::GetStyle().FramePadding.x * 2.0f;
            const float  btnH  = ImGui::GetFrameHeight();
            ImGui::SetCursorPos(ImVec2((avail.x - btnW) * 0.5f,
                                       (avail.y - btnH) * 0.5f));
            if (ImGui::Button("Show Message"))
                m_pendingMsgBox = true;
        //}

        ImGui::End();
        // --------------------------------------------------------------------

        ImGui::Render();

        // Clear colour matches ImGuiCol_WindowBg from SetStyleRestDark.
        constexpr float kClearColor[] = { 0.09411765f, 0.09411765f, 0.09411765f, 1.0f };
        m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
        m_context->ClearRenderTargetView(m_rtv, kClearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_swapChain->Present(1, 0);  // present with vsync

        // Defer any modal dialog until AFTER Render()/Present() so that the
        // Win32 message loop inside MessageBoxW can only trigger renderFrame()
        // for a fresh new frame — not re-enter while a frame is still open.
        if (m_pendingMsgBox)
        {
            m_pendingMsgBox = false;
            MessageBoxW(m_hwnd,
                        L"Hello from the ImGui task pane!",
                        L"XLThermo",
                        MB_OK | MB_ICONINFORMATION);
        }
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
        auto* self = reinterpret_cast<ImGuiTaskPane*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        // Forward to ImGui's Win32 backend before any custom handling.
        if (self && self->m_imguiCtx)
        {
            ImGui::SetCurrentContext(self->m_imguiCtx);
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

