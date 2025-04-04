//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"

#include <format>
#include <string>

namespace xll
{

    class Error : public impl::Base<Error, xltypeErr>
    {
        using BASE = impl::Base<Error, xltypeErr>;
        friend BASE;
        static inline std::string_view type_name = "xll::Error";

    public:
        using BASE::BASE;

        // Returns a string representation of the error code
        std::string to_string() const
        {
            switch (val.err) {
                case 0:
                    return "#NULL!";
                case 7:
                    return "#DIV/0!";
                case 15:
                    return "#VALUE!";
                case 23:
                    return "#REF!";
                case 29:
                    return "#NAME?";
                case 36:
                    return "#NUM!";
                case 42:
                    return "#N/A";
                default:
                    return "UNKNOWN_ERROR#" + std::to_string(val.err);
            }
        }

        // Stream output operator
        friend std::ostream& operator<<(std::ostream& os, const Error& error) { return os << error.to_string(); }
    };





    static const Error ErrNull  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 0; return err; }());
    static const Error ErrDiv0  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 7; return err; }());
    static const Error ErrValue = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 15; return err; }());
    static const Error ErrRef   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 23; return err; }());
    static const Error ErrName  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 29; return err; }());
    static const Error ErrNum   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 36; return err; }());
    static const Error ErrNA    = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 42; return err; }());

}    // namespace xll

// Standard formatter specialization
template<>
struct std::formatter<xll::Error> : std::formatter<std::string>
{
    // constexpr auto parse(std::format_parse_context& ctx) {
    //     return ctx.begin();
    // }

    auto format(const xll::Error& error, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(error.to_string(), ctx);
    }
};
