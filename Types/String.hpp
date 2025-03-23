//
// Created by kenne on 18/03/2025.
//

#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <xlcall.h>

namespace xll
{
    class String;
    inline void swap(String& lhs, String& rhs) noexcept;

    class String : public XLOPER12
    {
    public:
        using XLOPER12::val;
        using XLOPER12::xltype;

        // Default constructor
        String() : String("") {}

        String(const char* str) : String(std::string_view(str)) {}  // NOLINT

        String(std::string_view str) : XLOPER12()  // NOLINT
        {
            xltype  = xltypeStr;
            val.str = make_string(str).release();
        }

        explicit String(const XLOPER12& v) : XLOPER12()
        {
            if (v.xltype != xltypeStr) {
                xltype = xltypeNil;
                return;
            }

            std::string str = to_string(v.val.str);
            xltype          = xltypeStr;
            val.str         = make_string(str).release();
        }

        String(const String& other) : XLOPER12(static_cast<XLOPER12>(other))
        {
            if (other.xltype != xltypeStr) {
                xltype = xltypeNil;
                return;
            }

            std::string str = other;
            xltype          = xltypeStr;
            val.str         = make_string(str).release();
        }

        String(String&& other) noexcept : XLOPER12()
        {
            if (other.xltype != xltypeStr) {
                xltype = xltypeNil;
                return;
            }

            using std::swap;
            swap(other, *this);
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

            if (other.xltype != xltypeStr)
                xltype = xltypeNil;
            else {
                auto rhs = other;
                swap(rhs, *this);
            }
            return *this;
        }

        String& operator=(String&& other) noexcept
        {
            using std::swap;
            if (this == &other) return *this;

            if (other.xltype != xltypeStr) {
                xltype = xltypeNil;
                delete[] val.str;
                val.str  = nullptr;
            }
            else {
                swap(other, *this);
                other.val.str = nullptr;
            }
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& os, const String& str)
        {
            if (str.xltype != xltypeStr) throw std::bad_cast();
            os << static_cast<std::string>(str);
            return os;
        }

        operator std::string() const    // NOLINT
        {
            if (xltype != xltypeStr) throw std::bad_cast();

            // Get the required buffer size
            int size = WideCharToMultiByte(CP_UTF8,        // Code page
                                           0,              // Flags
                                           &val.str[1],    // Source wide string
                                           val.str[0],     // Source length
                                           nullptr,        // Destination buffer (null for size calculation)
                                           0,              // Destination buffer size
                                           nullptr,        // Default char
                                           nullptr         // Used default char flag
            );

            if (size == 0) throw std::runtime_error("String conversion failed");

            // Create the output string with the required size
            std::string text(size, '\0');

            // Convert the wide string to UTF-8
            if (WideCharToMultiByte(CP_UTF8, 0, &val.str[1], val.str[0], text.data(), size, nullptr, nullptr) == 0) {
                throw std::runtime_error("String conversion failed");
            }

            return text;
        }

        [[nodiscard]]
        std::optional<String> optional() const
        {
            if (xltype != xltypeStr) return std::nullopt;
            return *this;
        }

        [[nodiscard]]
        bool has_value() const { return xltype == xltypeStr; }

        explicit operator bool() const { return xltype == xltypeStr; }

    private:

        static std::string to_string(XCHAR const* str)
        {
            int size = WideCharToMultiByte(CP_UTF8,        // Code page
                               0,              // Flags
                               &str[1],    // Source wide string
                               str[0],     // Source length
                               nullptr,        // Destination buffer (null for size calculation)
                               0,              // Destination buffer size
                               nullptr,        // Default char
                               nullptr         // Used default char flag
);

            if (size == 0) throw std::runtime_error("String conversion failed");

            // Create the output string with the required size
            std::string text(size, '\0');

            // Convert the wide string to UTF-8
            if (WideCharToMultiByte(CP_UTF8, 0, &str[1], str[0], text.data(), size, nullptr, nullptr) == 0) {
                throw std::runtime_error("String conversion failed");
            }

            return text;
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
        }
    };

    inline bool operator==(const String& lhs, const String& rhs) { return static_cast<std::string>(lhs) == static_cast<std::string>(rhs); }

    inline std::strong_ordering operator<=>(const String& lhs, const String& rhs)
    {
        return static_cast<std::string>(lhs) <=> static_cast<std::string>(rhs);
    }

    template<typename TOther>
    String operator+(const String& lhs, TOther&& rhs)
    {
        const std::string result = static_cast<std::string>(lhs) + static_cast<std::string>(rhs);
        return String(result);
    }

    inline void swap(String& lhs, String& rhs) noexcept
    {
        using std::swap;
        swap(lhs.xltype, rhs.xltype);
        swap(lhs.val.str, rhs.val.str);
    }

    namespace literals
    {
        inline String operator""_xs(const char* str, std::size_t len) { return String(std::string_view(str, len)); }
    }    // namespace literals
}    // namespace xll