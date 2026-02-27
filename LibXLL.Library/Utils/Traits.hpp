//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "../Types/Array.hpp"
#include "../Types/StringEnum.hpp"

namespace xll
{
    class Number;
    class Any;

    template<typename TValue>
    requires is_xll_type<TValue>
    class Optional;
}
namespace xll::traits
{

    template<typename T>
    struct arg_traits;

    template<>
    struct arg_traits<bool>
    {
        static constexpr std::string_view excel_type = "A";
    };

    template<>
    struct arg_traits<double>
    {
        static constexpr std::string_view excel_type = "B";
    };

    template<>
    struct arg_traits<std::string>
    {
        static constexpr std::string_view excel_type = "C";
    };

    template<>
    struct arg_traits<std::string_view>
    {
        static constexpr std::string_view excel_type = "C";
    };

    template<>
    struct arg_traits<String>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<>
    struct arg_traits<Number>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<typename T>
    struct arg_traits<Array<T>>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<typename T>
    struct arg_traits<Variant<T>>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<typename T>
    struct arg_traits<Expected<T, xll::Error>>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<typename T>
    struct arg_traits<Optional<T>>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<>
    struct arg_traits<Any>
    {
        static constexpr std::string_view excel_type = "Q";
    };

    template<fixstr::basic_fixed_string... Strings>
    struct arg_traits<StringEnum<Strings...>>
    {
        // StringEnum inherits from xll::String — same XLOPER12 layout, same type code.
        static constexpr std::string_view excel_type = "Q";
    };

}    // namespace xll::traits