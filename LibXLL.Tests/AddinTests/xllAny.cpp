// xllAny.cpp
// XLL add-in used by the xllAnyTests Catch2 suite.
//
// Exports four functions that exercise xll::Any round-trips and the
// xll::cast / Optional pattern through the XLL dispatch mechanism:
//
//   ANY.ECHO           — returns a deep copy of its argument unchanged.
//   ANY.TYPE.CODE      — returns the xltype of its argument as a Number.
//   ANY.NEGATE.NUMBER  — negates a Number; returns #VALUE! for other types.
//   ANY.CONCAT         — concatenates two Strings; returns #VALUE! for other types.
//
// All exported functions use xll::Any* for every parameter and the return type,
// matching the uniform function-pointer type that MockXL dispatches through.
// Using a more specific type (e.g. xll::Number*) would be binary-compatible but
// would cause Undefined Behaviour Sanitizer to warn on every call.

#include <Auto.hpp>
#include <Register.hpp>
#include <Types.hpp>

xll::AddInManagerInfo dllName([] { return xll::String("Any Test Addin"); });

auto onOpen =
    xll::OnOpen()
    | xll::OnError([](const std::string& err) { std::cerr << "  ERROR: " << err << "\n"; });
XLL_REGISTER(onOpen);

auto onFree =
    xll::OnFree()
    | xll::OnError([](const std::string& err) { std::cerr << "  ERROR: " << err << "\n"; });
XLL_REGISTER(onFree);

// ============================================================================
// ANY.ECHO
// ============================================================================

auto echoReg =
    xll::Function("ANY.ECHO")
    | xll::Result<xll::Any>()
    | xll::Procedure("AnyEcho")
    | xll::Parameter<xll::Any>("Value", "Value to echo back")
    | xll::ThreadSafe()
    | xll::Category("Any Tests")
    | xll::Description("Returns a deep copy of its argument unchanged.");
XLL_REGISTER(echoReg);

XLL_FUNCTION xll::Any* XLLAPI AnyEcho(const xll::Any* value)
{
    return xll::AutoFree()(xll::Any{ *value });
}

// ============================================================================
// ANY.TYPE.CODE
// ============================================================================

auto typeCodeReg =
    xll::Function("ANY.TYPE.CODE")
    | xll::Result<xll::Number>()
    | xll::Procedure("AnyTypeCode")
    | xll::Parameter<xll::Any>("Value", "Value whose xltype code to return")
    | xll::ThreadSafe()
    | xll::Category("Any Tests")
    | xll::Description("Returns the xltype code of its argument as a Number.");
XLL_REGISTER(typeCodeReg);

XLL_FUNCTION xll::Any* XLLAPI AnyTypeCode(const xll::Any* value)
{
    return xll::AutoFree()(xll::Any{ xll::Number(static_cast<double>(value->type())) });
}

// ============================================================================
// ANY.NEGATE.NUMBER
// ============================================================================

auto negateNumberReg =
    xll::Function("ANY.NEGATE.NUMBER")
    | xll::Result<xll::Any>()
    | xll::Procedure("AnyNegateNumber")
    | xll::Parameter<xll::Any>("Value", "Number to negate")
    | xll::ThreadSafe()
    | xll::Category("Any Tests")
    | xll::Description("Negates a Number. Returns #VALUE! if the argument is not xltypeNum.");
XLL_REGISTER(negateNumberReg);

XLL_FUNCTION xll::Any* XLLAPI AnyNegateNumber(const xll::Any* value)
{
    auto n = xll::cast<xll::Number>(*value);
    if (!n)
        return xll::AutoFree()(xll::Any{ xll::ErrValue });
    return xll::AutoFree()(xll::Any{ xll::Number(-static_cast<double>(*n)) });
}

// ============================================================================
// ANY.CONCAT
// ============================================================================

auto concatReg =
    xll::Function("ANY.CONCAT")
    | xll::Result<xll::Any>()
    | xll::Procedure("AnyConcat")
    | xll::Parameter<xll::Any>("Left",  "First string")
    | xll::Parameter<xll::Any>("Right", "Second string")
    | xll::ThreadSafe()
    | xll::Category("Any Tests")
    | xll::Description("Concatenates two Strings. Returns #VALUE! if either argument is not xltypeStr.");
XLL_REGISTER(concatReg);

XLL_FUNCTION xll::Any* XLLAPI AnyConcat(const xll::Any* left, const xll::Any* right)
{
    auto lhs = xll::cast<xll::String>(*left);
    auto rhs = xll::cast<xll::String>(*right);
    if (!lhs || !rhs)
        return xll::AutoFree()(xll::Any{ xll::ErrValue });
    return xll::AutoFree()(xll::Any{ *lhs + *rhs });
}

