//
// Created by kenne on 03/04/2025.
//

#pragma once

#include <stdexcept>
#include <format>
#include <source_location>

namespace xll::detail {
    // Overload without message
    [[noreturn]] inline void ensure_failed(
        const char* condition,
        const std::source_location& loc = std::source_location::current())
    {
        throw std::runtime_error(
            std::format("Ensure failed: {} at {}:{}",
                condition, loc.file_name(), loc.line())
        );
    }

    // Overload with message
    [[noreturn]] inline void ensure_failed(
        const char* condition,
        const std::source_location& loc,
        const char* message)
    {
        throw std::runtime_error(
            std::format("Ensure failed: {} - {} at {}:{}",
                condition, message, loc.file_name(), loc.line())
        );
    }
}

// Internal implementation macros - DO NOT USE DIRECTLY
// These macros handle argument counting and expansion for MSVC compatibility
#define XLL_ENSURE_EXPAND_(x) x
#define XLL_ENSURE_GET_MACRO_IMPL_(_1, _2, NAME, ...) NAME
#define XLL_ENSURE_GET_MACRO_(...) XLL_ENSURE_EXPAND_(XLL_ENSURE_GET_MACRO_IMPL_(__VA_ARGS__, XLL_ENSURE_2_, XLL_ENSURE_1_, ))

// Internal: Macro for 1 argument (condition only)
#define XLL_ENSURE_1_(condition) \
    do { \
        if (!(condition)) { \
            ::xll::detail::ensure_failed(#condition, std::source_location::current()); \
        } \
    } while (0)

// Internal: Macro for 2 arguments (condition and message)
#define XLL_ENSURE_2_(condition, message) \
    do { \
        if (!(condition)) { \
            ::xll::detail::ensure_failed(#condition, std::source_location::current(), message); \
        } \
    } while (0)

// Public API - Use this macro for runtime assertions
// Usage: ensure(condition) or ensure(condition, "error message")
#define XLL_ENSURE(...) \
    XLL_ENSURE_EXPAND_(XLL_ENSURE_GET_MACRO_(__VA_ARGS__)(__VA_ARGS__))

