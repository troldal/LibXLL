//
// Created by kenne on 27/03/2025.
//

#pragma once

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

    template<typename T, typename Function>
        requires(not std::ranges::range<T> and std::invocable<Function, T>)
    constexpr auto operator|(T&& t, Function&& f) -> std::invoke_result_t<Function, T>
    {
        return std::invoke(std::forward<Function>(f), std::forward<T>(t));
    }

    template<typename T, template<typename> class TFunction>
        requires (not std::ranges::range<T> and std::invocable<TFunction<T>, T>)
    constexpr auto operator|(const T& t, TFunction<T> f) -> std::invoke_result_t<TFunction<T>, T>
    {
        return std::invoke(std::forward<TFunction<T>>(f), std::forward<T>(t));
    }

}    // namespace xll