//
// Created by kenne on 01/04/2026.
//

#pragma once

#include <windows.h>
#include <objbase.h>

static bool isDarkMode()
{
    DWORD value = 1;   // default: light
    DWORD size  = sizeof(value);

    // Windows dark mode: AppsUseLightTheme == 0 means dark.
    RegGetValueW(HKEY_CURRENT_USER,
                 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme",
                 RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (value == 0) return true;

    // Office UI theme: value 2 = Black (dark).
    value = 0;
    RegGetValueW(HKEY_CURRENT_USER,
                 L"SOFTWARE\\Microsoft\\Office\\16.0\\Common",
                 L"UI Theme",
                 RRF_RT_REG_DWORD, nullptr, &value, &size);
    return value == 2;
}