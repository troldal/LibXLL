//
// Error Policy Examples for xll::Expected
//
// This file demonstrates how to use custom error policies with xll::Expected
// to specify default error values when construction fails.
//

#include "../LibXLL.Library/Types/Expected.hpp"
#include "../LibXLL.Library/Types/String.hpp"
#include "../LibXLL.Library/Types/Number.hpp"
#include "../LibXLL.Library/Types/Error.hpp"
#include <iostream>

// Example 1: Using the default error policy (default construction)
void example_default_policy() {
    std::cout << "Example 1: Default Error Policy\n";
    std::cout << "================================\n";

    // Uses DefaultErrorPolicy<xll::Error> - creates Error{} on failure
    xll::Expected<xll::Number, xll::Error> exp1;

    // When emplace fails, it creates a default-constructed Error
    try {
        exp1.emplace(/* some value that throws */);
    } catch (...) {
        // exp1 now contains Error{} (xll::ErrNull by default)
    }

    std::cout << "Default policy creates Error{} on construction failure\n\n";
}

// Example 2: Using StringErrorPolicy with a custom message
void example_string_policy() {
    std::cout << "Example 2: String Error Policy\n";
    std::cout << "==============================\n";

    // Define an Expected with a custom error message
    using MyExpected = xll::Expected<
        xll::Number,
        xll::String,
        xll::StringErrorPolicy<"Construction failed">
    >;

    MyExpected exp;

    // When emplace fails, it creates String("Construction failed")
    try {
        exp.emplace(/* some value that throws */);
    } catch (...) {
        if (!exp.has_value()) {
            // exp.error() returns String("Construction failed")
            std::cout << "Error message: " << std::string(exp.error()) << "\n";
        }
    }

    std::cout << "\n";
}

// Example 3: Using ErrorCodePolicy for specific Excel errors
void example_error_code_policy() {
    std::cout << "Example 3: Error Code Policy\n";
    std::cout << "============================\n";

    // Create Expected that uses #VALUE! error on construction failure
    using MyExpected = xll::Expected<
        xll::Number,
        xll::Error,
        xll::ErrorCodePolicy<xll::ErrValue>
    >;

    MyExpected exp;

    // When emplace fails, it creates xll::ErrValue (#VALUE!)
    try {
        exp.emplace(/* some value that throws */);
    } catch (...) {
        if (!exp.has_value()) {
            // exp.error() returns xll::ErrValue
            std::cout << "Error: " << std::string(exp.error().to_string()) << "\n";
        }
    }

    std::cout << "\n";
}

// Example 4: Creating a custom error policy
struct CustomErrorPolicy {
    static constexpr xll::String create() {
        return xll::String("Custom error: Operation failed");
    }
};

void example_custom_policy() {
    std::cout << "Example 4: Custom Error Policy\n";
    std::cout << "==============================\n";

    using MyExpected = xll::Expected<
        xll::Number,
        xll::String,
        CustomErrorPolicy
    >;

    MyExpected exp;

    // When emplace fails, it uses CustomErrorPolicy::create()
    try {
        exp.emplace(/* some value that throws */);
    } catch (...) {
        if (!exp.has_value()) {
            std::cout << "Error: " << std::string(exp.error()) << "\n";
        }
    }

    std::cout << "\n";
}

// Example 5: Different policies for different Expected types
void example_multiple_policies() {
    std::cout << "Example 5: Multiple Policies\n";
    std::cout << "============================\n";

    // Different Expected types with different error policies
    using ValidationExpected = xll::Expected<
        xll::String,
        xll::String,
        xll::StringErrorPolicy<"Validation failed">
    >;

    using CalculationExpected = xll::Expected<
        xll::Number,
        xll::String,
        xll::StringErrorPolicy<"Calculation error">
    >;

    using ExcelExpected = xll::Expected<
        xll::Number,
        xll::Error,
        xll::ErrorCodePolicy<xll::ErrValue>
    >;

    std::cout << "Created three Expected types with different error policies:\n";
    std::cout << "  - ValidationExpected: 'Validation failed'\n";
    std::cout << "  - CalculationExpected: 'Calculation error'\n";
    std::cout << "  - ExcelExpected: #VALUE! error\n\n";
}

// Example 6: Policy-based error recovery
template<typename TErrorPolicy>
void process_with_policy() {
    using MyExpected = xll::Expected<xll::Number, xll::String, TErrorPolicy>;

    MyExpected exp;

    try {
        // Try to construct a value that might fail
        exp.emplace(42.0);
        std::cout << "Success! Value: " << exp.value() << "\n";
    } catch (...) {
        // On failure, the policy determines the error
        std::cout << "Failed with error: " << std::string(exp.error()) << "\n";
    }
}

void example_policy_templates() {
    std::cout << "Example 6: Policy Templates\n";
    std::cout << "===========================\n";

    std::cout << "Using StringErrorPolicy<\"Network error\">:\n";
    process_with_policy<xll::StringErrorPolicy<"Network error">>();

    std::cout << "\nUsing StringErrorPolicy<\"Database error\">:\n";
    process_with_policy<xll::StringErrorPolicy<"Database error">>();

    std::cout << "\n";
}

int main() {
    example_default_policy();
    example_string_policy();
    example_error_code_policy();
    example_custom_policy();
    example_multiple_policies();
    example_policy_templates();

    return 0;
}

