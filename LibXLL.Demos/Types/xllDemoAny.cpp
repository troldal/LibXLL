// xllDemoAny.cpp
//
// Excel add-in demonstrating xll::Any — a type-erased container that accepts
// any XLOPER12 value.  The functions below show the three key use-cases:
//
//   ANY.DESCRIBE   — inspect whatever the caller passes and return a string
//                    describing the type and value.
//   ANY.TO.NUMBER  — coerce the argument to a number if possible, otherwise
//                    return #VALUE!.
//   ANY.ROUNDTRIP  — accept any single value and return it unchanged, showing
//                    that xll::Any preserves the original XLOPER12 payload.

#include <Auto.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>
#include <Types/Any.hpp>

#include <cmath>
#include <string>

using namespace xll::literals;

// ============================================================================
// Helper: format an xll::Any as a human-readable string
// ============================================================================

static xll::String describe(const xll::Any& any)
{
    // Try each concrete type in turn via xll::cast (returns Optional<T>).

    if (auto n = xll::cast<xll::Number>(any); n.has_value())
        return xll::String("Number: " + std::to_string(static_cast<double>(n.value())));

    if (auto s = xll::cast<xll::String>(any); s.has_value())
        return xll::String("String: " + std::string(s.value()));

    if (auto i = xll::cast<xll::Int>(any); i.has_value())
        return xll::String("Int: " + std::to_string(static_cast<int>(i.value())));

    if (auto b = xll::cast<xll::Bool>(any); b.has_value())
        return xll::String(std::string("Bool: ") + (static_cast<bool>(b.value()) ? "TRUE" : "FALSE"));

    if (auto e = xll::cast<xll::Error>(any); e.has_value())
        return xll::String("Error: " + std::string(e.value().to_string()));

    if (xll::holds<xll::Missing>(any))
        return xll::String("(missing argument)");

    if (xll::holds<xll::Array>(any))
        return xll::String("Array");

    return xll::String("(empty/nil)");
}

// ============================================================================
// Function 1: ANY.DESCRIBE
// Returns a string describing the type and value of its argument.
// ============================================================================

auto anyDescribeReg =
    xll::Function("ANY.DESCRIBE")
    | xll::Result<xll::String>()
    | xll::Procedure("AnyDescribe")
    | xll::Parameter<xll::Any>("Value", "Any Excel value")
    | xll::Category("Any Examples")
    | xll::Description("Returns a text description of the type and value of the argument");
XLL_REGISTER(anyDescribeReg);

XLL_FUNCTION xll::String* XLLAPI AnyDescribe(const xll::Any* value)
{
    static xll::String result;
    result = describe(*value);
    return &result;
}

// ============================================================================
// Function 2: ANY.TO.NUMBER
// Coerces the argument to a Number if possible; returns #VALUE! otherwise.
// Accepts Number (direct), Int (widened), Bool (0/1), and numeric String.
// ============================================================================

auto anyToNumberReg =
    xll::Function("ANY.TO.NUMBER")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("AnyToNumber")
    | xll::Parameter<xll::Any>("Value", "Any Excel value to coerce to a number")
    | xll::Category("Any Examples")
    | xll::Description("Coerces any value to a number. Returns #VALUE! if conversion is not possible.");
XLL_REGISTER(anyToNumberReg);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI AnyToNumber(const xll::Any* value)
{
    using Result = xll::Expected<xll::Number>;
    static Result result;

    // Number — direct
    if (auto n = xll::cast<xll::Number>(*value); n.has_value()) {
        result = Result(n.value());
        return &result;
    }

    // Int — widen to double
    if (auto i = xll::cast<xll::Int>(*value); i.has_value()) {
        result = Result(xll::Number(static_cast<double>(static_cast<int>(i.value()))));
        return &result;
    }

    // Bool — 0.0 or 1.0
    if (auto b = xll::cast<xll::Bool>(*value); b.has_value()) {
        result = Result(xll::Number(static_cast<bool>(b.value()) ? 1.0 : 0.0));
        return &result;
    }

    // String — attempt stod
    if (auto s = xll::cast<xll::String>(*value); s.has_value()) {
        try {
            const double d = std::stod(std::string(s.value()));
            result = Result(xll::Number(d));
            return &result;
        }
        catch (...) { /* fall through to #VALUE! */ }
    }

    result = Result(xll::Unexpected(xll::ErrValue));
    return &result;
}

// ============================================================================
// Function 3: ANY.ROUNDTRIP
// Accepts any single value and returns it unchanged.
// Demonstrates that xll::Any transparently preserves the XLOPER12 payload.
// ============================================================================

auto anyRoundtripReg =
    xll::Function("ANY.ROUNDTRIP")
    | xll::Result<xll::Any>()
    | xll::Procedure("AnyRoundtrip")
    | xll::Parameter<xll::Any>("Value", "Any Excel value")
    | xll::Category("Any Examples")
    | xll::Description("Returns its argument unchanged — demonstrates xll::Any round-trip fidelity");
XLL_REGISTER(anyRoundtripReg);

XLL_FUNCTION xll::Any* XLLAPI AnyRoundtrip(const xll::Any* value)
{
    static xll::Any result;
    result = *value;
    return &result;
}

