//
// Created by kenne on 03/04/2025.
//

#pragma once

#include <stdexcept>
#include <format>
#include <source_location>

#define ensure(condition) \
    do { \
        if (!(condition)) { \
            auto loc = std::source_location::current(); \
            throw std::runtime_error( \
                std::format("Ensure failed: {} at {}:{}", \
                    #condition, \
                    loc.file_name(), \
                    loc.line() \
                ) \
            ); \
        } \
    } while (0)

