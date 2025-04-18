//
// Created by kenne on 24/03/2025.
//

#include "Types/Array.hpp"
#include "Types/String.hpp"
#include "Types/Variant.hpp"
#include <Register.hpp>
#include <Types/Expected.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <ranges>
#include <OpenXLL.hpp>

// extern "C" xll::Number* XLLAPI MakeNum(xll::Number const* d, xll::Number const* arr);
// extern "C" void XLLAPI xlAutoFree12(LPXLOPER12 px);

int main() {

    using namespace xll::literals;

    // auto value = xll::Array<xll::Expected<xll::String>>();
    // value = xll::Array<xll::Expected<xll::String>>(4,1);
    // auto value2 = value;
    // //value = value2;
    //
    // xll::Int invalid;
    // invalid.xltype = xltypeNil;
    //
    // // Invalid copy construction
    // auto i = xll::Int(invalid);

    //for (auto& e : value) e = "Blah"_xs;
    using namespace xll::literals;

    auto value = xll::Array<xll::String>(4,1);
    for (auto& v : value) { v = "Hello"_xs; }
    auto arr = value.to<std::vector, std::string>();

    auto inp = xll::Expected<xll::String>("Input");
    auto exp = inp.to_expected();
    auto out = exp | fxt::transform([](xll::String) { return "Hello, World!"_xs; });

    std::cout << *out << std::endl;



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