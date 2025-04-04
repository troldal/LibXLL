//
// Created by kenne on 18/03/2025.
//

#pragma once

#include "Base.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <xlcall.hpp>
#include <ranges>
#include <algorithm>

#ifndef _WIN32
#include <utf8.h>
#endif



namespace xll
{
    class String : public impl::Base<String, xltypeStr>
    {
        using BASE = impl::Base<String, xltypeStr>;

    public:
        using BASE::BASE;
        //using BASE::operator=;

        String() : String("") {}

        String(const char* str) : String(std::string_view(str)) {}    // NOLINT

        String(std::string_view str) : Base()    // NOLINT
        {
            value() = make_string(str).release();
        }

        explicit String(const XLOPER12& v)
        {
            switch (v.xltype == xltypeStr) {
                case true:
                    value() = make_string(to_string(v.val.str)).release();
                    break;
                default:
                    xltype = xltypeNil;
            }
        }

        String(const String& other)
        {
            switch (other.xltype == xltypeStr) {
                case true:
                    value() = make_string(to_string(other.val.str)).release();
                break;
                default:
                    xltype = xltypeNil;
            }
        }

        String(String&& other) noexcept
        {
            using std::swap;
            switch (other.xltype == xltypeStr) {
                case true:
                    val.str = other.val.str;
                    other.val.str = nullptr;
                    //value() = make_string(to_string(other.val.str)).release();
                    //swap(other, *this);
                break;
                default:
                    xltype = xltypeNil;
            }
        }

        ~String()
        {
            delete[] val.str;
            val.str = nullptr;
            xltype  = xltypeNil;
        }

        String& operator=(const String& other)
        {
            using std::swap;

            if (this == &other) return *this;
            delete[] val.str;
            val.str  = nullptr;

            switch (other.xltype == xltypeStr) {
                case true:
                    xltype = xltypeStr;
                    value() = make_string(to_string(other.val.str)).release();
                    break;
                default:
                    xltype = xltypeNil;
            }

            return *this;
        }

        String& operator=(String&& other) noexcept
        {
            using std::swap;

            if (this == &other) return *this;

            delete[] val.str;
            val.str  = nullptr;

            switch (other.xltype == xltypeStr) {
                case true:
                    xltype = xltypeStr;
                    value() = other.val.str;
                    other.val.str = nullptr;
                break;
                default:
                    xltype = xltypeNil;

            }

            return *this;
        }

        [[nodiscard]] std::string to_string() const
        {
            return to_string(val.str);
        }

        friend String operator+(const String& lhs, const String& rhs)
        {
            const std::string result = to_string(lhs.value()) + to_string(rhs.value());
            return {result};
        }

        template<typename TOther>
            requires (!std::same_as<String, TOther>) && std::convertible_to<TOther, std::string>
        friend String operator+(const String& lhs, TOther&& rhs)
        {
            const std::string result = to_string(lhs.value()) + rhs;
            return {result};
        }

        friend std::ostream& operator<<(std::ostream& os, const String& str)
        {
            if (str.xltype != xltypeStr) throw std::bad_cast();
            os << to_string(str.val.str);
            return os;
        }


        friend bool operator==(const String& lhs, const String& rhs) {
            return to_string(lhs.val.str) == to_string(rhs.val.str);
        }

        template<typename TOther>
            requires (!std::same_as<String, TOther>) && std::convertible_to<TOther, std::string>
        friend bool operator==(const String& lhs, TOther&& rhs)
        {
            return to_string(lhs.val.str) == rhs;
        }


        friend std::strong_ordering operator<=>(const String& lhs, const String& rhs)
        {
            return to_string(lhs.val.str) <=> to_string(rhs.val.str);
        }

        template<typename TOther>
            requires (!std::same_as<String, TOther>) && std::convertible_to<TOther, std::string>
        friend std::strong_ordering operator<=>(const String& lhs, TOther&& rhs)
        {
            return to_string(lhs.val.str) <=> rhs;
        }




    private:
        static std::string to_string(XCHAR const* str)
        {
#ifdef _WIN32

            if (str[0] == '\0') return "";

            const int size = WideCharToMultiByte(CP_UTF8,    // Code page
                                           0,          // Flags
                                           &str[1],    // Source wide string
                                           str[0],     // Source length
                                           nullptr,    // Destination buffer (null for size calculation)
                                           0,          // Destination buffer size
                                           nullptr,    // Default char
                                           nullptr     // Used default char flag
            );

            if (size == 0) throw std::runtime_error("String conversion failed");

            // Create the output string with the required size
            std::string text(size, '\0');

            // Convert the wide string to UTF-8
            if (WideCharToMultiByte(CP_UTF8, 0, &str[1], str[0], text.data(), size, nullptr, nullptr) == 0) {
                throw std::runtime_error("String conversion failed");
            }

            return text;
#else
            static_assert(sizeof(wchar_t) == sizeof(char32_t), "wchat_t and char32_t does not have the same size!");
            auto view = std::u32string(reinterpret_cast<const char32_t*>(&str[1]));
            auto result = utf8::utf32to8(view);
            return result;
#endif
        }

        static std::unique_ptr<XCHAR[]> make_string(std::string_view str)
        {
            if (str.empty()) {
                // Handle empty string case
                auto buffer = std::make_unique<XCHAR[]>(2);
                if (!buffer) throw std::bad_alloc();
                buffer[0] = 0;
                buffer[1] = 0;
                return buffer;
            }

#ifdef _WIN32
            // Get required buffer size
            auto data = str.data();
            auto sz   = MultiByteToWideChar(CP_UTF8, 0, data, static_cast<int>(str.size()), nullptr, 0);
            if (sz == 0) throw std::runtime_error("String conversion failed");

            // Allocate memory with proper size and error checking
            auto buffer = std::make_unique<XCHAR[]>(sz + 2);
            if (!buffer) throw std::bad_alloc();

            // Setup the Excel string format (length byte + contents + null terminator)
            buffer[0]      = static_cast<XCHAR>(sz);
            buffer[sz + 1] = 0;

            if (MultiByteToWideChar(CP_UTF8, 0, data, static_cast<int>(str.size()), &buffer[1], sz) == 0) {
                throw std::runtime_error("String conversion failed");
            }

            return buffer;
#else
            auto input = std::string(str);
            auto output = utf8::utf8to32(input);
            auto sz = output.size();

            // Allocate memory with proper size and error checking
            auto buffer = std::make_unique<XCHAR[]>(sz + 2);
            if (!buffer) throw std::bad_alloc();

            // Setup the Excel string format (length byte + contents + null terminator)
            buffer[0]      = static_cast<XCHAR>(sz);
            buffer[sz + 1] = 0;

            for (int idx = 1; auto c : output) {
                buffer[idx++] = static_cast<XCHAR>(c);
            }

            return buffer;
#endif
        }
    };

    namespace literals
    {
        inline String operator""_xs(const char* str, std::size_t len) { return String(std::string_view(str, len)); }
    }    // namespace literals

    inline xll::String trim(const xll::String& str)
    {
        auto s = str.to_string();

        // Find the first non-whitespace character.
        auto start = std::ranges::find_if_not(s, [](unsigned char ch) { return std::isspace(ch); });

        // If the string is entirely whitespace, return an empty string.
        if (start == s.end()) {
            return "";
        }

        // Find the last non-whitespace character.
        auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();

        // Construct the trimmed string.
        std::string result(start, end);

        return xll::String(result);
    }

    inline xll::String to_upper(const xll::String& str)
    {
        std::string result = str.to_string();
        std::ranges::transform(result, result.begin(), [](unsigned char c) {
            return std::toupper(c);
        });

        return xll::String(result);
    }

}    // namespace xll