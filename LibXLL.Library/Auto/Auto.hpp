/*

     .d88888b.                             Y88b   d88P 888      888
    d88P" "Y88b                             Y88b d88P  888      888
    888     888                              Y88o88P   888      888
    888     888 88888b.   .d88b.  88888b.     Y888P    888      888
    888     888 888 "88b d8P  Y8b 888 "88b    d888b    888      888
    888     888 888  888 88888888 888  888   d88888b   888      888
    Y88b. .d88P 888 d88P Y8b.     888  888  d88P Y88b  888      888
     "Y88888P"  88888P"   "Y8888  888  888 d88P   Y88b 88888888 88888888
                888
                888
                888

    This code is based on xlladdins by Keith Lewis provided under the MIT License.
    Modifications have been made by Kenneth Troldal Balslev/KinetiQ.dev

    MIT License

    Copyright (c) 2022 xlladdins
    Copyright (c) 2025 Kenneth Troldal Balslev and KinetiQ.dev

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include "../Utils/Concepts.hpp"
#include <Macros/Defines.hpp>
#include <Register/Registry.hpp>
#include <functional>
#include <tl/expected.hpp>
#include "../xlFunctions/Register.hpp"

#include <Register/Function.hpp>

namespace xll
{
    struct Open{};
    struct Close{};
    struct Add{};
    struct Remove{};
    struct Free{};

    template<typename>
    class Auto
    {
        // using TFunction = std::function<tl::expected<std::monostate, std::string>()>;
        using TFunction = std::function<void()>;
        inline static std::vector<TFunction> functions {};

        inline static std::vector<TFunction>                        s_before {};
        inline static std::vector<TFunction>                        s_after {};
        inline static std::vector<std::function<void(std::string)>> s_onError {};

        std::vector<TFunction>                        m_before {};
        std::vector<TFunction>                        m_after {};
        std::vector<std::function<void(std::string)>> m_onError {};

    public:
        struct BeforeTag{};
        struct AfterTag{};
        struct OnErrorTag{};

        Auto() = default;

        explicit Auto(TFunction&& func) { functions.emplace_back(func); }

        template<typename TPhase>
        explicit Auto(TPhase&&, TFunction&& func)
        {
            if constexpr (std::same_as<TPhase, BeforeTag>)
                s_before.emplace_back(std::forward<TFunction>(func));
            else if constexpr (std::same_as<TPhase, AfterTag>)
                s_after.emplace_back(std::forward<TFunction>(func));
        }

        template<typename TErrorHandler>
        explicit Auto(OnErrorTag&&, TErrorHandler&& func)
        {
            s_onError.emplace_back(std::forward<TErrorHandler>(func));
        }

        template<typename TFunc>
        Auto& Before(TFunc&& func)
        {
            m_before.emplace_back(std::forward<TFunc>(func));
            return *this;
        }

        template<typename TFunc>
        Auto& After(TFunc&& func)
        {
            m_after.emplace_back(std::forward<TFunc>(func));
            return *this;
        }

        template<typename TFunc>
        Auto& OnError(TFunc&& func)
        {
            m_onError.emplace_back(std::forward<TFunc>(func));
            return *this;
        }

        Auto& Register()
        {
            s_before  = std::move(m_before);
            s_after   = std::move(m_after);
            s_onError = std::move(m_onError);

            m_before  = {};
            m_after   = {};
            m_onError = {};

            return *this;
        }

        // static tl::expected<std::monostate, std::string> Execute()
        // {
        //     for (const auto& func : functions)
        //         if (auto result = func(); !result) return result;
        //
        //     return std::monostate {};
        // }
        //
        // template<typename TPhase>
        // static tl::expected<std::monostate, std::string> Execute()
        // {
        //     static_assert(std::same_as<TPhase, BeforeTag> || std::same_as<TPhase, AfterTag>);
        //
        //     if constexpr (std::same_as<TPhase, BeforeTag>)
        //         for (const auto& func : s_before)
        //             if (auto result = func(); !result) return result;
        //
        //     if constexpr (std::same_as<TPhase, AfterTag>)
        //         for (const auto& func : s_after)
        //             if (auto result = func(); !result) return result;
        //
        //     return std::monostate {};
        // }

        static void Execute()
        {
            for (const auto& func : functions) func();

        }

        template<typename TPhase>
        static void Execute()
        {
            static_assert(std::same_as<TPhase, BeforeTag> || std::same_as<TPhase, AfterTag>);

            if constexpr (std::same_as<TPhase, BeforeTag>)
                for (const auto& func : s_before) func();

            if constexpr (std::same_as<TPhase, AfterTag>)
                for (const auto& func : s_after) func();
        }

        static void HandleError(const std::string& err)
        {
            for (const auto& func : s_onError) func(err);
        }

    };

    template<typename TEvent, typename TFunc>
        requires std::invocable<TFunc, xll::Auto<TEvent>>
    constexpr auto operator|(xll::Auto<TEvent>&& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Auto<TEvent>>
    {
        return std::invoke(std::forward<TFunc>(f), std::forward<xll::Auto<TEvent>>(t));
    }

    template<typename TFunc>
    auto Before(TFunc&& rhs)
    {
        return [rhs]<typename TAuto>(Auto<TAuto>&& lhs) { return lhs.Before(rhs); };
    }

    template<typename TFunc>
    auto After(TFunc&& rhs)
    {
        return [rhs]<typename TAuto>(Auto<TAuto>&& lhs) { return lhs.After(rhs); };
    }

    template<typename TFunc>
    auto OnError(TFunc&& rhs)
    {
        return [rhs]<typename TAuto>(Auto<TAuto>&& lhs) { return lhs.OnError(rhs); };
    }

    using OnOpen = Auto<Open>;
    using OnAdd = Auto<Add>;
    using OnClose = Auto<Close>;
    using OnRemove = Auto<Remove>;
    using OnFree = Auto<Free>;

    template<typename TEvent, typename TFunc>
    int xlAuto(const std::string& funcName, TFunc&& func)
    {
        try {
            xll::Registry::instance().register_all();
            xll::Auto<TEvent>::template Execute<typename xll::Auto<TEvent>::BeforeTag>();

            func();
            // if (!Auto<Add>::Call()) {
            //     return FALSE;
            // }

            xll::Auto<TEvent>::template Execute<typename xll::Auto<TEvent>::AfterTag>();
        }
        catch (const std::exception& ex) {
            xll::Auto<TEvent>::HandleError(ex.what());
            return XLL_FAILURE;
        }
        catch (...) {
            xll::Auto<TEvent>::HandleError(std::format("Unknown error in {}", funcName));
            return XLL_FAILURE;
        }

        return XLL_SUCCESS;
    }

    inline auto AutoFree()
    {
        return []<typename T>(T&& arg) {
            if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
                arg->xltype |= xlbitDLLFree;
                return arg;
            }
            else if constexpr (is_unique_ptr<std::remove_reference_t<T>>) {
                arg->xltype |= xlbitDLLFree;
                return arg.release();
            }
            else if constexpr (not std::same_as<std::remove_reference_t<T>, xll::Function>) {
                using U  = std::decay_t<T>;
                auto ptr = std::make_unique<U>(std::forward<T>(arg));
                ptr->xltype |= xlbitDLLFree;
                return ptr.release();
            }

            throw std::runtime_error("AutoFree: Cannot free this type");
        };
    }
}    // namespace xll

extern "C" inline XLL_EXPORTS int XLLAPI xlAutoOpen()
{
    return xll::xlAuto<xll::Open>("xlAutoOpen", [] {
        for (const auto& fn : xll::Function::functionArgs) {
            xll::Variant<xll::Nil, xll::Number> id = xll::Register(xll::impl::All(fn));
            if (not holds_alternative<xll::Number>(id)) {
                throw std::runtime_error(std::format("[xlAutoOpen]: Failed to register function {}", fn.functionName.to_string()));
            }
        }
    });
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoOpen")
#endif

extern "C" inline XLL_EXPORTS int XLLAPI xlAutoClose()
{
    return xll::xlAuto<xll::Close>("xlAutoClose", []{});
    // try {
    //     xll::Registry::instance().register_all();
    //     if (!xll::Auto<xll::Close>::Execute<xll::Auto<xll::Close>::BeforeTag>()) return XLL_FAILURE;
    //
    //     // if (!Auto<CloseBefore>::Call()) {
    //     //     return FALSE;
    //     // }
    //     //
    //     // for (const auto& [key, args] : AddIn::Map) {
    //     //     Excel12(xlfSetName, key);
    //     // }
    //     //
    //     // if (!Auto<Close>::Call()) {
    //     //     return FALSE;
    //     // }
    //
    //     if (!xll::Auto<xll::Close>::Execute<xll::Auto<xll::Close>::AfterTag>()) return XLL_FAILURE;
    // }
    // catch (const std::exception& ex) {
    //     xll::Auto<xll::Close>::HandleError(ex.what());
    //     return XLL_FAILURE;
    // }
    // catch (...) {
    //     xll::Auto<xll::Close>::HandleError("Unknown error in xlAutoOpen");
    //     return XLL_FAILURE;
    // }
    //
    // return XLL_SUCCESS;
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoClose")
#endif

extern "C" inline XLL_EXPORTS int XLLAPI xlAutoAdd()
{
    return xll::xlAuto<xll::Add>("xlAutoAdd", [] {});
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoAdd")
#endif


extern "C" inline XLL_EXPORTS int XLLAPI xlAutoRemove()
{
    return xll::xlAuto<xll::Remove>("xlAutoRemove", [] {});
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoRemove")
#endif

extern "C" inline XLL_EXPORTS void XLLAPI xlAutoFree12(LPXLOPER12 px)
{
    xll::Registry::instance().register_all();
    xll::Auto<xll::Free>::Execute<xll::Auto<xll::Free>::BeforeTag>();

    if (not px->xltype & xlbitDLLFree) return;

    px->xltype &= ~xlbitDLLFree;
    switch (px->xltype) {
        case xltypeMulti:
            delete reinterpret_cast<xll::Array<xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number>>*>(px);
        break;
        case xltypeBool:
            delete reinterpret_cast<xll::Bool*>(px);
        break;
        case xltypeErr:
            delete reinterpret_cast<xll::Error*>(px);
        break;
        case xltypeInt:
            delete reinterpret_cast<xll::Int*>(px);
        break;
        case xltypeMissing:
            delete reinterpret_cast<xll::Missing*>(px);
        break;
        case xltypeNil:
            delete reinterpret_cast<xll::Nil*>(px);
        break;
        case xltypeNum:
            delete reinterpret_cast<xll::Number*>(px);
        break;
        case xltypeStr:
            delete reinterpret_cast<xll::String*>(px);
        break;
        default:
            break;
    }

    xll::Auto<xll::Free>::Execute<xll::Auto<xll::Free>::AfterTag>();
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:xlAutoFree12")
#endif