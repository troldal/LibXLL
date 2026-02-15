//
// Test file demonstrating error policy functionality
//
#include "../LibXLL.Library/Types/Expected.hpp"
#include "../LibXLL.Library/Types/String.hpp"
#include "../LibXLL.Library/Types/Number.hpp"
#include "../LibXLL.Library/Types/Error.hpp"

// Custom error policy for validation errors
struct ValidationErrorPolicy {
    static constexpr xll::String create() {
        return xll::String("Validation failed: invalid input");
    }
};

// Custom error policy for calculation errors
struct CalculationErrorPolicy {
    static constexpr xll::String create() {
        return xll::String("Calculation failed: division by zero");
    }
};

int main() {
    // Test 1: Default error policy (uses default constructor)
    {
        xll::Expected<xll::Number, xll::Error> exp1;
        // exp1 uses DefaultErrorPolicy<Error> - on emplace failure, creates Error{}
    }

    // Test 2: StringErrorPolicy with compile-time string
    {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Operation failed">
        >;

        MyExpected exp2;
        // On emplace failure, creates String("Operation failed")
    }

    // Test 3: ErrorCodePolicy for Excel errors
    {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::Error,
            xll::ErrorCodePolicy<xll::ErrValue>
        >;

        MyExpected exp3;
        // On emplace failure, creates xll::ErrValue (#VALUE!)
    }

    // Test 4: Custom validation error policy
    {
        using ValidationExpected = xll::Expected<
            xll::String,
            xll::String,
            ValidationErrorPolicy
        >;

        ValidationExpected exp4;
        // On emplace failure, creates String("Validation failed: invalid input")
    }

    // Test 5: Custom calculation error policy
    {
        using CalculationExpected = xll::Expected<
            xll::Number,
            xll::String,
            CalculationErrorPolicy
        >;

        CalculationExpected exp5;
        // On emplace failure, creates String("Calculation failed: division by zero")
    }

    // Test 6: Different error policies for same types
    {
        using Policy1Expected = xll::Expected<xll::Number, xll::String, ValidationErrorPolicy>;
        using Policy2Expected = xll::Expected<xll::Number, xll::String, CalculationErrorPolicy>;

        Policy1Expected exp6a;  // Uses ValidationErrorPolicy
        Policy2Expected exp6b;  // Uses CalculationErrorPolicy
        // Same value/error types, different policies!
    }

    // Test 7: Type aliases still work with default policy
    {
        xll::ExpNumber exp7a;  // Uses Expected<Number, Error, DefaultErrorPolicy<Error>>
        xll::ExpString exp7b;  // Uses Expected<String, Error, DefaultErrorPolicy<Error>>
        xll::ExpInt exp7c;     // Uses Expected<Int, Error, DefaultErrorPolicy<Error>>
    }

    return 0;
}

