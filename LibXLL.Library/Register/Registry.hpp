//
// Created by kenne on 29/03/2025.
//

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <mutex>

namespace xll
{

    // 1. Define a registry class to store all registerable objects
    class Registry
    {
    private:
        // Private constructor (singleton pattern)
        Registry() = default;

        // Collection of objects to register
        std::vector<std::function<void()>> register_functions;

        static inline std::once_flag init_flag {};

    public:
        // Get singleton instance
        static Registry& instance()
        {
            static Registry registry;
            return registry;
        }

        // Add a function to the registry
        template<typename T>
        void add(T& obj)
        {
            register_functions.push_back([&obj]() { obj.Register(); });
        }

        // Register all functions (called by DllMain)
        void register_all() const
        {
            std::call_once(init_flag, [&] {
                for (auto& func : register_functions) func();
            });

            // for (auto& func : register_functions) {
            //     func();
            // }
        }
    };

}    // namespace xll