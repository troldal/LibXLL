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
#include <xlCommands/Alert.hpp>

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

XLL_FUNCTION xll::Any* AddNumbers(const xll::Any* a, const xll::Any* b)
{
    auto na = xll::cast<xll::Number>(*a);
    auto nb = xll::cast<xll::Number>(*b);
    return xll::AutoFree()(std::make_unique<xll::Any>(*na + *nb));
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

XLL_FUNCTION xll::Any* XLLAPI NegateBool(
    const xll::Any* val)
{
    // operator bool() calls ensure(is_valid()) — throws if xltype != xltypeBool
    auto result = xll::Bool(!static_cast<bool>(*xll::cast<xll::Bool>(*val)));
    return xll::AutoFree()(xll::Any{result});
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

XLL_FUNCTION xll::Any* XLLAPI StringLength(
    const xll::Any* text)
{
    auto str = xll::cast<xll::String>(*text);
    if (!str)
        return xll::AutoFree()(xll::Any{xll::ErrValue});
    auto result = xll::Number(static_cast<double>(str->size()));
    return xll::AutoFree()(xll::Any{result});
}

// ============================================================================
// Function 4: ShowAlert
// ============================================================================
// Takes an xll::String message and an xll::Int type, then calls xll::alert().
// Validates both argument types upfront via ensure() so that wrong-typed
// XLOPER12 arguments produce a runtime_error, consistent with the other demo
// functions.

auto showAlertReg =
    xll::Function("SHOW.ALERT")
    | xll::Result<xll::Bool>()
    | xll::Procedure("ShowAlert")
    | xll::Parameter<xll::Any>("Message", "Message text")
    | xll::Parameter<xll::Any>("Type", "0=None 1=Question 2=Information 3=Error")
    | xll::Category("Scalar Validation")
    | xll::Description("Shows an Excel alert. Throws if Message is not xltypeStr or Type is not xltypeInt.");
XLL_REGISTER(showAlertReg);

XLL_FUNCTION xll::Any* XLLAPI ShowAlert(
    const xll::Any* message,
    const xll::Any* type)
{
    auto msg  = xll::cast<xll::String>(*message);
    auto atype = xll::cast<xll::Int>(*type);
    if (!msg || !atype)
        return xll::AutoFree()(xll::Any{xll::ErrValue});
    xll::alert(*msg, static_cast<xll::Alert::Type>(static_cast<int>(*atype)));
    return xll::AutoFree()(xll::Any{xll::Bool(true)});
}






