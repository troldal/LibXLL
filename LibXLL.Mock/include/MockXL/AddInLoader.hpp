#pragma once

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace MockXL::impl {

/**
 * @brief Loads an XLL (DLL) at runtime and resolves exported symbols.
 *
 * Handles platform differences (LoadLibrary vs dlopen), converts relative
 * paths to absolute, and on Windows adds the XLL's directory to the DLL
 * search path so MinGW runtime dependencies are found automatically.
 *
 * Usage:
 * @code
 * XllLoader loader("scalar_validation.xll");
 * auto fn = loader.resolve<AddNumbersFn>("AddNumbers");
 * @endcode
 */
class AddInLoader
{
public:
    /**
     * @brief Loads the XLL at the given path.
     * @throws std::runtime_error if the library cannot be loaded.
     */
    explicit AddInLoader(const std::filesystem::path& path);
    ~AddInLoader();

    AddInLoader(const AddInLoader&)            = delete;
    AddInLoader& operator=(const AddInLoader&) = delete;
    AddInLoader(AddInLoader&&)                 = delete;
    AddInLoader& operator=(AddInLoader&&)      = delete;

    /**
     * @brief Resolves an exported symbol and casts it to FnPtr.
     * @throws std::runtime_error if the symbol is not found.
     */
    template<typename FnPtr>
    FnPtr resolve(const char* name) const
    {
        auto* raw = resolve_raw(name);
        if (!raw)
            throw std::runtime_error(std::string("Symbol not found in ") +
                                     m_path.filename().string() + ": " + name);
        FnPtr fn{};
        static_assert(sizeof(FnPtr) == sizeof(raw), "Function pointer size mismatch");
        std::memcpy(&fn, &raw, sizeof(fn));
        return fn;
    }

    /**
     * @brief Resolves an exported symbol, returning nullptr if not found.
     */
    template<typename FnPtr>
    FnPtr try_resolve(const char* name) const noexcept
    {
        try { return resolve<FnPtr>(name); }
        catch (...) { return nullptr; }
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }

private:
#ifdef _WIN32
    using Handle = HMODULE;
    using RawPtr = FARPROC;
#else
    using Handle = void*;
    using RawPtr = void*;
#endif

    [[nodiscard]] RawPtr resolve_raw(const char* name) const;

    Handle                m_handle{};
    std::filesystem::path m_path;
};

// ============================================================================
// Inline implementations
// ============================================================================

namespace impl {

inline std::string last_load_error()
{
#ifdef _WIN32
    DWORD err = GetLastError();
    char buf[512]{};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buf, sizeof(buf), nullptr);
    std::string s{buf};
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return s;
#else
    const char* msg = dlerror();
    return msg ? msg : "unknown error";
#endif
}

} // namespace impl

inline AddInLoader::AddInLoader(const std::filesystem::path& path)
    : m_path(std::filesystem::absolute(path))
{
#ifdef _WIN32
    auto dir = m_path.parent_path().string();
    SetDllDirectoryA(dir.c_str());
    m_handle = LoadLibraryA(m_path.string().c_str());
#else
    m_handle = dlopen(m_path.string().c_str(), RTLD_NOW);
#endif

    if (!m_handle)
        throw std::runtime_error("Failed to load " + m_path.string() +
                                 ": " + impl::last_load_error());
}

inline AddInLoader::~AddInLoader()
{
    if (m_handle) {
#ifdef _WIN32
        FreeLibrary(m_handle);
#else
        dlclose(m_handle);
#endif
    }
}

inline AddInLoader::RawPtr AddInLoader::resolve_raw(const char* name) const
{
#ifdef _WIN32
    return GetProcAddress(m_handle, name);
#else
    return dlsym(m_handle, name);
#endif
}

} // namespace MockExcel

