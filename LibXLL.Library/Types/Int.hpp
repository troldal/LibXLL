//
// Created by kenne on 21/03/2025.
//

#pragma once
#include "Base.hpp"
#include "Bool.hpp"
#include "Number.hpp"

#include <iostream>
#include <optional>

namespace xll
{

    class Int : public impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>
    {
        using BASE = impl::Base<Int, xltypeInt, xltypeNum, xltypeBool>;
        friend BASE;
        static constexpr std::string_view type_name = "xll::Int";

    public:
        using BASE::BASE;
        using BASE::operator=;

        constexpr Int() : Int(0) {}

        constexpr Int& operator++()
        {
            ++value();
            return *this;
        }

        constexpr Int operator++(int)
        {
            Int old = *this;
            ++value();
            return old;
        }

        constexpr Int& operator--()
        {
            --value();
            return *this;
        }

        constexpr Int operator--(int)
        {
            Int old = *this;
            --value();
            return old;
        }

        // Int& operator%=(const Int& rhs)
        // {
        //     // ensure(is_valid());
        //     // ensure(rhs.is_valid());
        //     // ensure(rhs.value() != 0 && "Modulo by zero");
        //     value() %= rhs.value();
        //     return *this;
        // }

        // template<typename TOther>
        //     requires (std::same_as<TOther, Int> || std::same_as<TOther, Bool>)
        // Int& operator%=(const TOther& rhs)
        // {
        //     // ensure(is_valid());
        //     // ensure(rhs.is_valid());
        //     auto rhsValue = static_cast<int>(rhs);
        //     // ensure(rhsValue != 0 && "Modulo by zero");
        //     value() %= rhsValue;
        //     return *this;
        // }

        template<typename TValue>
        constexpr Int& operator%=(TValue rhs)
            requires std::convertible_to<TValue, int>
        {
            // ensure(is_valid());
            auto rhsValue = static_cast<int>(rhs);
            // ensure(rhsValue != 0 && "Modulo by zero");
            value() %= rhsValue;
            return *this;
        }

        template<typename TOther>
            requires (std::same_as<TOther, Int> || std::same_as<TOther, xll::Bool>)
        constexpr friend Int operator%(const Int& lhs, TOther&& rhs)
        {
            Int result = lhs;
            result %= std::forward<TOther>(rhs);
            return result;
        }

        template<typename TValue>
            requires std::integral<TValue>
        constexpr friend Int operator%(const Int& lhs,TValue rhs)
        {
            Int result = lhs;
            result %= rhs;
            return result;
        }


    };

    // namespace literals
    // {
    //     inline String operator""_xs(const char* str, std::size_t len) { return String(std::string_view(str, len)); }
    // }    // namespace literals

}    // namespace xll