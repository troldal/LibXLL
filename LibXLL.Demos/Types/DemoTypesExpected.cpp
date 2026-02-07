//
// Created by Copilot on 07/02/2026.
//
// Demonstrates the usage of xll::Expected for error handling

#include <iostream>
#include <Types/Expected.hpp>
#include <Types/String.hpp>
#include <Types/Number.hpp>
#include <Types/Error.hpp>

using namespace xll::literals;

// Example 1: Basic Expected usage with success case
xll::Expected<xll::Number> divide(double numerator, double denominator) {
    if (denominator == 0.0) {
        // Return an error for division by zero
        auto err = xll::Error();
        err.val.err = 7;  // #DIV/0!
        return xll::Unexpected(err);
    }
    return xll::Number(numerator / denominator);
}

// Example 2: String processing that might fail
xll::Expected<xll::String> validate_username(const std::string& username) {
    if (username.empty()) {
        auto err = xll::Error();
        err.val.err = 15;  // #VALUE!
        return xll::Unexpected(err);
    }
    if (username.length() < 3) {
        auto err = xll::Error();
        err.val.err = 15;  // #VALUE!
        return xll::Unexpected(err);
    }
    return xll::String(username);
}

// Example 3: Chaining operations with and_then
xll::Expected<xll::Number> safe_sqrt(double value) {
    if (value < 0) {
        auto err = xll::Error();
        err.val.err = 36;  // #NUM!
        return xll::Unexpected(err);
    }
    return xll::Number(std::sqrt(value));
}

xll::Expected<xll::Number> sqrt_then_inverse(double value) {
    return safe_sqrt(value).and_then([](const xll::Number& num) {
        return divide(1.0, static_cast<double>(num));
    });
}

// Example 4: Using transform to modify values
xll::Expected<xll::String> to_uppercase(const std::string& input) {
    auto result = validate_username(input);
    return result.transform([](const xll::String& str) {
        std::string s = std::string(str);
        for (auto& c : s) c = std::toupper(c);
        return xll::String(s);
    });
}

void demo_basic_usage() {
    std::cout << "=== Basic Usage Demo ===\n\n";

    // Success case
    auto result1 = divide(10.0, 2.0);
    if (result1.has_value()) {
        std::cout << "10 / 2 = " << static_cast<double>(result1.value()) << "\n";
    }

    // Error case
    auto result2 = divide(10.0, 0.0);
    if (!result2) {  // Using bool operator
        std::cout << "10 / 0 = ERROR: " << std::string(result2.error().to_string()) << "\n";
    }

    std::cout << "\n";
}

void demo_value_or() {
    std::cout << "=== value_or() Demo ===\n\n";

    auto result1 = divide(10.0, 2.0);
    auto value1 = result1.value_or(xll::Number(0.0));
    std::cout << "Result with value_or (success): " << static_cast<double>(value1) << "\n";

    auto result2 = divide(10.0, 0.0);
    auto value2 = result2.value_or(xll::Number(-999.0));
    std::cout << "Result with value_or (error): " << static_cast<double>(value2) << "\n";

    std::cout << "\n";
}

void demo_chaining() {
    std::cout << "=== Chaining with and_then() Demo ===\n\n";

    // Success case: sqrt(16) = 4, then 1/4 = 0.25
    auto result1 = sqrt_then_inverse(16.0);
    if (result1.has_value()) {
        std::cout << "1/sqrt(16) = " << static_cast<double>(result1.value()) << "\n";
    }

    // Error case: sqrt(-1) fails
    auto result2 = sqrt_then_inverse(-1.0);
    if (!result2.has_value()) {
        std::cout << "1/sqrt(-1) = ERROR: " << std::string(result2.error().to_string()) << "\n";
    }

    // Error case: sqrt(0) succeeds, but 1/0 fails
    auto result3 = sqrt_then_inverse(0.0);
    if (!result3.has_value()) {
        std::cout << "1/sqrt(0) = ERROR: " << std::string(result3.error().to_string()) << "\n";
    }

    std::cout << "\n";
}

void demo_transform() {
    std::cout << "=== Transform Demo ===\n\n";

    // Success case
    auto result1 = to_uppercase("alice");
    if (result1.has_value()) {
        std::cout << "Uppercase of 'alice': " << std::string(result1.value()) << "\n";
    }

    // Error case - empty string
    auto result2 = to_uppercase("");
    if (!result2.has_value()) {
        std::cout << "Uppercase of '': ERROR: " << std::string(result2.error().to_string()) << "\n";
    }

    // Error case - too short
    auto result3 = to_uppercase("ab");
    if (!result3.has_value()) {
        std::cout << "Uppercase of 'ab': ERROR: " << std::string(result3.error().to_string()) << "\n";
    }

    std::cout << "\n";
}

void demo_or_else() {
    std::cout << "=== or_else() Demo ===\n\n";

    auto result = divide(10.0, 0.0);
    auto recovered = result.or_else([](const xll::Error& err) -> xll::Expected<xll::Number> {
        std::cout << "Error occurred: " << std::string(err.to_string()) << ", returning default value\n";
        return xll::Number(0.0);
    });

    std::cout << "Final value: " << static_cast<double>(recovered.value()) << "\n";
    std::cout << "\n";
}

void demo_copy_and_move() {
    std::cout << "=== Copy and Move Semantics Demo ===\n\n";

    // Copy construction
    auto original = divide(10.0, 2.0);
    auto copy = original;
    std::cout << "Original value: " << static_cast<double>(original.value()) << "\n";
    std::cout << "Copied value: " << static_cast<double>(copy.value()) << "\n";

    // Move construction
    auto source = divide(20.0, 4.0);
    auto moved = std::move(source);
    std::cout << "Moved value: " << static_cast<double>(moved.value()) << "\n";

    // Copy assignment
    auto target = divide(5.0, 1.0);
    target = divide(15.0, 3.0);
    std::cout << "After assignment: " << static_cast<double>(target.value()) << "\n";

    std::cout << "\n";
}

void demo_conversion_to_fxt_expected() {
    std::cout << "=== Conversion to fxt::expected Demo ===\n\n";

    auto xll_result = divide(10.0, 2.0);
    fxt::expected<xll::Number, xll::Error> fxt_result = xll_result;

    if (fxt_result.has_value()) {
        std::cout << "Converted to fxt::expected successfully: "
                  << static_cast<double>(*fxt_result) << "\n";
    }

    // With monadic operations using fxt
    auto result = divide(100.0, 5.0)
        .to_expected()
        .transform([](const xll::Number& n) {
            return xll::Number(static_cast<double>(n) * 2);
        });

    if (result.has_value()) {
        std::cout << "After transform: " << static_cast<double>(*result) << "\n";
    }

    std::cout << "\n";
}

int main() {
    std::cout << "=== xll::Expected Demonstration ===\n\n";

    try {
        demo_basic_usage();
        demo_value_or();
        demo_chaining();
        demo_transform();
        demo_or_else();
        demo_copy_and_move();
        demo_conversion_to_fxt_expected();

        std::cout << "=== All demos completed successfully ===\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}




