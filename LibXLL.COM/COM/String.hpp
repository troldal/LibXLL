#pragma once

#include <oleauto.h>   // BSTR, SysAllocString, SysAllocStringLen, SysFreeString, SysStringLen
#include <string>
#include <string_view>

namespace com {

// ---------------------------------------------------------------------------
// com::String — RAII wrapper for BSTR.
//
// Owns the BSTR allocation.  Constructible from narrow (UTF-8) and wide
// sources; implicitly converts back to std::string (UTF-8) and std::wstring.
//
// The raw BSTR can be obtained via get() for COM calls that read the value,
// or via release() when passing ownership to a COM output parameter.
// ---------------------------------------------------------------------------

class String
{
public:
    // -------------------------------------------------------------------------
    // Constructors
    // -------------------------------------------------------------------------

    String() noexcept : m_bstr(nullptr) {}

    String(const wchar_t* str) : m_bstr(str ? SysAllocString(str) : nullptr) {} // NOLINT

    String(std::wstring_view str)                                                // NOLINT
        : m_bstr(SysAllocStringLen(str.data(), static_cast<UINT>(str.size()))) {}

    String(const std::wstring& str) : String(std::wstring_view(str)) {}         // NOLINT

    String(const char* str) : String(str ? std::string_view(str) : std::string_view{}) {} // NOLINT

    String(std::string_view str) : m_bstr(nullptr)                              // NOLINT
    {
        if (str.empty()) return;
        const int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                             str.data(), static_cast<int>(str.size()),
                                             nullptr, 0);
        if (wlen <= 0) return;
        std::wstring wide(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0,
                            str.data(), static_cast<int>(str.size()),
                            wide.data(), wlen);
        m_bstr = SysAllocStringLen(wide.data(), static_cast<UINT>(wide.size()));
    }

    String(const std::string& str) : String(std::string_view(str)) {}          // NOLINT

    // -------------------------------------------------------------------------
    // Copy / move
    // -------------------------------------------------------------------------

    String(const String& other)
        : m_bstr(other.m_bstr
                     ? SysAllocStringLen(other.m_bstr, SysStringLen(other.m_bstr))
                     : nullptr)
    {}

    String(String&& other) noexcept : m_bstr(other.m_bstr) { other.m_bstr = nullptr; }

    ~String() { SysFreeString(m_bstr); }

    String& operator=(const String& other)
    {
        if (this != &other)
        {
            SysFreeString(m_bstr);
            m_bstr = other.m_bstr
                         ? SysAllocStringLen(other.m_bstr, SysStringLen(other.m_bstr))
                         : nullptr;
        }
        return *this;
    }

    String& operator=(String&& other) noexcept
    {
        if (this != &other)
        {
            SysFreeString(m_bstr);
            m_bstr       = other.m_bstr;
            other.m_bstr = nullptr;
        }
        return *this;
    }

    // -------------------------------------------------------------------------
    // Conversion
    // -------------------------------------------------------------------------

    [[nodiscard]] std::wstring to_wstring() const
    {
        if (!m_bstr) return {};
        return std::wstring(m_bstr, SysStringLen(m_bstr));
    }

    [[nodiscard]] std::string to_string() const
    {
        if (!m_bstr) return {};
        const UINT wlen = SysStringLen(m_bstr);
        if (wlen == 0) return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0,
                                             m_bstr, static_cast<int>(wlen),
                                             nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string result(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0,
                            m_bstr, static_cast<int>(wlen),
                            result.data(), size, nullptr, nullptr);
        return result;
    }

    operator std::wstring() const { return to_wstring(); }  // NOLINT
    operator std::string()  const { return to_string();  }  // NOLINT

    // -------------------------------------------------------------------------
    // Raw BSTR access
    // -------------------------------------------------------------------------

    // Read-only access — for COM calls that accept a BSTR (in-parameter).
    [[nodiscard]] BSTR get() const noexcept { return m_bstr; }

    // Address-of — for COM output parameters: pass as `str.put()`.
    // Frees any previously held value before returning the pointer.
    [[nodiscard]] BSTR* put()
    {
        SysFreeString(m_bstr);
        m_bstr = nullptr;
        return &m_bstr;
    }

    // Transfers ownership to the caller; the wrapper becomes empty.
    [[nodiscard]] BSTR release() noexcept
    {
        BSTR tmp = m_bstr;
        m_bstr   = nullptr;
        return tmp;
    }

    // -------------------------------------------------------------------------
    // Capacity
    // -------------------------------------------------------------------------

    [[nodiscard]] bool empty()  const noexcept { return !m_bstr || SysStringLen(m_bstr) == 0; }
    [[nodiscard]] UINT size()   const noexcept { return m_bstr ? SysStringLen(m_bstr) : 0u; }
    [[nodiscard]] UINT length() const noexcept { return size(); }

    // -------------------------------------------------------------------------
    // Comparison
    // -------------------------------------------------------------------------

    bool operator==(const String& rhs) const { return to_wstring() == rhs.to_wstring(); }
    auto operator<=>(const String& rhs) const { return to_wstring() <=> rhs.to_wstring(); }

private:
    BSTR m_bstr;
};

} // namespace com

