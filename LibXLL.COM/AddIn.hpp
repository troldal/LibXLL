// ---------------------------------------------------------------------------
// AddIn.hpp — declares the COM add-in identity for this DLL.
//
// This file is part of the LibXLL.COM library.
//
// Each DLL that uses LibXLL.COM must construct exactly one static
// com::AddIn instance (typically in Handlers.cpp) to declare its
// GUID, ProgID, friendly name, and description.  The constructor
// self-registers into a per-DLL global pointer so that the COM
// server exports (DllGetClassObject, DllRegisterServer, etc.) can
// retrieve the identity at runtime.
//
// Because the global pointer is an `inline` variable in a header-only
// library, each DLL image gets its own copy — there is no cross-DLL
// conflict when multiple add-in DLLs are loaded into the same process.
//
// Usage:
//   static const com::AddIn s_addin(
//       "1E0739E0-A1B5-4AB4-953C-2D8959196A13",
//       L"xlCOM.ImGui.Connect",
//       L"xlCOM ImGui",
//       L"xlCOM Excel COM Add-in (Dear ImGui)"
//   );
// ---------------------------------------------------------------------------

#pragma once

#include "Utils/ParseGUID.hpp"

namespace com {

class AddIn;

namespace detail {
    // Per-DLL global pointer — each DLL image gets its own copy because
    // this is an `inline` variable in a header-only library.
    // Set by AddIn's constructor during static initialisation.
    inline const AddIn* g_addIn = nullptr;
}

// ---------------------------------------------------------------------------
// AddIn — declares the COM add-in identity for this DLL.
//
// Construct exactly one static instance per DLL, in the same TU that
// includes COMServer.hpp (typically Handlers.cpp).  The constructor
// self-registers into the per-DLL global so DllGetClassObject,
// DllRegisterServer, etc. can find the identity at runtime.
// ---------------------------------------------------------------------------

class AddIn
{
public:
    AddIn(const char* guid, const wchar_t* progId,
          const wchar_t* friendlyName, const wchar_t* description)
        : m_clsid(::detail::parseGUID(guid))
        , m_progId(progId)
        , m_friendlyName(friendlyName)
        , m_description(description)
    {
        detail::g_addIn = this;
    }

    [[nodiscard]] const CLSID&   clsid()         const { return m_clsid; }
    [[nodiscard]] const wchar_t* progId()        const { return m_progId; }
    [[nodiscard]] const wchar_t* friendlyName()  const { return m_friendlyName; }
    [[nodiscard]] const wchar_t* description()   const { return m_description; }

private:
    CLSID          m_clsid;
    const wchar_t* m_progId;
    const wchar_t* m_friendlyName;
    const wchar_t* m_description;
};

// Convenience accessor.
inline const AddIn& addIn()
{
    return *detail::g_addIn;
}

} // namespace com

