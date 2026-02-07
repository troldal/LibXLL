//
// Created by Copilot on 07/02/2026.
//
// Demonstrates xll::Expected usage in Excel add-in functions

#include <Auto.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>
#include <cmath>

using namespace xll::literals;

// ============================================================================
// Example 1: Safe Division Function
// ============================================================================
// Returns #DIV/0! error when denominator is zero

auto safeDivide =
    xll::Function("SAFE.DIVIDE")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("SafeDivide")
    | xll::Parameter<xll::Expected<xll::Number>>("Numerator", "The numerator")
    | xll::Parameter<xll::Expected<xll::Number>>("Denominator", "The denominator")
    | xll::ThreadSafe()
    | xll::Category("Expected Examples")
    | xll::Description("Safely divides two numbers, returning #DIV/0! if denominator is zero");
XLL_REGISTER(safeDivide);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI SafeDivide(
    xll::Expected<xll::Number> const* numerator,
    xll::Expected<xll::Number> const* denominator)
{
    // Check if inputs are valid
    if (!numerator->has_value()) {
        return *numerator | xll::AutoFree();
    }
    if (!denominator->has_value()) {
        return *denominator | xll::AutoFree();
    }

    // Check for division by zero
    double denom_val = static_cast<double>(denominator->value());
    if (denom_val == 0.0) {
        auto err = xll::Error();
        err.val.err = 7;  // #DIV/0!
        auto result = xll::Expected<xll::Number>(xll::Unexpected(err));
        return result | xll::AutoFree();
    }

    // Perform division
    double num_val = static_cast<double>(numerator->value());
    auto result = xll::Expected<xll::Number>(xll::Number(num_val / denom_val));
    return result | xll::AutoFree();
}

// ============================================================================
// Example 2: Safe Square Root Function
// ============================================================================
// Returns #NUM! error for negative numbers

auto safeSqrt =
    xll::Function("SAFE.SQRT")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("SafeSqrt")
    | xll::Parameter<xll::Expected<xll::Number>>("Value", "The value to take square root of")
    | xll::ThreadSafe()
    | xll::Category("Expected Examples")
    | xll::Description("Safely computes square root, returning #NUM! for negative values");
XLL_REGISTER(safeSqrt);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI SafeSqrt(
    xll::Expected<xll::Number> const* value)
{
    // Check if input is valid
    if (!value->has_value()) {
        return *value | xll::AutoFree();
    }

    double val = static_cast<double>(value->value());

    // Check for negative value
    if (val < 0.0) {
        auto err = xll::Error();
        err.val.err = 36;  // #NUM!
        auto result = xll::Expected<xll::Number>(xll::Unexpected(err));
        return result | xll::AutoFree();
    }

    // Compute square root
    auto result = xll::Expected<xll::Number>(xll::Number(std::sqrt(val)));
    return result | xll::AutoFree();
}

// ============================================================================
// Example 3: Chaining Operations with transform
// ============================================================================
// Demonstrates using transform to chain operations

auto doubleIfPositive =
    xll::Function("DOUBLE.IF.POSITIVE")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("DoubleIfPositive")
    | xll::Parameter<xll::Expected<xll::Number>>("Value", "The value to double if positive")
    | xll::ThreadSafe()
    | xll::Category("Expected Examples")
    | xll::Description("Doubles the value if positive, returns #NUM! otherwise");
XLL_REGISTER(doubleIfPositive);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI DoubleIfPositive(
    xll::Expected<xll::Number> const* value)
{
    // Check if input is valid
    if (!value->has_value()) {
        return *value | xll::AutoFree();
    }

    double val = static_cast<double>(value->value());

    // Check if positive
    if (val <= 0.0) {
        auto err = xll::Error();
        err.val.err = 36;  // #NUM!
        auto result = xll::Expected<xll::Number>(xll::Unexpected(err));
        return result | xll::AutoFree();
    }

    // Double the value
    auto result = xll::Expected<xll::Number>(xll::Number(val * 2.0));
    return result | xll::AutoFree();
}

// ============================================================================
// Example 4: Array Operations with Expected
// ============================================================================
// Processes an array of values, handling errors element-wise

auto safeDivideArray =
    xll::Function("SAFE.DIVIDE.ARRAY")
    | xll::Result<xll::Array<xll::Expected<xll::Number>>>()
    | xll::Procedure("SafeDivideArray")
    | xll::Parameter<xll::Array<xll::Expected<xll::Number>>>("Numerators", "Array of numerators")
    | xll::Parameter<xll::Array<xll::Expected<xll::Number>>>("Denominators", "Array of denominators")
    | xll::ThreadSafe()
    | xll::Category("Expected Examples")
    | xll::Description("Safely divides two arrays element-wise, propagating errors");
XLL_REGISTER(safeDivideArray);

XLL_FUNCTION xll::Array<xll::Expected<xll::Number>>* XLLAPI SafeDivideArray(
    xll::Array<xll::Expected<xll::Number>> const* numerators,
    xll::Array<xll::Expected<xll::Number>> const* denominators)
{
    // Ensure arrays have the same size
    if (numerators->size() != denominators->size()) {
        auto err = xll::Error();
        err.val.err = 23;  // #REF!
        auto result = xll::Array<xll::Expected<xll::Number>>(1, 1);
        result[0] = xll::Expected<xll::Number>(xll::Unexpected(err));
        return result | xll::AutoFree();
    }

    auto result = xll::Array<xll::Expected<xll::Number>>(numerators->rows(), numerators->cols());

    for (size_t i = 0; i < numerators->size(); ++i) {
        const auto& num = (*numerators)[i];
        const auto& denom = (*denominators)[i];

        // Propagate errors from inputs
        if (!num.has_value()) {
            result[i] = num;
            continue;
        }
        if (!denom.has_value()) {
            result[i] = denom;
            continue;
        }

        // Check for division by zero
        double denom_val = static_cast<double>(denom.value());
        if (denom_val == 0.0) {
            auto err = xll::Error();
            err.val.err = 7;  // #DIV/0!
            result[i] = xll::Expected<xll::Number>(xll::Unexpected(err));
            continue;
        }

        // Perform division
        double num_val = static_cast<double>(num.value());
        result[i] = xll::Expected<xll::Number>(xll::Number(num_val / denom_val));
    }

    return result | xll::AutoFree();
}

// ============================================================================
// Example 5: Using value_or for default values
// ============================================================================

auto getValueOrDefault =
    xll::Function("VALUE.OR.DEFAULT")
    | xll::Result<xll::Number>()
    | xll::Procedure("GetValueOrDefault")
    | xll::Parameter<xll::Expected<xll::Number>>("Value", "The value to retrieve")
    | xll::Parameter<xll::Number>("Default", "Default value if error")
    | xll::ThreadSafe()
    | xll::Category("Expected Examples")
    | xll::Description("Returns the value if valid, otherwise returns the default");
XLL_REGISTER(getValueOrDefault);

XLL_FUNCTION xll::Number* XLLAPI GetValueOrDefault(
    xll::Expected<xll::Number> const* value,
    xll::Number const* default_value)
{
    xll::Number result_value = value->value_or(xll::Number(*default_value));
    auto result = std::make_unique<xll::Number>(result_value);
    result->xltype |= xlbitDLLFree;
    return result.release();
}

// ============================================================================
// Example 6: Complex chaining with and_then
// ============================================================================

auto sqrtThenInverse =
    xll::Function("SQRT.THEN.INVERSE")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("SqrtThenInverse")
    | xll::Parameter<xll::Expected<xll::Number>>("Value", "The value to process")
    | xll::ThreadSafe()
    | xll::Category("Expected Examples")
    | xll::Description("Computes 1/sqrt(value), returning errors for negative or zero values");
XLL_REGISTER(sqrtThenInverse);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI SqrtThenInverse(
    xll::Expected<xll::Number> const* value)
{
    // Check if input is valid
    if (!value->has_value()) {
        return *value | xll::AutoFree();
    }

    double val = static_cast<double>(value->value());

    // Check for negative value
    if (val < 0.0) {
        auto err = xll::Error();
        err.val.err = 36;  // #NUM!
        auto result = xll::Expected<xll::Number>(xll::Unexpected(err));
        return result | xll::AutoFree();
    }

    // Check for zero (would cause division by zero)
    if (val == 0.0) {
        auto err = xll::Error();
        err.val.err = 7;  // #DIV/0!
        auto result = xll::Expected<xll::Number>(xll::Unexpected(err));
        return result | xll::AutoFree();
    }

    // Compute sqrt then inverse
    double sqrt_val = std::sqrt(val);
    auto result = xll::Expected<xll::Number>(xll::Number(1.0 / sqrt_val));
    return result | xll::AutoFree();
}

// ============================================================================
// Add-in Information and Lifecycle Hooks
// ============================================================================

xll::AddInManagerInfo dllName([] {
    return xll::String("Expected Demo Add-In");
});

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { std::cerr << "Expected Demo Add-In loaded\n"; });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] { std::cerr << "Expected Demo Add-In unloaded\n"; });
XLL_REGISTER(onClose);






