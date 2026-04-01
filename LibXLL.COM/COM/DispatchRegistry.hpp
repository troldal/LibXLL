#pragma once

#include "Interfaces.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace com {

// ---------------------------------------------------------------------------
// DispatchRegistry — central runtime registry for IDispatch callbacks.
//
// Each DispatchCallback<"Name"> self-registers here at static-init time.
// Connect::GetIDsOfNames and Connect::Invoke delegate to this registry,
// so no hand-maintained if-chains or hard-coded maps are needed.
// ---------------------------------------------------------------------------

class DispatchRegistry
{
public:
    using ExecuteFn = std::function<HRESULT(DISPPARAMS*, VARIANT*)>;

    static DispatchRegistry& instance()
    {
        static DispatchRegistry s_instance;
        return s_instance;
    }

    // Register a callback under the given (case-insensitive) name.
    // Assigns a DISPID automatically.  Re-registering the same name
    // replaces the callback but keeps the existing DISPID.
    DISPID add(const std::wstring& name, ExecuteFn fn)
    {
        auto it = m_nameToId.find(name);
        if (it != m_nameToId.end())
        {
            m_idToCallback[it->second] = std::move(fn);
            return it->second;
        }
        const DISPID id = m_nextId++;
        m_nameToId[name] = id;
        m_idToCallback[id] = std::move(fn);
        return id;
    }

    // Look up a DISPID by name (case-insensitive).
    // Returns DISPID_UNKNOWN if not found.
    [[nodiscard]] DISPID getDispId(const std::wstring& name) const
    {
        auto it = m_nameToId.find(name);
        return it != m_nameToId.end() ? it->second : DISPID_UNKNOWN;
    }

    // Invoke the callback for the given DISPID.
    HRESULT invoke(DISPID id, DISPPARAMS* params, VARIANT* result) const
    {
        auto it = m_idToCallback.find(id);
        if (it == m_idToCallback.end() || !it->second)
            return DISP_E_MEMBERNOTFOUND;
        return it->second(params, result);
    }

private:
    DispatchRegistry() = default;

    // Case-insensitive wide-string hash / equality for the name map.
    struct WStrIHash
    {
        size_t operator()(const std::wstring& s) const
        {
            std::wstring lower(s.size(), L'\0');
            std::transform(s.begin(), s.end(), lower.begin(), ::towlower);
            return std::hash<std::wstring>{}(lower);
        }
    };

    struct WStrIEqual
    {
        bool operator()(const std::wstring& a, const std::wstring& b) const
        {
            return _wcsicmp(a.c_str(), b.c_str()) == 0;
        }
    };

    std::unordered_map<std::wstring, DISPID, WStrIHash, WStrIEqual> m_nameToId;
    std::unordered_map<DISPID, ExecuteFn>                            m_idToCallback;
    DISPID m_nextId = 1;
};

} // namespace com

