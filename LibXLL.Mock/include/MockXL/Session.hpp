#pragma once

#include "AddInLoader.hpp"
#include "Excel12Server.hpp"
#include "XloperResult.hpp"
#include "fxt/monads/Tap.hpp"
#include "fxt/monads/Tee.hpp"

#include <filesystem>
#include <fxt/monads/Expected.hpp>
#include <fxt/utils/Failure.hpp>
#include <iostream>
#include <xlcall.hpp>

namespace MockXL
{

    namespace fs = std::filesystem;

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
        using xlAutoOpen  = decltype(+[]{ return 0; });
        using xlAutoClose = decltype(+[]{ return 0; });
        using xlAutoFree  = decltype(+[](const XLOPER12*){ });

        impl::AddInLoader m_loader;
        FAutoFree         m_xlAutoFree {};

    public:
        explicit Session(const fs::path& xllPath) : m_loader(xllPath)
        {
            // resolve returns fxt::expected; value_or({}) gives an empty
            // std::function if xlAutoFree12 is not exported.
            m_xlAutoFree = m_loader.resolve<xlAutoFree>("xlAutoFree12").value_or(FAutoFree{});

            // Tell the mock server the XLL's path so xlGetName returns the right value.
            Excel12Server::instance().set_xll_name(xllPath.stem().string() + ".xll");

#ifdef _WIN32
            // On Windows xlcall_cpp.h uses GetProcAddress(GetModuleHandle(NULL),
            // "MdCallBack12") to find the callback.  The add-in also exports
            // SetExcel12EntryPt which lets a non-Excel host inject the pointer
            // directly — use it here as a belt-and-suspenders measure.
            using SetEntryPt = void (*)(EXCEL12PROC);
            if (auto setter = m_loader.resolve<SetEntryPt>("SetExcel12EntryPt"); setter.has_value()) {
                (*setter)([](int xlfn, int coper, LPXLOPER12* rgpxloper12, LPXLOPER12 res) -> int {
                    return MockXL::Excel12Server::instance().dispatch(xlfn, coper, rgpxloper12, res);
                });
            }
#endif

            // Drive xlAutoOpen exactly as Excel does on add-in load.
            m_loader.resolve<xlAutoOpen>("xlAutoOpen")
                | fxt::tee([](const auto& fn) { fn(); })
                | fxt::tap_error([](const fxt::failure& err) { throw std::runtime_error{ err.message() }; });
        }

        /**
         * @brief Calls xlAutoClose and unloads the XLL.
         */
        ~Session()
        {
            // Drive xlAutoClose exactly as Excel does on add-in unload.
            m_loader.resolve<xlAutoClose>("xlAutoClose")
                | fxt::tee([](const auto& fn) { fn(); })
                | fxt::tap_error([](const fxt::failure& err) { std::cerr << err.message() << std::endl; });
        }

        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;
        Session(Session&&)                 = delete;
        Session& operator=(Session&&)      = delete;

        [[nodiscard]] const fs::path& path() const noexcept { return m_loader.path(); }

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
        /**
         * @brief Resolves exportName, calls it with args, and returns the result.
         *
         * Returns an fxt::expected containing the XloperResult on success, or an
         * fxt::failure if the symbol could not be resolved.  The caller decides
         * whether to treat a missing export as fatal (.value()) or optional
         * (.value_or(XloperResult{})).
         *
         * Each argument must be of a type derived from XLOPER12 (e.g. xll::Number,
         * xll::String).  The cast to const XLOPER12* is performed internally.
         */
        template<typename FnPtr, typename... Args>
            requires(std::is_base_of_v<XLOPER12, std::remove_cvref_t<Args>> && ...)
        auto call(const char* exportName, Args&&... args) const -> fxt::expected<XloperResult, fxt::failure>
        {
            auto fn = m_loader.resolve<FnPtr>(exportName);
            if (!fn.has_value())
                return fxt::unexpected(fn.error());
            auto* raw = (*fn)(static_cast<const XLOPER12*>(&args)...);
            return XloperResult { raw, m_xlAutoFree };
        }
    };

}    // namespace MockXL
