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

#include <Register/Registry.hpp>
#include <functional>
#include <tl/expected.hpp>
#include <memory>
#include <type_traits>

namespace xll
{
    struct Open{};
    struct Close{};
    struct Add{};
    struct Remove{};

    template<typename>
    class Auto
    {
        using TFunction = std::function<tl::expected<std::monostate, std::string>()>;
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

        static tl::expected<std::monostate, std::string> Execute()
        {
            for (const auto& func : functions)
                if (auto result = func(); !result) return result;

            return std::monostate {};
        }

        template<typename TPhase>
        static tl::expected<std::monostate, std::string> Execute()
        {
            static_assert(std::same_as<TPhase, BeforeTag> || std::same_as<TPhase, AfterTag>);

            if constexpr (std::same_as<TPhase, BeforeTag>)
                for (const auto& func : s_before)
                    if (auto result = func(); !result) return result;

            if constexpr (std::same_as<TPhase, AfterTag>)
                for (const auto& func : s_after)
                    if (auto result = func(); !result) return result;

            return std::monostate {};
        }

        static void HandleError(const std::string& err)
        {
            for (const auto& func : s_onError) func(err);
        }
    };

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

    using AutoOpen = Auto<Open>;
    using AutoAdd = Auto<Add>;
    using AutoClose = Auto<Close>;
    using AutoRemove = Auto<Remove>;

    template<typename TEvent, typename TFunc>
    int xlAuto(const std::string& funcName, TFunc&& func)
    {
        try {
            xll::Registry::instance().register_all();
            if (!xll::Auto<TEvent>::template Execute<typename xll::Auto<TEvent>::BeforeTag>()) return XLL_FAILURE;

            func();
            // if (!Auto<Add>::Call()) {
            //     return FALSE;
            // }

            if (!xll::Auto<TEvent>::template Execute<typename xll::Auto<TEvent>::AfterTag>()) return XLL_FAILURE;
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

    // Helper trait to detect unique_ptr
    template<typename>
    struct is_unique_ptr : std::false_type
    {
    };

    template<typename T, typename D>
    struct is_unique_ptr<std::unique_ptr<T, D>> : std::true_type
    {
    };

    template<typename T>
    inline constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

    inline auto AutoFree()
    {
        return []<typename T>(T&& arg) {
            if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
                arg->xltype |= xlbitDLLFree;
                return arg;
            }
            else if constexpr (is_unique_ptr_v<std::remove_reference_t<T>>) {
                arg->xltype |= xlbitDLLFree;
                return arg.release();
            }
            else {
                using U  = std::decay_t<T>;
                auto ptr = std::make_unique<U>(std::forward<T>(arg));
                ptr->xltype |= xlbitDLLFree;
                return ptr.release();
            }
        };
    }


}    // namespace xll

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