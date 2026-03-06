#pragma once

#include <stdexcept>
#include <string>
#include <typeinfo>
#include <xlcall.hpp>

namespace MockXL {

using AutoFreeFn = void (*)(const XLOPER12*);

/**
 * @brief RAII owner of an XLOPER12* returned by an XLL function.
 *
 * Calls xlAutoFree12 on destruction when xlbitDLLFree is set,
 * mirroring what Excel does after each UDF call returns.
 *
 * Usage:
 * @code
 * XloperResult r{ fn(&a, &b), autoFree };
 * if (r.type() == xltypeNum)
 *     double v = r.as_number();
 * @endcode
 */
class XloperResult
{
public:
    XloperResult() = default;

    explicit XloperResult(XLOPER12* ptr, AutoFreeFn autoFree = nullptr) noexcept
        : m_ptr(ptr), m_autoFree(autoFree) {}

    ~XloperResult() { reset(); }

    XloperResult(const XloperResult&)            = delete;
    XloperResult& operator=(const XloperResult&) = delete;

    XloperResult(XloperResult&& o) noexcept
        : m_ptr(o.m_ptr), m_autoFree(o.m_autoFree)
    {
        o.m_ptr = nullptr;
    }

    XloperResult& operator=(XloperResult&& o) noexcept
    {
        if (this != &o) {
            reset();
            m_ptr      = o.m_ptr;
            m_autoFree = o.m_autoFree;
            o.m_ptr    = nullptr;
        }
        return *this;
    }

    /** @brief Releases the owned pointer, calling xlAutoFree12 if needed. */
    void reset() noexcept
    {
        if (m_ptr && m_autoFree && (m_ptr->xltype & xlbitDLLFree))
            m_autoFree(m_ptr);
        m_ptr = nullptr;
    }

    [[nodiscard]] const XLOPER12* get()   const noexcept { return m_ptr; }
    [[nodiscard]] bool            empty() const noexcept { return m_ptr == nullptr; }
    explicit operator bool()              const noexcept { return m_ptr != nullptr; }

    /**
     * @brief Returns the xltype with the DLL/XL-free bits masked off.
     * Returns xltypeNil when the pointer is null.
     */
    [[nodiscard]] decltype(XLOPER12::xltype) type() const noexcept
    {
        if (!m_ptr) return static_cast<decltype(XLOPER12::xltype)>(xltypeNil);
        constexpr auto mask =
            static_cast<decltype(XLOPER12::xltype)>(~(xlbitDLLFree | xlbitXLFree));
        return m_ptr->xltype & mask;
    }

    // ---- typed accessors — throw std::bad_cast on type mismatch ----

    [[nodiscard]] double as_number() const
    {
        if (type() != xltypeNum) throw std::bad_cast{};
        return m_ptr->val.num;
    }

    [[nodiscard]] bool as_bool() const
    {
        if (type() != xltypeBool) throw std::bad_cast{};
        return m_ptr->val.xbool != 0;
    }

    [[nodiscard]] int as_int() const
    {
        if (type() != xltypeInt) throw std::bad_cast{};
        return static_cast<int>(m_ptr->val.w);
    }

    /** @brief Returns the Pascal wide-string payload (excluding the length prefix). */
    [[nodiscard]] std::wstring as_string() const
    {
        if (type() != xltypeStr) throw std::bad_cast{};
        const wchar_t* buf = m_ptr->val.str;
        return std::wstring(buf + 1, static_cast<std::size_t>(buf[0]));
    }

    [[nodiscard]] int as_error() const
    {
        if (type() != xltypeErr) throw std::bad_cast{};
        return m_ptr->val.err;
    }

    /**
     * @brief Implicit conversion to any type derived from XLOPER12.
     *
     * Allows an XloperResult to be passed directly wherever an xll:: type
     * reference is expected, e.g.:
     * @code
     * XloperResult r = session.call<MyFn>("Export", n);
     * xll::Number& num = r;   // implicit — no cast needed
     * @endcode
     *
     * @throws std::bad_cast if the pointer is null.
     */
    template<typename T>
        requires std::is_base_of_v<XLOPER12, T>
    operator T&() const
    {
        if (!m_ptr) throw std::bad_cast{};
        return static_cast<T&>(*m_ptr);
    }

private:
    XLOPER12*  m_ptr{};
    AutoFreeFn m_autoFree{};
};

} // namespace MockExcel


