// ImGuiDemoDialog.hpp
//
// Self-contained modal window (800 x 1200) that hosts the Dear ImGui built-in
// demo via ImGui::ShowDemoWindow().
//
// Follows the same Win32 / D3D11 / per-instance ImGuiContext pattern as
// ImGuiGreetingDialog.  Prerequisites: include ImGuiGreetingDialog.hpp (which
// provides NativeOwnerModal, D3D11Block, dll_handle, applyStyle, etc.) before
// including this header.

#pragma once

class ImGuiDemoDialog
{
public:
    /// Opens an 800 x 1200 modal window running the ImGui built-in demo.
    /// Returns when the user closes the Win32 window or the ImGui demo window.
    static void show(HWND owner)
    {
        ImGuiDemoDialog dlg;
        if (!dlg.create()) return;

        NativeOwnerModal modal(dlg.m_hwnd, owner);

        ShowWindow(dlg.m_hwnd, SW_SHOW);
        UpdateWindow(dlg.m_hwnd);

        dlg.runModal();
    }

private:
    static constexpr UINT_PTR kTimerId = 0xCBA4;
    static constexpr wchar_t  kClass[] = L"ImGuiDemoDlg";

    HWND          m_hwnd          = nullptr;
    D3D11Block    m_d3d;
    ImGuiContext* m_ctx           = nullptr;
    bool          m_done          = false;
    bool          m_show          = true;   // ImGui demo window open flag —
                                            // set to false by ShowDemoWindow
                                            // when user clicks its close button
    int           m_width         = 800;
    int           m_height        = 1200;
    bool          m_resizePending = false;

    [[nodiscard]] bool create()
    {
        ensureClass();

        m_hwnd = CreateWindowExW(
            0, kClass, L"Dear ImGui Demo",
            WS_OVERLAPPEDWINDOW,
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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename  = nullptr;

        applyStyle(m_hwnd);

        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(m_d3d.device, m_d3d.context);

        SetTimer(m_hwnd, kTimerId, 16, nullptr);
        return true;
    }

    ~ImGuiDemoDialog()
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

        m_d3d.destroy();
        if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
    }

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

        ImGui::ShowDemoWindow(&m_show);
        if (!m_show) m_done = true;

        ImGui::Render();

        // #292929 — Excel dark-mode background
        constexpr float kClear[] = { 0.1608f, 0.1608f, 0.1608f, 1.0f };
        m_d3d.context->OMSetRenderTargets(1, &m_d3d.rtv, nullptr);
        m_d3d.context->ClearRenderTargetView(m_d3d.rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_d3d.swapChain->Present(1, 0);
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
        RegisterClassExW(&wc);   // succeeds, or ERROR_CLASS_ALREADY_EXISTS — both fine
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

        auto* self = reinterpret_cast<ImGuiDemoDialog*>(
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
                // Suppress the default erase; DX11 clears the surface every frame.
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
                // Don't let the OS destroy the window; we manage its lifetime.
                self->m_done = true;
                return 0;

            default: break;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

