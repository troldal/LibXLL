//
// Created by kenne on 31/03/2026.
//

#pragma once

#include <objbase.h>

// XLCOM_CONNECT_GUID is injected by the build system (CMakeLists.txt) as a
// string literal, e.g. "1E0739E0-A1B5-4AB4-953C-2D8959196A13".
// The constexpr parser below converts it to a GUID struct at compile time,
// which works on MSVC, Clang-CL, and MinGW/GCC without __declspec(uuid).

namespace detail
{
    namespace impl {
        constexpr unsigned char guidHex(char c) noexcept
        {
            return (c >= '0' && c <= '9') ? static_cast<unsigned char>(c - '0')      :
                   (c >= 'a' && c <= 'f') ? static_cast<unsigned char>(c - 'a' + 10) :
                                            static_cast<unsigned char>(c - 'A' + 10);
        }
        constexpr unsigned char guidByte(const char* s, int i) noexcept
        {
            return static_cast<unsigned char>((guidHex(s[i]) << 4) | guidHex(s[i + 1]));
        }
        constexpr unsigned short guidWord(const char* s, int i) noexcept
        {
            return static_cast<unsigned short>(
                (static_cast<unsigned>(guidHex(s[i]))     << 12) | (static_cast<unsigned>(guidHex(s[i + 1])) << 8) |
                (static_cast<unsigned>(guidHex(s[i + 2])) << 4)  |  static_cast<unsigned>(guidHex(s[i + 3])));
        }
        constexpr unsigned long guidDword(const char* s, int i) noexcept
        {
            return (static_cast<unsigned long>(guidHex(s[i]))     << 28) | (static_cast<unsigned long>(guidHex(s[i + 1])) << 24) |
                   (static_cast<unsigned long>(guidHex(s[i + 2])) << 20) | (static_cast<unsigned long>(guidHex(s[i + 3])) << 16) |
                   (static_cast<unsigned long>(guidHex(s[i + 4])) << 12) | (static_cast<unsigned long>(guidHex(s[i + 5])) << 8)  |
                   (static_cast<unsigned long>(guidHex(s[i + 6])) << 4)  |  static_cast<unsigned long>(guidHex(s[i + 7]));
        }
    }
    // Parses "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" into a GUID.
    constexpr CLSID parseGUID(const char* s) noexcept
    {
        return CLSID{
            impl::guidDword(s,  0),
            impl::guidWord (s,  9),
            impl::guidWord (s, 14),
            {
                impl::guidByte(s, 19), impl::guidByte(s, 21),
                impl::guidByte(s, 24), impl::guidByte(s, 26),
                impl::guidByte(s, 28), impl::guidByte(s, 30),
                impl::guidByte(s, 32), impl::guidByte(s, 34)
            }
        };
    }
} // namespace detail