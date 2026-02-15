//
// Created by Copilot on 14/02/2026.
//
// Excel Add-in Demo: String validation using xll::Expected<xll::String, xll::String>
// This demonstrates using Expected with same type for value and error,
// validating string format and returning either "SUCCESS" or an Excel error.

#include <Auto.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>
#include <iostream>
#include <string>
#include <regex>

using namespace xll::literals;

// ============================================================================
// String Validation Function
// ============================================================================

/**
 * @brief Validates a string against a specific format.
 *
 * Format rules:
 * - Must not be empty
 * - Must be 5-20 characters long
 * - Must start with uppercase letter
 * - Must contain only alphanumeric characters and hyphens
 * - Must not start or end with hyphen
 *
 * @param input The string to validate (as xll::Expected<xll::String, xll::String>)
 * @return xll::Expected<xll::String, xll::String>*
 *         - Value state: String "SUCCESS" if validation passes
 *         - Error state: Error message describing the validation failure
 *                       (logged to console and returned as #VALUE! to Excel)
 */
xll::Expected<xll::String, xll::String> validate_string_format(const xll::String& input) {
    std::string str(input);

    // Rule 1: Must not be empty
    if (str.empty()) {
        return xll::Unexpected(xll::String("Error: String cannot be empty"));
    }

    // Rule 2: Length must be 5-20 characters
    if (str.length() < 5) {
        return xll::Unexpected(xll::String("Error: String must be at least 5 characters"));
    }
    if (str.length() > 20) {
        return xll::Unexpected(xll::String("Error: String must be at most 20 characters"));
    }

    // Rule 3: Must start with uppercase letter
    if (!std::isupper(static_cast<unsigned char>(str[0]))) {
        return xll::Unexpected(xll::String("Error: String must start with uppercase letter"));
    }

    // Rule 4: Must contain only alphanumeric characters and hyphens
    for (char c : str) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
            return xll::Unexpected(xll::String("Error: String must contain only alphanumeric characters and hyphens"));
        }
    }

    // Rule 5: Must not start or end with hyphen
    if (str[0] == '-' || str[str.length() - 1] == '-') {
        return xll::Unexpected(xll::String("Error: String must not start or end with hyphen"));
    }

    // All validation passed
    return xll::String("SUCCESS");
}

// ============================================================================
// Excel Function Registration
// ============================================================================

auto expectedValidation =
    xll::Function("XLL.EXPECTED")
    | xll::Result<xll::Expected<xll::String, xll::Error>>()
    | xll::Procedure("XLLExpected")
    | xll::Parameter<xll::Expected<xll::String, xll::Error>>("Input", "The string to validate")
    | xll::ThreadSafe()
    | xll::Category("Expected Demos")
    | xll::Description("Validates a string format. Returns 'SUCCESS' or #VALUE! error.");
XLL_REGISTER(expectedValidation);


/**
 * @brief Error policy for String errors with a custom message.
 *
 * This policy creates String error values with a predefined error message.
 * Useful when you want a specific error message for construction failures.
 *
 * @example
 * @code
 * using MyExpected = Expected<Number, String, StringErrorPolicy<"Construction failed">>;
 * MyExpected exp;
 * // On construction failure, creates String("Construction failed")
 * @endcode
 */
struct StringErrorPolicy {
    static xll::String create() {
        return {"Error: Invalid input type"};
    }
};

/**
 * @brief Excel add-in function that validates string format using monadic operations.
 *
 * This function demonstrates the use of xll::Expected<xll::String, xll::String>
 * with monadic operations in an Excel add-in context. It:
 * 1. Receives an Expected<String> from Excel (handles non-string inputs gracefully)
 * 2. Uses | xll::transform_error() to convert any input errors to string errors
 * 3. Uses | xll::and_then() to chain validation (only if input is valid string)
 * 4. Uses | xll::or_else() to handle errors and log them
 * 5. Uses | xll::transform_error() to convert string errors to Excel errors
 * 6. Returns "SUCCESS" on valid input or appropriate error (#VALUE!, etc.)
 *
 * Thanks to lazy error materialization in Expected::error(), invalid input types
 * (Missing, Nil, wrong types) are automatically converted to default errors when accessed.
 *
 * @param input Pointer to xll::Expected<xll::String> containing the input from Excel
 * @return Pointer to xll::Expected<xll::String> containing either "SUCCESS" string or xll::Error
 */
XLL_FUNCTION xll::Expected<xll::String, xll::Error>* XLLAPI XLLExpected(
    xll::Expected<xll::String, xll::String, StringErrorPolicy> const* input)
{
    try {

        //auto inp = *input;

        static xll::Expected<xll::String, xll::Error> result;

        // Pure monadic pipeline using pipe operator and free functions
        // Lazy materialization handles all xltype mismatches!
        result = *input
            | xll::and_then([](const xll::String& str) { return validate_string_format(str); })
            | xll::or_else([](const xll::String& error) -> xll::Expected<xll::String, xll::String> {
                std::cout << "Validation failed: " << std::string(error) << std::endl;
                return xll::Unexpected(error); })
            | xll::transform_error([](const xll::String&) { return xll::ErrValue;});

        //return result | xll::AutoFree();
        return &result;
    }
    catch (const std::exception& ex) {
        // Handle any unexpected exceptions
        std::cout << "Exception in XLL.EXPECTED: " << ex.what() << std::endl;
        auto error = xll::Expected<xll::String, xll::Error>(xll::Unexpected(xll::ErrValue));
        return error | xll::AutoFree();
    }
}


