#pragma once

#include "AddInLoader.hpp"
#include "XloperResult.hpp"
#include <filesystem>
#include <xlcall.hpp>

namespace MockXL {

/**
 * @brief Façade that owns an XllLoader and drives the xlAutoOpen/xlAutoClose
 * lifecycle, exactly as Excel does when loading and unloading an add-in.
 *
 * call() resolves an export by name, invokes it with the supplied xll:: type
 * arguments (which must be derived from XLOPER12), and returns an XloperResult
 * that owns the returned pointer.  The cast to const XLOPER12* is performed
 * internally — no explicit cast is needed at the call site.
 *
 * Usage:
 * @code
 * MockXL::Session session("scalar_validation.xll");
 *
 * xll::Number a{3.0}, b{4.0};
 * auto result = session.call<AddNumbersFn>("AddNumbers", a, b);
 *
 * std::cout << result.as_number() << "\n"; // 7.0
 * @endcode
 */
class Session
{
public:
    /**
     * @brief Loads the XLL and calls xlAutoOpen.
     * @throws std::runtime_error if the library cannot be loaded.
     */
    explicit Session(const std::filesystem::path& xllPath);

    /**
     * @brief Calls xlAutoClose and unloads the XLL.
     */
    ~Session();

    Session(const Session&)            = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&)                 = delete;
    Session& operator=(Session&&)      = delete;

    [[nodiscard]] const impl::AddInLoader& loader() const noexcept { return m_loader; }

    /**
     * @brief Resolves exportName, calls it with args, and returns the result.
     *
     * Each argument must be of a type derived from XLOPER12 (e.g. xll::Number,
     * xll::String).  The cast to const XLOPER12* is performed internally, so
     * no explicit cast is required at the call site:
     * @code
     * xll::Number n{42.0};
     * session.call<MyFn>("Export", n);
     * @endcode
     *
     * @throws std::runtime_error if the export is not found.
     */
    template<typename FnPtr, typename... Args>
        requires (std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
    XloperResult call(const char* exportName, Args&&... args) const
    {
        auto fn   = m_loader.resolve<FnPtr>(exportName);
        auto* raw = fn(static_cast<const XLOPER12*>(&args)...);
        return XloperResult{ raw, m_autoFree };
    }

    /**
     * @brief Like call(), but returns an empty XloperResult if the export
     * is not found rather than throwing.
     */
    template<typename FnPtr, typename... Args>
        requires (std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
    XloperResult try_call(const char* exportName, Args&&... args) const
    {
        auto fn = m_loader.try_resolve<FnPtr>(exportName);
        if (!fn) return XloperResult{};
        auto* raw = fn(static_cast<const XLOPER12*>(&args)...);
        return XloperResult{ raw, m_autoFree };
    }

private:
    using AutoOpenFn  = int (*)();
    using AutoCloseFn = int (*)();

    impl::AddInLoader  m_loader;
    AutoFreeFn m_autoFree{};
};

// ============================================================================
// Inline implementations
// ============================================================================

inline Session::Session(const std::filesystem::path& xllPath)
    : m_loader(xllPath)
{
    m_autoFree = m_loader.try_resolve<AutoFreeFn>("xlAutoFree12");

    // Drive xlAutoOpen exactly as Excel does on add-in load.
    if (auto open = m_loader.try_resolve<AutoOpenFn>("xlAutoOpen"))
        open();
}

inline Session::~Session()
{
    // Drive xlAutoClose exactly as Excel does on add-in unload.
    if (auto close = m_loader.try_resolve<AutoCloseFn>("xlAutoClose"))
        close();
}

} // namespace MockExcel

