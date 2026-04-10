// ---------------------------------------------------------------------------
// LicenseActivationContent.hpp — License activation dialog for ImGui.
//
// Translates the WPF LicenseActivationUI.xaml to Dear ImGui.  Satisfies the
// WindowContent concept and can be hosted in ImGuiModalWindow.
//
// Layout (matching the WPF original):
//   Header        — "XLThermo" title text
//   Description   — three explanatory paragraphs
//   Separator
//   Form fields   — Email, License Key (with validation dots), Device ID
//   Progress bar  — indeterminate, shown during operations
//   Action btns   — Activate / Deactivate (right-aligned)
//   Status        — coloured status, license type, expiry date
//   Footer        — hyperlink + Close button
// ---------------------------------------------------------------------------

#pragma once

#include "imgui.h"
#include "SetStyle.hpp"              // ExcelButton, HighlightedExcelButton
#include "ImGuiWindowContent.hpp"    // FrameAction

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include "picosha2.h"

struct LicenseActivationContent
{
    LicenseActivationContent()
    {
        const std::string guid = readMachineGuid();
        std::string hex = picosha2::hash256_hex_string(guid);
        std::transform(hex.begin(), hex.end(), hex.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        std::snprintf(m_deviceId, sizeof(m_deviceId), "%s", hex.c_str());
    }

    FrameAction renderContent()
    {
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);

        constexpr ImGuiWindowFlags kFlags =
            ImGuiWindowFlags_NoTitleBar            |
            ImGuiWindowFlags_NoResize              |
            ImGuiWindowFlags_NoMove                |
            ImGuiWindowFlags_NoCollapse            |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("##LicenseActivation", nullptr, kFlags);

        // Tick the pending-activation timer each frame.
        updatePendingActivation();

        renderHeader();
        renderDescription();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        renderInputFields();
        ImGui::Spacing();
        // renderActionButtons();

        ImGui::Spacing();
        ImGui::Spacing();

        renderStatusSection();

        ImGui::Spacing();
        ImGui::Spacing();

        renderFooter();

        ImGui::End();

        return m_requestClose ? FrameAction::RequestClose : FrameAction::Continue;
    }

private:
    // ---- State --------------------------------------------------------------
    char m_email[256]      = "";
    char m_licenseKey[256] = "";
    char m_deviceId[65]    = {};

    bool m_showProgress = false;
    bool m_inputEnabled = true;
    bool m_requestClose = false;

    enum class Status { NotActivated, Pending, Activated, Error };
    Status      m_status      = Status::NotActivated;
    const char* m_licenseType = "Basic";
    const char* m_expiryDate  = "N/A";
    double      m_activateStartTime = 0.0;
    static constexpr double kActivateDelaySec = 5.0;
    static constexpr float  kDotCol           = 22.0f;

    // Mouse-stationarity tracking for the Device ID tooltip.
    ImVec2 m_devIdTooltipMousePos       = { -1.0f, -1.0f };
    double m_devIdTooltipStationarySince = 0.0;
    static constexpr double kTooltipDelaySec = 0.5;

    // ---- Validation ---------------------------------------------------------
    enum class Validity { Unknown, Valid, Invalid };

    Validity validateEmail() const
    {
        if (m_email[0] == '\0') return Validity::Unknown;
        return (std::strchr(m_email, '@') && std::strchr(m_email, '.'))
                   ? Validity::Valid
                   : Validity::Invalid;
    }

    Validity validateKey() const
    {
        if (m_licenseKey[0] == '\0') return Validity::Unknown;
        return Validity::Valid;
    }

    // ---- Drawing helpers ----------------------------------------------------

    // Reads HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid.
    static std::string readMachineGuid()
    {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Cryptography",
                          0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
            return {};

        wchar_t buf[64] = {};
        DWORD   size    = sizeof(buf);
        DWORD   type    = REG_SZ;
        const LONG res  = RegQueryValueExW(hKey, L"MachineGuid", nullptr, &type,
                                           reinterpret_cast<LPBYTE>(buf), &size);
        RegCloseKey(hKey);
        if (res != ERROR_SUCCESS) return {};

        char narrow[64] = {};
        WideCharToMultiByte(CP_UTF8, 0, buf, -1, narrow, sizeof(narrow), nullptr, nullptr);
        return narrow;
    }

    // Mirrors the WPF InputUI status rectangle (10 × 10, filled + black stroke).
    static void drawStatusDot(Validity v)
    {
        ImU32 fill;
        switch (v)
        {
            case Validity::Valid:   fill = IM_COL32(0, 200, 0, 255);   break;
            case Validity::Invalid: fill = IM_COL32(220, 40, 40, 255); break;
            default:                fill = IM_COL32(128, 128, 128, 255); break;
        }

        constexpr float kSize = 10.0f;
        const ImVec2 pos  = ImGui::GetCursorScreenPos();
        const float  yOff = (ImGui::GetFrameHeight() - kSize) * 0.5f;
        const ImVec2 pMin(pos.x + 2.0f, pos.y + yOff);
        const ImVec2 pMax(pMin.x + kSize, pMin.y + kSize);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pMin, pMax, fill);
        dl->AddRect(pMin, pMax, IM_COL32(0, 0, 0, 180));

        ImGui::Dummy(ImVec2(kSize + 4.0f, ImGui::GetFrameHeight()));
    }

    // Right-aligns a label within its current content region (like
    // HorizontalContentAlignment="Right" in WPF).
    static void rightAlignedLabel(const char* text)
    {
        const float textW = ImGui::CalcTextSize(text).x;
        const float colW  = ImGui::GetContentRegionAvail().x;
        if (colW > textW)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + colW - textW);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text);
    }

    // Draws a clickable hyperlink with underline-on-hover.
    static bool hyperlink(const char* label)
    {
        const ImVec4 linkCol(0.0f, 0.47f, 0.84f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, linkCol);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        const bool hovered = ImGui::IsItemHovered();
        if (hovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const ImVec2 mn = ImGui::GetItemRectMin();
            const ImVec2 mx = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(mn.x, mx.y), ImVec2(mx.x, mx.y),
                ImGui::ColorConvertFloat4ToU32(linkCol));
        }
        return ImGui::IsItemClicked();
    }

    // ---- Sections -----------------------------------------------------------

    void renderHeader()
    {
        ImGui::Spacing();

        // Scaled-up title (mimics FontSize="36" / AnitaSemiSquare in the WPF).
        // PushFont(NULL, size) keeps the current font and changes the base size.
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.2f);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        ImGui::TextUnformatted("XLThermo");

        ImGui::PopFont();

        ImGui::Spacing();
    }

    void renderDescription()
    {
        const float indent = 20.0f;
        ImGui::Indent(indent);

        ImGui::TextWrapped(
            "If you only intend to use XLThermo Basic, there is no need "
            "to purchase a license and to activate your device.");
        ImGui::Spacing();

        ImGui::TextWrapped(
            "If you have purchased a license, please enter the registered "
            "email address and the license key below. One license can be "
            "used for activating XLThermo on up to three different devices.");
        ImGui::Spacing();

        ImGui::TextWrapped(
            "For one-time purchases, the activation is only required once "
            "for every installation. For subscriptions, the activation will "
            "need to be re-confirmed regularly. This, however, will happen "
            "automatically in the background.");

        ImGui::Unindent(indent);
    }

    void renderInputFields()
    {
        const float kLabelCol = ImGui::CalcTextSize("License Expiry:").x
                              + ImGui::GetStyle().FramePadding.x * 2.0f;

        if (!ImGui::BeginTable("##inputs", 3, ImGuiTableFlags_None))
            return;

        ImGui::TableSetupColumn("Label",  ImGuiTableColumnFlags_WidthFixed,   kLabelCol);
        ImGui::TableSetupColumn("Input",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,   kDotCol);

        const ImGuiInputTextFlags roFlag =
            m_inputEnabled ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_ReadOnly;

        // Email Address
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); rightAlignedLabel("Email Address:");
        ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##email", "user@example.com",
                                 m_email, sizeof(m_email), roFlag);
        ImGui::TableNextColumn(); drawStatusDot(validateEmail());

        // License Key
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); rightAlignedLabel("License Key:");
        ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##key", "XXXXXX-XXXXXX-XXXXXX-XXXXXX-XX",
                                 m_licenseKey, sizeof(m_licenseKey), roFlag);
        ImGui::TableNextColumn(); drawStatusDot(validateKey());

        // Device ID (disabled: visually dimmed and non-interactive)
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); rightAlignedLabel("Device ID:");
        ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::BeginDisabled();
        ImGui::InputText("##devid", m_deviceId, sizeof(m_deviceId));
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            const ImVec2 mouse = ImGui::GetMousePos();
            const float  dx    = mouse.x - m_devIdTooltipMousePos.x;
            const float  dy    = mouse.y - m_devIdTooltipMousePos.y;
            if (dx * dx + dy * dy > 4.0f) // moved more than ~2 px — reset timer
            {
                m_devIdTooltipMousePos        = mouse;
                m_devIdTooltipStationarySince = ImGui::GetTime();
            }
            if (ImGui::GetTime() - m_devIdTooltipStationarySince >= kTooltipDelaySec)
            {
                const ImVec2 pad = ImGui::GetStyle().WindowPadding;
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                                    ImVec2(pad.x, pad.y * 0.5f));
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("A unique device identifier.");
                ImGui::EndTooltip();
                ImGui::PopStyleVar();
            }
        }
        else
        {
            // Cursor left the item — reset so the delay applies fresh next time.
            m_devIdTooltipMousePos        = { -1.0f, -1.0f };
            m_devIdTooltipStationarySince = ImGui::GetTime();
        }
        ImGui::TableNextColumn(); // empty

        // Progress bar row — always present to keep layout stable.
        // The bar is only visible while activation is in progress.
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); // skip label
            ImGui::TableNextColumn();
            if (m_showProgress)
            {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                                      ImVec4(0.0f, 0.7f, 0.25f, 1.0f));
                ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()),
                                   ImVec2(-FLT_MIN, 10.0f), "");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Dummy(ImVec2(-FLT_MIN, 10.0f));
            }
            ImGui::TableNextColumn(); // skip dot
        }

        // Action buttons — placed in the Input column so their right edge
        // aligns with the input fields automatically, with no pixel math.
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); // skip label
            ImGui::TableNextColumn();

            const float kBtnW = ImGui::CalcTextSize("Deactivate").x
                              + ImGui::GetStyle().FramePadding.x * 2.0f;
            const float spacing   = ImGui::GetStyle().ItemSpacing.x;
            const float totalW    = kBtnW * 2.0f + spacing;
            const float availW    = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availW - totalW);

            const bool pending = (m_status == Status::Pending);
            if (pending) ImGui::BeginDisabled();

            if (HighlightedExcelButton("Activate", ImVec2(kBtnW, 0.0f)))
                onActivate();
            ImGui::SameLine();
            if (ExcelButton("Deactivate", ImVec2(kBtnW, 0.0f)))
                onDeactivate();

            if (pending) ImGui::EndDisabled();

            ImGui::TableNextColumn(); // skip dot
        }

        ImGui::EndTable();
    }

    void renderStatusSection()
    {
        const float kLabelCol = ImGui::CalcTextSize("License Expiry:").x
                              + ImGui::GetStyle().FramePadding.x * 2.0f;

        if (!ImGui::BeginTable("##status", 2, ImGuiTableFlags_None))
            return;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed,   kLabelCol);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Status (coloured text, matching WPF StatusUI)
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); rightAlignedLabel("Status:");
        ImGui::TableNextColumn();
        {
            ImVec4      col;
            const char* text;
            switch (m_status)
            {
                case Status::Pending:
                    col  = ImVec4(0.85f, 0.65f, 0.0f, 1.0f);
                    text = "Activating...";
                    break;
                case Status::Activated:
                    col  = ImVec4(0.0f, 0.7f, 0.3f, 1.0f);
                    text = "Activated";
                    break;
                case Status::Error:
                    col  = ImVec4(0.85f, 0.0f, 0.0f, 1.0f);
                    text = "Activation Error";
                    break;
                default:
                    col  = ImVec4(0.85f, 0.0f, 0.0f, 1.0f);
                    text = "Not Activated";
                    break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }

        // License Type
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); rightAlignedLabel("License Type:");
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(m_licenseType);

        // License Expiry
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); rightAlignedLabel("License Expiry:");
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(m_expiryDate);

        ImGui::EndTable();
    }

    void renderFooter()
    {
        // Hyperlink (left-aligned with indent)
        ImGui::Indent(20.0f);
        if (hyperlink("Go to license administration console"))
        {
            ShellExecuteW(nullptr, L"open",
                          L"https://admin.xlthermo.com",
                          nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::Unindent(20.0f);

        // Close button — placed in a table matching the input fields layout so
        // its right edge aligns exactly with the Activate / Deactivate buttons.
        ImGui::Spacing();

        const float kLabelCol = ImGui::CalcTextSize("License Expiry:").x
                              + ImGui::GetStyle().FramePadding.x * 2.0f;

        if (!ImGui::BeginTable("##footer", 3, ImGuiTableFlags_None))
            return;

        ImGui::TableSetupColumn("Label",  ImGuiTableColumnFlags_WidthFixed,   kLabelCol);
        ImGui::TableSetupColumn("Input",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,   kDotCol);

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); // skip label
        ImGui::TableNextColumn();

        const float kBtnW  = ImGui::CalcTextSize("Deactivate").x
                           + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float availW = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availW - kBtnW);
        if (ExcelButton("Close", ImVec2(kBtnW, 0.0f)))
            m_requestClose = true;

        ImGui::TableNextColumn(); // skip dot

        ImGui::EndTable();
    }

    // ---- Actions (demo stubs) -----------------------------------------------

    // Called every frame; completes the activation after the delay elapses.
    void updatePendingActivation()
    {
        if (m_status != Status::Pending) return;

        if (ImGui::GetTime() - m_activateStartTime >= kActivateDelaySec)
        {
            m_status        = Status::Activated;
            m_showProgress  = false;
            m_inputEnabled  = true;
            m_licenseType   = "Professional";
            m_expiryDate    = "2027-01-01";
        }
    }

    void onActivate()
    {
        if (validateEmail() != Validity::Valid ||
            m_licenseKey[0] == '\0')
        {
            m_status = Status::Error;
            return;
        }

        // Begin the 5-second simulated activation.
        m_status            = Status::Pending;
        m_showProgress      = true;
        m_inputEnabled      = false;
        m_activateStartTime = ImGui::GetTime();
    }

    void onDeactivate()
    {
        m_status        = Status::NotActivated;
        m_licenseType   = "Basic";
        m_expiryDate    = "N/A";
        m_email[0]      = '\0';
        m_licenseKey[0] = '\0';
    }
};


