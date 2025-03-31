//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Bool.hpp"
#include "Error.hpp"
#include "Int.hpp"
#include "Missing.hpp"
#include "Nil.hpp"
#include "Number.hpp"
#include "String.hpp"

#include <functional>
#include <variant>

namespace xll
{
    namespace impl
    {
        template<typename T>
        concept is_valid_type = std::same_as<T, xll::Bool> || std::same_as<T, Error> || std::same_as<T, Int> || std::same_as<T, Number> ||
                                std::same_as<T, String> || std::same_as<T, Nil> || std::same_as<T, Missing>;
    }    // namespace impl

    class Variant;

    template<typename T>
        requires impl::is_valid_type<T>
    T& get(Variant& v);

    template<typename T>
        requires impl::is_valid_type<T>
    const T& get(const Variant& v);

    template<typename T>
        requires impl::is_valid_type<T>
    bool holds_alternative(const Variant& v);

    template<class... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
    };

        class Variant : public XLOPER12
    {
    public:
        Variant() : XLOPER12() { xltype = xltypeNum; }

        template<typename T>
            requires impl::is_valid_type<T>
        Variant(const T& t) : Variant() // NOLINT
        {
            T::lift(*static_cast<XLOPER12*>(this)) = T();
            xll::get<T>(*this) = t;
        }

        Variant(const Variant& v) : Variant()
        {
            xltype = v.xltype;

            if (xll::holds_alternative<xll::Bool>(v))
                xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);
            else if (xll::holds_alternative<xll::Error>(v))
                xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);
            else if (xll::holds_alternative<xll::Int>(v))
                xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);
            else if (xll::holds_alternative<xll::Number>(v))
                xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);
            else if (xll::holds_alternative<xll::String>(v))
                xll::get<xll::String>(*this) = xll::get<xll::String>(v);
            else if (xll::holds_alternative<xll::Nil>(v))
                xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);
            else if (xll::holds_alternative<xll::Missing>(v))
                xll::get<xll::Missing>(*this) = xll::get<xll::Missing>(v);
        }

        Variant(Variant&& v) noexcept : Variant()
        {
            xltype = v.xltype;

            if (xll::holds_alternative<xll::Bool>(v))
                xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);
            else if (xll::holds_alternative<xll::Error>(v))
                xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);
            else if (xll::holds_alternative<xll::Int>(v))
                xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);
            else if (xll::holds_alternative<xll::Number>(v))
                xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);
            else if (xll::holds_alternative<xll::String>(v))
                xll::get<xll::String>(*this) = xll::get<xll::String>(v);
            else if (xll::holds_alternative<xll::Nil>(v))
                xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);
            else if (xll::holds_alternative<xll::Missing>(v))
                xll::get<xll::Missing>(*this) = xll::get<xll::Missing>(v);
        }

        ~Variant()
        {
            if (xll::holds_alternative<xll::Bool>(*this))
                xll::get<xll::Bool>(*this).~Bool();
            else if (xll::holds_alternative<xll::Error>(*this))
                xll::get<xll::Error>(*this).~Error();
            else if (xll::holds_alternative<xll::Int>(*this))
                xll::get<xll::Int>(*this).~Int();
            else if (xll::holds_alternative<xll::Number>(*this))
                xll::get<xll::Number>(*this).~Number();
            else if (xll::holds_alternative<xll::String>(*this))
                xll::get<xll::String>(*this).~String();
            else if (xll::holds_alternative<xll::Nil>(*this))
                xll::get<xll::Nil>(*this).~Nil();
            else if (xll::holds_alternative<xll::Missing>(*this))
                xll::get<xll::Missing>(*this).~Missing();

            static_cast<XLOPER12&>(*this) = XLOPER12();
        }

        Variant& operator=(const Variant& v)
        {
            xltype = v.xltype;

            if (xll::holds_alternative<xll::Bool>(v))
                xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);
            else if (xll::holds_alternative<xll::Error>(v))
                xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);
            else if (xll::holds_alternative<xll::Int>(v))
                xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);
            else if (xll::holds_alternative<xll::Number>(v))
                xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);
            else if (xll::holds_alternative<xll::String>(v))
                xll::get<xll::String>(*this) = xll::get<xll::String>(v);
            else if (xll::holds_alternative<xll::Nil>(v))
                xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);
            else if (xll::holds_alternative<xll::Missing>(v))
                xll::get<xll::Missing>(*this) = xll::get<xll::Missing>(v);

            return *this;
        }

        Variant& operator=(Variant&& v) noexcept
        {
            xltype = v.xltype;

            if (xll::holds_alternative<xll::Bool>(v))
                xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);
            else if (xll::holds_alternative<xll::Error>(v))
                xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);
            else if (xll::holds_alternative<xll::Int>(v))
                xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);
            else if (xll::holds_alternative<xll::Number>(v))
                xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);
            else if (xll::holds_alternative<xll::String>(v))
                xll::get<xll::String>(*this) = xll::get<xll::String>(v);
            else if (xll::holds_alternative<xll::Nil>(v))
                xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);
            else if (xll::holds_alternative<xll::Missing>(v))
                xll::get<xll::Missing>(*this) = xll::get<xll::Missing>(v);

            return *this;
        }

        template<typename T>
            requires impl::is_valid_type<T>
        Variant& operator=(const T& t)
        {
            this->~Variant();
            static_cast<XLOPER12&>(*this) = XLOPER12();
            T::lift(*static_cast<XLOPER12*>(this)) = T();
            xll::get<T>(*this) = t;
            return *this;
        }

        // template<typename T>
        //     requires impl::is_valid_type<T>
        // Variant& operator=(T&& t) noexcept
        // {
        //     this->~Variant();
        //     static_cast<XLOPER12&>(*this) = XLOPER12();
        //     T::lift(*static_cast<XLOPER12*>(this)) = T();
        //     xll::get<T>(*this) = t;
        //     return *this;
        // }

        [[nodiscard]]
        auto index() const
        {
            return xltype;
        }

        XLOPER12& operator*() { return static_cast<XLOPER12&>(*this); }
    };

    template<typename T>
        requires impl::is_valid_type<T>
    bool holds_alternative(const Variant& v)
    {
        if (std::same_as<T, Bool> && v.xltype == xltypeBool) return true;
        if (std::same_as<T, Error> && v.xltype == xltypeErr) return true;
        if (std::same_as<T, Int> && v.xltype == xltypeInt) return true;
        if (std::same_as<T, Number> && v.xltype == xltypeNum) return true;
        if (std::same_as<T, String> && v.xltype == xltypeStr) return true;
        if (std::same_as<T, Nil> && v.xltype == xltypeNil) return true;
        if (std::same_as<T, Missing> && v.xltype == xltypeMissing) return true;

        return false;
    }

    template<typename T>
        requires impl::is_valid_type<T>
    T& get(Variant& v)
    {
        if (holds_alternative<T>(v) == false) throw std::bad_variant_access();
        return T::lift(*static_cast<XLOPER12*>(&v));
    }

    template<typename T>
        requires impl::is_valid_type<T>
    const T& get(const Variant& v)
    {
        if (holds_alternative<T>(v) == false) throw std::bad_variant_access();
        return T::lift(*static_cast<const XLOPER12*>(&v));
    }


    template<typename Visitor>
    auto visit(Visitor&& vis, Variant& va)
    {
        if (holds_alternative<xll::Bool>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::Bool>(va));
        if (holds_alternative<xll::Error>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::Error>(va));
        if (holds_alternative<xll::Int>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::Int>(va));
        if (holds_alternative<xll::Number>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::Number>(va));
        if (holds_alternative<xll::String>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::String>(va));
        if (holds_alternative<xll::Nil>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::Nil>(va));
        if (holds_alternative<xll::Missing>(va)) return std::invoke(std::forward<Visitor>(vis), get<xll::Missing>(va));

        throw std::bad_variant_access();
    }

}    // namespace xll