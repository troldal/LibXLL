//
// Created by kenne on 24/03/2025.
//

#include "Types/Array.hpp"
#include "Types/String.hpp"
#include "Types/Variant.hpp"
#include <Register.hpp>
#include "Types/Array.hpp"
#include "Types/String.hpp"
#include "Types/Variant.hpp"
#include <Register.hpp>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>

extern "C" xll::Number* XLLAPI MakeNum(xll::Number const* d, xll::Number const* arr);
extern "C" void XLLAPI xlAutoFree12(LPXLOPER12 px);

int main() {
    // Set the end time 60 seconds from now.
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(10);

    // Lambda function to be executed by each thread.
    auto threadFunc = [=]() {
        while (std::chrono::steady_clock::now() < end_time) {
            auto result = MakeNum(nullptr, nullptr);
            //std::cerr << result->to<double>() << std::endl;
            if (result->xltype & xlbitDLLFree) {
                xlAutoFree12(result);
            }
        }
    };

    // Create and launch 8 threads.
    std::vector<std::thread> threads;
    for (int i = 0; i < 1; ++i) {
        threads.emplace_back(threadFunc);
    }

    // Wait for all threads to finish.
    for (auto &t : threads) {
        t.join();
    }

    return 0;
}


// int main()
// {
    // const auto MakeNum_ =
    //     xll::Function()
    //     | xll::ReturnType<xll::Array>()
    //     | xll::ProcedureName("MakeNum")
    //     | xll::FunctionName("MAKE_NUM")
    //     | xll::Hidden()
    //     | xll::Argument<xll::Variant>("first", "The first argument")
    //     | xll::Argument<xll::Array>("second", "The second argument");



    // auto result = MakeNum(nullptr, nullptr);
    // std::cerr << result->to<double>() << std::endl;
    //
    // if (result->xltype & xlbitDLLFree) xlAutoFree12(result);

    // const auto MakeNum_ =
    // xll::Function<xll::Number>();

    // xll::String arg1 = xll::String("arg1");
    // xll::String arg2 = xll::String("arg2");
    //
    // auto result = MakeNum(&arg1, &arg2);
    // std::cout << *result << std::endl;

    // std::string* str = new std::string("Hello");
    // std::cout << *str << std::endl;

    // auto arg3 = *arg1;
    // auto arg4 = xll::String::lift(&arg3);

    // return 0;
// }