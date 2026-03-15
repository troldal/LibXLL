//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "../Types/Array.hpp"
#include "../Types/StringEnum.hpp"
#include "Types/MatrixBuffer.hpp"
#include "Types/Native.hpp"

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
    struct arg_traits<xll::NativeBool>
    {
        static constexpr std::string_view excel_type = "A";
    };

    template<>
    struct arg_traits<xll::NativeDouble>
    {
        static constexpr std::string_view excel_type = "B";
    };

    template<>
    struct arg_traits<xll::NativeString>
    {
        static constexpr std::string_view excel_type = "C";
    };

    template<>
    struct arg_traits<xll::NativeWString>
    {
        static constexpr std::string_view excel_type = "C%";
    };

    template<>
    struct arg_traits<xll::NativeUInt16>
    {
        static constexpr std::string_view excel_type = "H";
    };

    template<>
    struct arg_traits<xll::NativeInt16>
    {
        static constexpr std::string_view excel_type = "I";
    };

    template<>
    struct arg_traits<xll::NativeInt32>
    {
        static constexpr std::string_view excel_type = "J";
    };

    template<>
    struct arg_traits<xll::MatrixBuffer>
    {
        static constexpr std::string_view excel_type = "K%";
    };

    template<typename T>
        requires is_xll_type<T>
    struct arg_traits<T>
    {
        static constexpr std::string_view excel_type = "Q";
    };

}    // namespace xll::traits