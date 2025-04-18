//
// Created by kenne on 27/03/2025.
//

#pragma once

#include <Auto/Auto.hpp>
#include <Types/Array.hpp>
#include <concepts>
#include <functional>
#include <string>

namespace xll
{

    // template<typename T, typename Function>
    //     requires(not std::ranges::range<T> and std::invocable<Function, T>)
    // constexpr auto operator|(T&& t, Function&& f) -> std::invoke_result_t<Function, T>
    // {
    //     return std::invoke(std::forward<Function>(f), std::forward<T>(t));
    // }


    // template<typename T, typename Function>
    //     requires(not std::ranges::range<T> and std::invocable<Function, T>)
    // constexpr auto operator|(T&& t, Function&& f) -> std::invoke_result_t<Function, T>
    // {
    //     return std::invoke(std::forward<Function>(f), std::forward<T>(t));
    // }
    //
    // template<typename T, template<typename> class TFunction>
    //     requires (not std::ranges::range<T> and std::invocable<TFunction<T>, T>)
    // constexpr auto operator|(const T& t, TFunction<T> f) -> std::invoke_result_t<TFunction<T>, T>
    // {
    //     return std::invoke(std::forward<TFunction<T>>(f), std::forward<T>(t));
    // }

    template<typename T, typename E, typename TFunc>
        requires std::invocable<TFunc, xll::Expected<T, E>>
    constexpr auto operator|(const xll::Expected<T, E>& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Expected<T, E>>
    {
        return std::invoke(std::forward<TFunc>(f), t);
    }




    template<typename TFunc>
    requires std::invocable<TFunc, xll::Function>
    constexpr auto operator|(const xll::Function& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Function>
    {
        return std::invoke(std::forward<TFunc>(f), t);
    }

    template<typename TFunc>
        requires std::invocable<TFunc, xll::Function>
    constexpr auto operator|(xll::Function&& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Function>
    {
        return std::invoke(std::forward<TFunc>(f), std::forward<xll::Function>(t));
    }

    // template<typename T, template<typename> class TFunction>
    //     requires (not std::ranges::range<T> and std::invocable<TFunction<T>, T>)
    // constexpr auto operator|(const T& t, TFunction<T> f) -> std::invoke_result_t<TFunction<T>, T>
    // {
    //     return std::invoke(std::forward<TFunction<T>>(f), std::forward<T>(t));
    // }

    // template<typename T, typename Function>
    // // requires(not std::ranges::range<T> and std::invocable<Function, T>)
    // constexpr auto operator|(Array<T> t, Function&& f) -> std::invoke_result_t<Function, Array<T>>
    // {
    //     return std::invoke(std::forward<Function<Array<T>>>(f), t);
    // }

    template<typename T, typename Function>
        // requires(not std::ranges::range<T> and std::invocable<Function, T>)
        requires std::same_as<Function, decltype(AutoFree())>
    constexpr auto operator|(const Array<T> t, Function&& f) -> std::invoke_result_t<Function, Array<T>>
    {
        return std::invoke(std::forward<Function>(f), t);
    }

}    // namespace xll