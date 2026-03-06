#pragma once

#include "XloperResult.hpp"
#include <format>
#include <iosfwd>
#include <ostream>
#include <string>
#include <xlcall.hpp>

namespace MockXL {

/**
 * @brief Returns the name of an xltype constant (DLL/XL-free bits stripped).
 */
[[nodiscard]] inline std::string xltype_name(decltype(XLOPER12::xltype) t)
{
    constexpr auto mask =
        static_cast<decltype(XLOPER12::xltype)>(~(xlbitDLLFree | xlbitXLFree));
    switch (t & mask) {
        case xltypeNum:     return "xltypeNum";
        case xltypeStr:     return "xltypeStr";
        case xltypeBool:    return "xltypeBool";
        case xltypeErr:     return "xltypeErr";
        case xltypeMulti:   return "xltypeMulti";
        case xltypeMissing: return "xltypeMissing";
        case xltypeNil:     return "xltypeNil";
        case xltypeInt:     return "xltypeInt";
        default:            return std::format("unknown({})", t & mask);
    }
}

/**
 * @brief Formats a raw XLOPER12 pointer to a human-readable string.
 */
[[nodiscard]] inline std::string format_result(const XLOPER12* result)
{
    if (!result)
        return "[null pointer]";

    constexpr auto mask =
        static_cast<decltype(XLOPER12::xltype)>(~(xlbitDLLFree | xlbitXLFree));
    const auto t = result->xltype & mask;

    switch (t) {
        case xltypeNum:
            return std::format("{} (xltypeNum)", result->val.num);
        case xltypeBool:
            return std::format("{} (xltypeBool)", result->val.xbool ? "TRUE" : "FALSE");
        case xltypeStr:
            return std::format("[string of {} chars] (xltypeStr)",
                               static_cast<int>(result->val.str[0]));
        case xltypeErr:
            return std::format("#ERROR({}) (xltypeErr)", result->val.err);
        default:
            return std::format("[{}]", xltype_name(t));
    }
}

/** @brief Overload for XloperResult. */
[[nodiscard]] inline std::string format_result(const XloperResult& r)
{
    return format_result(r.get());
}

/**
 * @brief Writes "  <label>: <formatted result>\n" to the given stream.
 */
inline void print_result(std::ostream& out, std::string_view label, const XLOPER12* result)
{
    out << "  " << label << ": " << format_result(result) << "\n";
}

/** @brief Overload for XloperResult. */
inline void print_result(std::ostream& out, std::string_view label, const XloperResult& r)
{
    print_result(out, label, r.get());
}

} // namespace MockExcel

