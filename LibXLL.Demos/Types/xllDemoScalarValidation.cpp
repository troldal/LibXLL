//
// Created by Copilot on 06/03/2026.
//
// XLL add-in demonstrating scalar type validation.
//
// Exports three functions that take scalar parameters (Number, Bool, String).
// When called with the correct xltype, they work normally.
// When called with a mismatched xltype (e.g. a String XLOPER12 passed to a
// Number parameter), the ensure() inside the scalar type's operations throws,
// demonstrating the runtime type-safety guarantee of the basic/scalar types.

#include <Auto.hpp>
#include <Register.hpp>
#include <Types.hpp>

#include <format>

using namespace xll::literals;

// ============================================================================
// Add-in name
// ============================================================================

xll::AddInManagerInfo dllName([] { return xll::String("Scalar Validation Demo"); });

// ============================================================================
// xlAuto callbacks (minimal)
// ============================================================================

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { std::cerr << "[scalar_validation.xll] xlAutoOpen\n"; })
    | xll::OnError([](const std::string& err) { std::cerr << "  ERROR: " << err << "\n"; });
XLL_REGISTER(onOpen);

auto onFree =
    xll::OnFree()
    | xll::OnError([](const std::string& err) { std::cerr << "  ERROR: " << err << "\n"; });
XLL_REGISTER(onFree);

// ============================================================================
// Function 1: AddNumbers
// ============================================================================
// Takes two xll::Number parameters and returns their sum.
// If the caller passes an XLOPER12 whose xltype != xltypeNum,
// the ensure() inside operator+ will throw.

auto addNumbersReg =
    xll::Function("ADD.NUMBERS")
    | xll::Result<xll::Number>()
    | xll::Procedure("AddNumbers")
    | xll::Parameter<xll::Any>("A", "First number")
    | xll::Parameter<xll::Any>("B", "Second number")
    | xll::ThreadSafe()
    | xll::Category("Scalar Validation")
    | xll::Description("Adds two numbers. Throws if arguments are not xltypeNum.");
XLL_REGISTER(addNumbersReg);

XLL_FUNCTION xll::Number* XLLAPI AddNumbers(
    xll::Number const* a,
    xll::Number const* b)
{
    // Both a and b are pointers to XLOPER12-layout objects.
    // If the caller filled them with a correct xltypeNum, this works.
    // If the xltype is wrong (e.g. xltypeStr), operator+ calls
    // ensure(lhs.is_valid()) which throws std::runtime_error.
    auto result = *a + *b;
    return xll::AutoFree()(result);
}

// ============================================================================
// Function 2: NegateBoolean
// ============================================================================
// Takes an xll::Bool parameter and returns its negation.

auto negateBoolReg =
    xll::Function("NEGATE.BOOL")
    | xll::Result<xll::Number>()
    | xll::Procedure("NegateBool")
    | xll::Parameter<xll::Any>("Value", "Boolean value to negate")
    | xll::ThreadSafe()
    | xll::Category("Scalar Validation")
    | xll::Description("Negates a boolean. Throws if argument is not xltypeBool.");
XLL_REGISTER(negateBoolReg);

XLL_FUNCTION xll::Bool* XLLAPI NegateBool(
    xll::Bool const* val)
{
    // operator bool() calls ensure(is_valid()) — throws if xltype != xltypeBool
    auto result = xll::Bool(!static_cast<bool>(*val));
    return xll::AutoFree()(result);
}

// ============================================================================
// Function 3: StringLength
// ============================================================================
// Takes an xll::String parameter and returns its length as xll::Number.

auto strLenReg =
    xll::Function("STRING.LENGTH")
    | xll::Result<xll::Number>()
    | xll::Procedure("StringLength")
    | xll::Parameter<xll::Any>("Text", "String to measure")
    | xll::ThreadSafe()
    | xll::Category("Scalar Validation")
    | xll::Description("Returns string length. Throws if argument is not xltypeStr.");
XLL_REGISTER(strLenReg);

XLL_FUNCTION xll::Number* XLLAPI StringLength(
    xll::String const* text)
{
    // size() calls ensure(is_valid()) — throws if xltype != xltypeStr
    auto result = xll::Number(static_cast<double>(text->size()));
    return xll::AutoFree()(result);
}






