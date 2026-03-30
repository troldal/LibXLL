#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <ole2.h>

#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace xll::excel
{
    class ComError : public std::runtime_error
    {
    public:
        ComError(HRESULT result, const char* message)
            : std::runtime_error(message), m_result(result)
        {
        }

        [[nodiscard]] HRESULT result() const noexcept
        {
            return m_result;
        }

    private:
        HRESULT m_result;
    };

    inline void throw_if_failed(const HRESULT result, const char* message)
    {
        if (FAILED(result))
            throw ComError(result, message);
    }

    template <typename T>
    class ComPtr
    {
    public:
        ComPtr() = default;

        ComPtr(std::nullptr_t)
        {
        }

        explicit ComPtr(T* ptr)
            : m_ptr(ptr)
        {
            if (m_ptr)
                m_ptr->AddRef();
        }

        ComPtr(const ComPtr& other)
            : ComPtr(other.m_ptr)
        {
        }

        ComPtr(ComPtr&& other) noexcept
            : m_ptr(other.detach())
        {
        }

        ~ComPtr()
        {
            reset();
        }

        ComPtr& operator=(const ComPtr& other)
        {
            if (this != &other)
                ComPtr(other).swap(*this);
            return *this;
        }

        ComPtr& operator=(ComPtr&& other) noexcept
        {
            if (this != &other)
                attach(other.detach());
            return *this;
        }

        void swap(ComPtr& other) noexcept
        {
            std::swap(m_ptr, other.m_ptr);
        }

        [[nodiscard]] T* get() const noexcept
        {
            return m_ptr;
        }

        [[nodiscard]] T* operator->() const noexcept
        {
            return m_ptr;
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }

        [[nodiscard]] T** put() noexcept
        {
            reset();
            return &m_ptr;
        }

        void attach(T* ptr) noexcept
        {
            if (m_ptr != ptr) {
                reset();
                m_ptr = ptr;
            }
        }

        [[nodiscard]] T* detach() noexcept
        {
            T* ptr = m_ptr;
            m_ptr = nullptr;
            return ptr;
        }

        void reset(T* ptr = nullptr) noexcept
        {
            if (m_ptr)
                m_ptr->Release();
            m_ptr = ptr;
        }

    private:
        T* m_ptr = nullptr;
    };

    class BStr
    {
    public:
        BStr() = default;

        explicit BStr(const wchar_t* value)
            : BStr(value ? std::wstring_view(value) : std::wstring_view())
        {
        }

        explicit BStr(std::wstring_view value)
            : m_value(::SysAllocStringLen(value.empty() ? nullptr : value.data(),
                                          static_cast<UINT>(value.size())))
        {
            if (!m_value && !value.empty())
                throw std::bad_alloc();
        }

        BStr(const BStr& other)
            : BStr(other.view())
        {
        }

        BStr(BStr&& other) noexcept
            : m_value(other.release())
        {
        }

        ~BStr()
        {
            ::SysFreeString(m_value);
        }

        BStr& operator=(const BStr& other)
        {
            if (this != &other)
                BStr(other).swap(*this);
            return *this;
        }

        BStr& operator=(BStr&& other) noexcept
        {
            if (this != &other) {
                ::SysFreeString(m_value);
                m_value = other.release();
            }
            return *this;
        }

        void swap(BStr& other) noexcept
        {
            std::swap(m_value, other.m_value);
        }

        [[nodiscard]] BSTR get() const noexcept
        {
            return m_value;
        }

        [[nodiscard]] BSTR release() noexcept
        {
            BSTR value = m_value;
            m_value = nullptr;
            return value;
        }

        [[nodiscard]] UINT length() const noexcept
        {
            return ::SysStringLen(m_value);
        }

        [[nodiscard]] std::wstring_view view() const noexcept
        {
            return {m_value ? m_value : L"", length()};
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return m_value != nullptr;
        }

        [[nodiscard]] static BStr copy(BSTR value)
        {
            return BStr(value ? std::wstring_view(value, ::SysStringLen(value)) : std::wstring_view());
        }

    private:
        BSTR m_value = nullptr;
    };

    class Variant
    {
    public:
        Variant()
        {
            ::VariantInit(&m_value);
        }

        explicit Variant(const VARIANT& value)
            : Variant()
        {
            throw_if_failed(::VariantCopy(&m_value, const_cast<VARIANT*>(&value)),
                            "VariantCopy failed.");
        }

        explicit Variant(const BStr& value)
            : Variant(make_bstr(value.view()))
        {
        }

        Variant(const Variant& other)
            : Variant(other.m_value)
        {
        }

        Variant(Variant&& other) noexcept
            : m_value(other.detach())
        {
        }

        ~Variant()
        {
            clear();
        }

        Variant& operator=(const Variant& other)
        {
            if (this != &other)
                Variant(other).swap(*this);
            return *this;
        }

        Variant& operator=(Variant&& other) noexcept
        {
            if (this != &other) {
                clear();
                m_value = other.detach();
            }
            return *this;
        }

        [[nodiscard]] static Variant make_bstr(std::wstring_view value)
        {
            Variant result;
            result.m_value.vt = VT_BSTR;
            result.m_value.bstrVal = ::SysAllocStringLen(value.empty() ? nullptr : value.data(),
                                                         static_cast<UINT>(value.size()));
            if (!result.m_value.bstrVal && !value.empty())
                throw std::bad_alloc();
            return result;
        }

        void swap(Variant& other) noexcept
        {
            std::swap(m_value, other.m_value);
        }

        void clear() noexcept
        {
            ::VariantClear(&m_value);
            ::VariantInit(&m_value);
        }

        [[nodiscard]] VARIANT* put() noexcept
        {
            clear();
            return &m_value;
        }

        [[nodiscard]] VARIANT& get() noexcept
        {
            return m_value;
        }

        [[nodiscard]] const VARIANT& get() const noexcept
        {
            return m_value;
        }

        [[nodiscard]] VARTYPE type() const noexcept
        {
            return m_value.vt;
        }

        [[nodiscard]] bool is_dispatch() const noexcept
        {
            return m_value.vt == VT_DISPATCH && m_value.pdispVal != nullptr;
        }

        [[nodiscard]] IDispatch* dispatch() const noexcept
        {
            return is_dispatch() ? m_value.pdispVal : nullptr;
        }

    private:
        [[nodiscard]] VARIANT detach() noexcept
        {
            VARIANT value = m_value;
            ::VariantInit(&m_value);
            return value;
        }

        VARIANT m_value{};
    };

    class Dispatch
    {
    public:
        Dispatch() = default;

        explicit Dispatch(IDispatch* dispatch)
            : m_dispatch(dispatch)
        {
        }

        explicit Dispatch(ComPtr<IDispatch> dispatch) noexcept
            : m_dispatch(std::move(dispatch))
        {
        }

        [[nodiscard]] IDispatch* get() const noexcept
        {
            return m_dispatch.get();
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return static_cast<bool>(m_dispatch);
        }

        [[nodiscard]] HRESULT property(LPCOLESTR name, Variant& result) const noexcept
        {
            if (!m_dispatch)
                return E_POINTER;

            DISPID id{};
            if (const HRESULT hr = get_dispid(name, id); FAILED(hr))
                return hr;

            DISPPARAMS params{ nullptr, nullptr, 0, 0 };
            return m_dispatch->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                      DISPATCH_PROPERTYGET, &params, result.put(),
                                      nullptr, nullptr);
        }

        [[nodiscard]] HRESULT property_dispatch(LPCOLESTR name, Dispatch& result) const noexcept
        {
            Variant value;
            if (const HRESULT hr = property(name, value); FAILED(hr)) {
                result = Dispatch();
                return hr;
            }

            if (!value.is_dispatch()) {
                result = Dispatch();
                return DISP_E_TYPEMISMATCH;
            }

            result = Dispatch(value.dispatch());
            return S_OK;
        }

        [[nodiscard]] HRESULT put(LPCOLESTR name, const Variant& value) const noexcept
        {
            if (!m_dispatch)
                return E_POINTER;

            DISPID id{};
            if (const HRESULT hr = get_dispid(name, id); FAILED(hr))
                return hr;

            DISPID namedArg = DISPID_PROPERTYPUT;
            VARIANT valueCopy{};
            ::VariantInit(&valueCopy);
            const HRESULT copyResult = ::VariantCopy(&valueCopy, const_cast<VARIANT*>(&value.get()));
            if (FAILED(copyResult))
                return copyResult;

            DISPPARAMS params{ &valueCopy, &namedArg, 1, 1 };
            const HRESULT invokeResult = m_dispatch->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                                            DISPATCH_PROPERTYPUT, &params,
                                                            nullptr, nullptr, nullptr);
            ::VariantClear(&valueCopy);
            return invokeResult;
        }

        [[nodiscard]] HRESULT put(LPCOLESTR name, std::wstring_view value) const
        {
            return put(name, Variant::make_bstr(value));
        }

    private:
        [[nodiscard]] HRESULT get_dispid(LPCOLESTR name, DISPID& id) const noexcept
        {
            LPOLESTR mutableName = const_cast<LPOLESTR>(name);
            return m_dispatch->GetIDsOfNames(IID_NULL, &mutableName, 1,
                                             LOCALE_USER_DEFAULT, &id);
        }

        ComPtr<IDispatch> m_dispatch;
    };

    template <typename TTo, typename TFrom>
    [[nodiscard]] HRESULT query_interface(TFrom* source, ComPtr<TTo>& result) noexcept
    {
        if (!source) {
            result.reset();
            return E_POINTER;
        }

        TTo* target = nullptr;
        const HRESULT hr = source->QueryInterface(__uuidof(TTo),
                                                  reinterpret_cast<void**>(&target));
        if (SUCCEEDED(hr))
            result.attach(target);
        else
            result.reset();
        return hr;
    }

    inline HRESULT get_active_object(REFCLSID classId, Dispatch& result) noexcept
    {
        ComPtr<IUnknown> unknown;
        if (const HRESULT hr = ::GetActiveObject(classId, nullptr, reinterpret_cast<IUnknown**>(unknown.put()));
            FAILED(hr)) {
            result = Dispatch();
            return hr;
        }

        ComPtr<IDispatch> dispatch;
        if (const HRESULT hr = query_interface(unknown.get(), dispatch); FAILED(hr)) {
            result = Dispatch();
            return hr;
        }

        result = Dispatch(std::move(dispatch));
        return S_OK;
    }

    inline HRESULT get_active_object(LPCOLESTR progId, Dispatch& result) noexcept
    {
        CLSID classId{};
        if (const HRESULT hr = ::CLSIDFromProgID(progId, &classId); FAILED(hr)) {
            result = Dispatch();
            return hr;
        }

        return get_active_object(classId, result);
    }

} // namespace xll::excel

#endif // _WIN32


