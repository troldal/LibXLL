#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include <boost/dll/shared_library.hpp>
#include <boost/dll/shared_library_load_mode.hpp>
#include <boost/dll/import.hpp>

#include <fxt/monads/Expected.hpp>
#include <fxt/utils/Failure.hpp>

#ifdef _WIN32
#    include <windows.h>    // SetDllDirectoryA
#endif

namespace MockXL::impl
{

    /**
     * @brief Loads an XLL (DLL/SO) at runtime and resolves exported symbols.
     *
     * Wraps boost::dll::shared_library to handle platform differences
     * transparently.  On Windows, the XLL's directory is added to the DLL search
     * path before loading so MinGW runtime dependencies are found automatically.
     * On Linux, RTLD_GLOBAL is used so the executable's exported symbols
     * (Excel12, Excel12v) are visible to the loaded shared library.
     *
     * Usage:
     * @code
     * AddInLoader loader("scalar_validation.xll");
     * auto fn = loader.resolve<AddNumbersFn>("AddNumbers");
     * @endcode
     */
    class AddInLoader
    {
        boost::dll::shared_library m_lib;
        std::filesystem::path      m_path;

    public:
        /**
         * @brief Loads the XLL at the given path.
         * @throws std::runtime_error if the library cannot be loaded.
         */
        explicit AddInLoader(const std::filesystem::path& path) : m_path(std::filesystem::absolute(path))
        {
#ifdef _WIN32
            // Add the XLL's directory to the DLL search path so MinGW runtime
            // dependencies (libstdc++, libgcc, etc.) are found automatically.
            SetDllDirectoryA(m_path.parent_path().string().c_str());
#endif
            m_lib.load(boost::dll::fs::path { m_path.string() }, boost::dll::load_mode::rtld_now | boost::dll::load_mode::rtld_global);
        }

        ~AddInLoader() = default;

        AddInLoader(const AddInLoader&)            = delete;
        AddInLoader& operator=(const AddInLoader&) = delete;
        AddInLoader(AddInLoader&&)                 = delete;
        AddInLoader& operator=(AddInLoader&&)      = delete;

        /**
         * @brief Resolves an exported symbol.
         *
         * Returns an fxt::expected containing a std::function that calls the
         * symbol and keeps the library loaded via shared ownership, or an
         * fxt::failure describing why the symbol could not be resolved.
         */
        template<typename FnPtr>
        auto resolve(const char* name) const
            -> fxt::expected<std::function<std::remove_pointer_t<FnPtr>>, fxt::failure>
        {
            using Sig = std::remove_pointer_t<FnPtr>;
            if (!m_lib.has(name))
                return fxt::unexpected(fxt::failure{
                    std::string("Symbol not found in ") +
                    m_path.filename().string() + ": " + name });

            auto imported = boost::dll::import_symbol<Sig>(m_lib, name);
            return std::function<Sig>{ [imported]<typename... TArgs>(TArgs&&... args) {
                return imported(std::forward<TArgs>(args)...);
            }};
        }

        [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }
    };

}    // namespace MockXL::impl
