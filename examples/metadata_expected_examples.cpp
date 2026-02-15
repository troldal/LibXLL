// Example: Demonstrating the new metadata-based xll::Expected capabilities

#include "Types/Expected.hpp"
#include "Types/String.hpp"
#include "Types/Int.hpp"
#include "Types/Number.hpp"
#include "Types/Error.hpp"

// Example 1: String value with Int error code
xll::Expected<xll::String, xll::Int> fetchUserName(int userId) {
    if (userId < 0) {
        return xll::Unexpected(xll::Int(-1));  // Error code -1
    }
    if (userId > 1000) {
        return xll::Unexpected(xll::Int(-2));  // Error code -2
    }
    return xll::String(L"John Doe");
}

// Example 2: Number value with String error message
xll::Expected<xll::Number, xll::String> divide(double a, double b) {
    if (b == 0.0) {
        return xll::Unexpected(xll::String(L"Division by zero"));
    }
    return xll::Number(a / b);
}

// Example 3: Monadic pipeline with mixed types
auto processPipeline(int userId) {
    return fetchUserName(userId)
        .transform([](const xll::String& name) {
            // Transform String to Number (string length)
            return xll::Number(static_cast<double>(name.to_wstring().length()));
        })
        .and_then([](const xll::Number& length) -> xll::Expected<xll::Number, xll::Int> {
            // Validate length
            if (length.to_double() < 3) {
                return xll::Unexpected(xll::Int(-3));
            }
            return length;
        });
}

// Example 4: Complex error handling
xll::Expected<xll::Number, xll::String> calculatePrice(
    const xll::String& product,
    xll::Int quantity
) {
    // Validate inputs
    if (quantity.to_int() <= 0) {
        return xll::Unexpected(xll::String(L"Invalid quantity"));
    }

    if (product.to_wstring().empty()) {
        return xll::Unexpected(xll::String(L"Invalid product name"));
    }

    // Calculate price
    double basePrice = 10.0;
    double totalPrice = basePrice * quantity.to_int();

    // Apply discount
    if (quantity.to_int() > 100) {
        totalPrice *= 0.9;  // 10% discount
    }

    return xll::Number(totalPrice);
}

// Example 5: Chaining operations with different error types
auto complexWorkflow() {
    // Start with String/Int Expected
    xll::Expected<xll::String, xll::Int> step1 = fetchUserName(42);

    // Transform to Number/Int
    auto step2 = step1.transform([](const xll::String& name) {
        return xll::Number(static_cast<double>(name.to_wstring().length()));
    });

    // Convert error type from Int to String
    auto step3 = step2.transform_error([](const xll::Int& errCode) {
        return xll::String(L"Error code: " + std::to_wstring(errCode.to_int()));
    });

    // Now we have Expected<Number, String>!
    return step3;
}

// Example 6: Using with monadic operators
auto example_with_pipe() {
    using namespace xll;

    auto result = fetchUserName(42)
        | transform([](const String& name) {
            return Number(static_cast<double>(name.to_wstring().length()));
        })
        | transform_error([](const Int& code) {
            return String(L"Error: " + std::to_wstring(code.to_int()));
        });

    // result is Expected<Number, String>
    return result;
}

// Example 7: Error recovery with or_else
xll::Expected<xll::String, xll::Int> getUserNameWithFallback(int userId) {
    return fetchUserName(userId)
        .or_else([](const xll::Int& errorCode) -> xll::Expected<xll::String, xll::Int> {
            // Provide fallback based on error code
            if (errorCode.to_int() == -1) {
                return xll::String(L"Anonymous");
            }
            return xll::Unexpected(errorCode);  // Propagate error
        });
}

// Example 8: Value transformation with error propagation
xll::Expected<xll::Int, xll::String> countCharacters(int userId) {
    return fetchUserName(userId)
        .transform([](const xll::String& name) {
            return xll::Int(static_cast<int>(name.to_wstring().length()));
        })
        .transform_error([](const xll::Int& code) {
            return xll::String(L"Failed to fetch name: error code " +
                              std::to_wstring(code.to_int()));
        });
}

