//
// Created by kenne on 22/03/2025.
//

#pragma once

#include <unordered_map>
#include <mutex>
#include <thread>

namespace xll {

    class MemoryManager{

        MemoryManager() = default;
        ~MemoryManager() = default;

        //gtl::parallel_node_hash_map<LPXLOPER12, std::unique_ptr<XLOPER12>> m_xlopers {};
        std::unordered_map<LPXLOPER12, std::unique_ptr<XLOPER12>> m_xlopers {};
        std::mutex mutex;


    public:

        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator = (const MemoryManager&) = delete;

        LPXLOPER12 add(const XLOPER12& obj)
        {
            auto val = std::make_unique<XLOPER12>(obj);
            auto key = val.get();

            const std::lock_guard<std::mutex> lock(mutex);

            m_xlopers[key] = std::move(val);
            return m_xlopers[key].get();
            // return key;
        }

        void erase(LPXLOPER12 ptr)
        {
            using namespace std::literals;

            // if (!m_xlopers.contains(ptr))
                // std::this_thread::sleep_for(100ms);
            // if (!m_xlopers.contains(ptr))
                // return;

            const std::lock_guard<std::mutex> lock(mutex);

            auto sz = m_xlopers.size();
            m_xlopers.erase(ptr);
            sz = m_xlopers.size();
        }

        static MemoryManager& Instance(){
            static MemoryManager instance;        // (1)
            return instance;
        }
    };

}