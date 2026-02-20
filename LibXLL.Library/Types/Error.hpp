//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"
#include "String.hpp"

#include <format>
#include <string>

namespace xll
{

    class Error : public impl::Base<Error, xltypeErr>
    {
        using BASE = impl::Base<Error, xltypeErr>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Error";

    public:
        using BASE::BASE;

        // Returns a string representation of the error code
        [[nodiscard]]
        constexpr xll::String to_string() const
        {
            switch (val.err) {
                case xlerrNull:
                    return "#NULL!";
                case xlerrDiv0:
                    return "#DIV/0!";
                case xlerrValue:
                    return "#VALUE!";
                case xlerrRef:
                    return "#REF!";
                case xlerrName:
                    return "#NAME?";
                case xlerrNum:
                    return "#NUM!";
                case xlerrNA:
                    return "#N/A";
                default:
                    throw std::runtime_error("Unknown error");
            }
        }

        template <std::integral T = int>
        constexpr explicit operator T() const
        {
            return error_index();
        }

        // constexpr explicit operator int() const
        // {
        //     return error_index();
        // }

        [[nodiscard]]
        constexpr int error_index() const
        {
            ensure(xltype == xltypeErr);
            switch (val.err) {
                case xlerrNull:
                    return 0;
                case xlerrDiv0:
                    return 1;
                case xlerrValue:
                    return 2;
                case xlerrRef:
                    return 3;
                case xlerrName:
                    return 4;
                case xlerrNum:
                    return 5;
                case xlerrNA:
                    return 6;
                default:
                    throw std::runtime_error("Unknown error");
            }
        }

        [[nodiscard]]
        constexpr int error_id() const
        {
            ensure(xltype == xltypeErr);
            return val.err;
        }

        constexpr friend bool operator==(const xll::Error& lhs, const xll::Error& rhs)
        {
            return lhs.value() == rhs.value();
        }

        // Stream output operator
        friend std::ostream& operator<<(std::ostream& os, const Error& error) { return os << error.to_string(); }
    };

    inline static const Error ErrNull  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrNull; return err; }());
    inline static const Error ErrDiv0  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrDiv0; return err; }());
    inline static const Error ErrValue = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrValue; return err; }());
    inline static const Error ErrRef   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrRef; return err; }());
    inline static const Error ErrName  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrName; return err; }());
    inline static const Error ErrNum   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrNum; return err; }());
    inline static const Error ErrNA    = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = xlerrNA; return err; }());

}    // namespace xll

// Standard formatter specialization
// template<>
// struct std::formatter<xll::Error> : std::formatter<std::string>
// {
//     // constexpr auto parse(std::format_parse_context& ctx) {
//     //     return ctx.begin();
//     // }
//
//     auto format(const xll::Error& error, std::format_context& ctx) const
//     {
//         return std::formatter<std::string>::format(error.to_string(), ctx);
//     }
// };

template<>
struct std::formatter<xll::Error> : std::formatter<xll::String> {
    auto format(const xll::Error& str, std::format_context& ctx) const {
        return std::formatter<xll::String>::format(str.to_string(), ctx);
    }
};
