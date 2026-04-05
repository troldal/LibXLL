// ImGuiGreetingDialog.hpp
//
// Shared implementation of ImGuiGreetingDialog — a self-contained modal
// "Enter your name" dialog built on Dear ImGui + Win32 + Direct3D 11.
//
// Also provides the Win32/D3D11/ImGui infrastructure helpers that are common
// across the ImGui demo files:
//   NativeOwnerSetup, NativeOwnerModal — owner-window wiring and modal RAII
//   D3D11Block                         — D3D11 device / swap-chain wrapper
//   dll_handle()                       — HINSTANCE of the hosting DLL
//   IsWindowsDarkMode() / ApplyWindowTheme() — Windows dark-mode helpers
//   applyStyle()                       — per-window DPI scaling + font load
//
// Prerequisites: define WIN32_LEAN_AND_MEAN before including this header (or
// let the header define it via the #ifndef guard below).

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>

#include <array>
#include <string>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "SetStyle.hpp"

// ============================================================================
// NativeOwnerSetup / NativeOwnerModal
//
// Framework-agnostic Win32 helpers: wire a window to a foreign owner, centre
// it, and (for modal dialogs) disable/re-enable the owner with RAII.
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
// D3D11Block — thin RAII wrapper for the D3D11 device, context, swap chain,
// and render-target view used by each ImGui window.
// ============================================================================

struct D3D11Block
{
    ID3D11Device*           device    = nullptr;
    ID3D11DeviceContext*    context   = nullptr;
    IDXGISwapChain*         swapChain = nullptr;
    ID3D11RenderTargetView* rtv       = nullptr;

    [[nodiscard]] bool create(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC sd   = {};
        sd.BufferCount            = 2;
        sd.BufferDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate = { 60, 1 };
        sd.Flags                  = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage            = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow           = hwnd;
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
            &sd, &swapChain, &device, &fl, &context);

        if (hr == DXGI_ERROR_UNSUPPORTED)   // fall back to software rasteriser
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                kLevels, 2, D3D11_SDK_VERSION,
                &sd, &swapChain, &device, &fl, &context);

        if (FAILED(hr)) return false;
        createRenderTarget();
        return true;
    }

    void createRenderTarget()
    {
        ID3D11Texture2D* back = nullptr;
        swapChain->GetBuffer(0, IID_PPV_ARGS(&back));
        if (back)
        {
            device->CreateRenderTargetView(back, nullptr, &rtv);
            back->Release();
        }
    }

    void cleanupRenderTarget()
    {
        if (rtv) { rtv->Release(); rtv = nullptr; }
    }

    void resize(UINT w, UINT h)
    {
        cleanupRenderTarget();
        swapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
        createRenderTarget();
    }

    void destroy()
    {
        cleanupRenderTarget();
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
        if (context)   { context->Release();   context   = nullptr; }
        if (device)    { device->Release();    device    = nullptr; }
    }
};

// ============================================================================
// dll_handle — returns the HINSTANCE of the XLL DLL, used for Win32 window
// class registration so the class belongs to the DLL, not to Excel.exe.
// ============================================================================

inline HINSTANCE dll_handle()
{
    HMODULE mod = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&dll_handle),
        &mod);
    return mod ? mod : GetModuleHandleW(nullptr);
}

// ============================================================================
// Windows dark-mode helpers
// ============================================================================

// Returns true when the user has selected "dark" in Windows Settings →
// Personalisation → Colors.  Reads the standard registry value:
//   HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\Personalize
//     AppsUseLightTheme  REG_DWORD  0 = dark, 1 = light (default)
inline bool IsWindowsDarkMode()
{
    DWORD value = 1;   // default: light
    DWORD size  = sizeof(value);
    RegGetValueW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &size);
    return value == 0;
}

// Tells DWM to tint the non-client area (title bar, caption buttons) to match
// the current Windows colour scheme.  Must be called after CreateWindowExW.
//
// DWMWA_USE_IMMERSIVE_DARK_MODE (20) is available from Windows 10 build 18985
// onwards (all retail Windows 10 20H1+ and Windows 11).
inline void ApplyWindowTheme(HWND hwnd)
{
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
    BOOL dark = IsWindowsDarkMode() ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

// ============================================================================
// applyStyle — applies SetStyleExcelDark(), scales metrics and font for the
// window's DPI, and loads Segoe UI (falls back to ImGui's built-in font).
//
// Must be called after ImGui::CreateContext() and before ImGui_ImplWin32_Init /
// ImGui_ImplDX11_Init, with the target ImGuiContext already current.
// ============================================================================

inline void applyStyle(HWND hwnd)
{
    SetStyleExcelDark();

    const float  dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    ImGuiStyle&  style    = ImGui::GetStyle();
    style.ScaleAllSizes(dpiScale);
    style.FontScaleDpi = dpiScale;

    constexpr const char* kSegoeUI = "C:\\Windows\\Fonts\\segoeui.ttf";
    if (GetFileAttributesA(kSegoeUI) != INVALID_FILE_ATTRIBUTES)
        ImGui::GetIO().Fonts->AddFontFromFileTTF(kSegoeUI, 18.0f);
}

// ============================================================================
// ImGuiGreetingDialog — modal "Enter your name" dialog
//
// show() creates a Win32 window, initialises D3D11 and a per-instance ImGui
// context, applies NativeOwnerModal semantics (disables the Excel window,
// centres the dialog), then runs a local message loop.  Returns the user's
// input on OK or an empty string on Cancel / close.
// ============================================================================

class ImGuiGreetingDialog
{
public:
    [[nodiscard]] static std::string show(HWND owner)
    {
        ImGuiGreetingDialog dlg;
        if (!dlg.create()) return {};

        NativeOwnerModal modal(dlg.m_hwnd, owner);

        ShowWindow(dlg.m_hwnd, SW_SHOW);
        UpdateWindow(dlg.m_hwnd);

        return dlg.runModal();
    }

private:
    static constexpr UINT_PTR kTimerId = 0xCBA2;
    static constexpr wchar_t  kClass[] = L"ImGuiGreetingDlg";

    HWND          m_hwnd     = nullptr;
    D3D11Block    m_d3d;
    ImGuiContext* m_ctx      = nullptr;
    bool          m_ok       = false;
    bool          m_done     = false;
    bool          m_focusSet = false;

    std::array<char, 256> m_buf{};

    [[nodiscard]] bool create()
    {
        ensureClass();

        m_hwnd = CreateWindowExW(
            0, kClass, L"Dear ImGui inside an XLL",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 540, 225,
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

    ~ImGuiGreetingDialog()
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

    [[nodiscard]] std::string runModal()
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
        return m_ok ? std::string(m_buf.data()) : std::string{};
    }

    void renderFrame()
    {
        if (!m_ctx || !m_d3d.rtv) return;

        ImGui::SetCurrentContext(m_ctx);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        constexpr ImGuiWindowFlags kFlags =
            ImGuiWindowFlags_NoTitleBar            |
            ImGuiWindowFlags_NoResize              |
            ImGuiWindowFlags_NoMove                |
            ImGuiWindowFlags_NoScrollbar           |
            ImGuiWindowFlags_NoCollapse            |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##DlgMain", nullptr, kFlags);

        constexpr float kExtraRowGap = 10.0f;
        const float itemH   = ImGui::GetFrameHeight();
        const float spacing = ImGui::GetStyle().ItemSpacing.y;
        const float totalH  = itemH + spacing + kExtraRowGap + itemH;
        ImGui::SetCursorPosY((io.DisplaySize.y - totalH) * 0.5f);

        ImGui::Text("Enter your name:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(
            io.DisplaySize.x
            - ImGui::GetCursorPosX()
            - ImGui::GetStyle().WindowPadding.x);

        if (!m_focusSet)
        {
            ImGui::SetKeyboardFocusHere();
            m_focusSet = true;
        }

        const bool enter = ImGui::InputText(
            "##Name", m_buf.data(), m_buf.size(),
            ImGuiInputTextFlags_EnterReturnsTrue);

        // Right-align buttons so Cancel's right edge matches InputText's right edge.
        // InputText right edge = io.DisplaySize.x - WindowPadding.x
        // => OK starts at:  displayW - padding - 2*btnW - gap
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + kExtraRowGap);
        constexpr float kBtnW = 100.0f;
        constexpr float kGap  = 10.0f;
        ImGui::SetCursorPosX(
            io.DisplaySize.x
            - ImGui::GetStyle().WindowPadding.x
            - kBtnW * 2.0f
            - kGap);

        if (ExcelButton("OK", ImVec2(kBtnW, 0)) || enter)
        {
            m_ok   = true;
            m_done = true;
        }
        ImGui::SameLine(0.0f, kGap);
        if (HighlightedExcelButton("Cancel", ImVec2(kBtnW, 0)))
        {
            m_ok   = false;
            m_done = true;
        }

        ImGui::End();
        ImGui::Render();

        constexpr float kClear[] = { 0.1216f, 0.1216f, 0.1216f, 1.0f };
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

        auto* self = reinterpret_cast<ImGuiGreetingDialog*>(
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
                    self->m_d3d.resize(LOWORD(lParam), HIWORD(lParam));
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
