// xllDemoTuple.cpp
//
// Excel add-in demonstrating xll::Tuple — a heterogeneous, compile-time-typed
// tuple backed by an xltypeMulti XLOPER12 array.
//
// Exported functions
// ------------------
//   EMPLOYEE.PACK      – takes three Any arguments (name, salary, active flag),
//                        casts each to the declared element type, and packs
//                        them into a Tuple<String, Number, Bool> returned as
//                        a 1×3 array.  Returns #VALUE! if any argument has the
//                        wrong type.
//
//   EMPLOYEE.SALARY    – takes a 1×3 array (e.g. the output of EMPLOYEE.PACK),
//                        casts it to Tuple<String, Number, Bool>, and returns
//                        the salary (element 1).  Returns #VALUE! if the array
//                        has the wrong shape or the salary slot is not a number.
//
//   EMPLOYEE.SUMMARY   – takes a 1×3 array, casts it to
//                        Tuple<String, Number, Bool>, and returns a formatted
//                        string "Name | Salary | active/inactive".
//                        Returns #VALUE! on any mismatch.
//
//   POINT.DISTANCE     – takes two 1×2 arrays, each cast to Tuple<Number,Number>
//                        representing (x, y) coordinates, and returns the
//                        Euclidean distance between them.
//                        Demonstrates using multiple Tuple arguments.
//
// Two-level validation
// --------------------
// xll::cast<T>(any) performs full validation in one step:
//
//   1. xltype == xltypeMulti
//   2. element count == sizeof...(Ts)
//   3. every element at position I has an xltype compatible with Ts[I]
//
// If any check fails, cast returns xll::None.  A successful cast therefore
// guarantees that every xll::get call on the result will succeed without
// throwing, so no try/catch is needed around get calls on a cast result.

#include <Auto.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>
#include <Types/Tuple.hpp>

#include <cmath>
#include <iostream>
#include <string>

using namespace xll::literals;

// ============================================================================
// Type aliases
// ============================================================================

// An employee row: name, salary, active flag.
using EmployeeRow = xll::Tuple<xll::String, xll::Number, xll::Bool>;

// A 2-D point: x, y.
using Point2D = xll::Tuple<xll::Number, xll::Number>;

// ============================================================================
// Add-in lifecycle
// ============================================================================

xll::AddInManagerInfo dllName([] { return xll::String("Tuple Demo"); });

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { std::cerr << "[Tuple Demo] xlAutoOpen\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] { std::cerr << "[Tuple Demo] xlAutoClose\n"; });
XLL_REGISTER(onClose);

// ============================================================================
// Function 1: EMPLOYEE.PACK
//
// Takes three separate Any arguments and packs them into a Tuple.
// Each argument is cast to its declared type; the function returns #VALUE!
// if any argument has the wrong type.
//
// Usage:  =EMPLOYEE.PACK("Alice", 50000, TRUE)
//         → 1×3 array { "Alice", 50000, TRUE }
// ============================================================================

auto employeePackReg =
    xll::Function("EMPLOYEE.PACK")
    | xll::Result<xll::Expected<xll::Array<xll::Any>>>()
    | xll::Procedure("EmployeePack")
    | xll::Parameter<xll::Any>("Name",   "Employee name (string)")
    | xll::Parameter<xll::Any>("Salary", "Annual salary (number)")
    | xll::Parameter<xll::Any>("Active", "Whether the employee is active (TRUE/FALSE)")
    | xll::Category("Tuple Examples")
    | xll::Description(
        "Packs Name, Salary, and Active into a 1x3 array (EmployeeRow tuple). "
        "Returns #VALUE! if any argument has the wrong type.");
XLL_REGISTER(employeePackReg);

XLL_FUNCTION xll::Expected<xll::Array<xll::Any>>* XLLAPI
EmployeePack(const xll::Any* nameArg, const xll::Any* salaryArg, const xll::Any* activeArg)
{
    static xll::Expected<xll::Array<xll::Any>> result;

    const auto name   = xll::cast<xll::String>(*nameArg);
    const auto salary = xll::cast<xll::Number>(*salaryArg);
    const auto active = xll::cast<xll::Bool>(*activeArg);

    if (!name || !salary || !active) {
        std::cerr << "[EMPLOYEE.PACK] Wrong argument type(s)\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    // Build the EmployeeRow and return it as an Array<Any> (the tuple IS an
    // xltypeMulti array, so it is directly usable as the return value).

    result = xll::Array<xll::Any> { *name, *salary, *active };
    return &result;
}

// ============================================================================
// Function 2: EMPLOYEE.SALARY
//
// Takes a 1×3 array (e.g. from EMPLOYEE.PACK or a named range) and returns
// the salary.  The cast to EmployeeRow validates that the array has exactly
// 3 elements; xll::get<1> validates that element 1 is a Number.
//
// Usage:  =EMPLOYEE.SALARY(A1:C1)
//         → number (the salary), or #VALUE! on bad input
// ============================================================================

auto employeeSalaryReg =
    xll::Function("EMPLOYEE.SALARY")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("EmployeeSalary")
    | xll::Parameter<xll::Any>("Row",
        "A 1x3 array with columns: Name (string), Salary (number), Active (bool)")
    | xll::Category("Tuple Examples")
    | xll::Description(
        "Returns the salary from a 1x3 employee-row array. "
        "Returns #VALUE! if the array has the wrong shape or the salary element is not a number.");
XLL_REGISTER(employeeSalaryReg);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI EmployeeSalary(const xll::Any* rowArg)
{
    static xll::Expected<xll::Number> result;

    // cast validates structure AND element types.  None means the input is
    // not a valid 3-element array with String/Number/Bool in the right slots.
    const auto row = xll::cast<EmployeeRow>(*rowArg);
    if (!row) {
        std::cerr << "[EMPLOYEE.SALARY] Input is not a valid EmployeeRow\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    result = xll::get<1>(*row);
    return &result;

    return &result;
}

// ============================================================================
// Function 3: EMPLOYEE.SUMMARY
//
// Takes a 1×3 array and returns a formatted string:
//   "Alice | 50000.00 | active"
//
// Usage:  =EMPLOYEE.SUMMARY(A1:C1)
// ============================================================================

auto employeeSummaryReg =
    xll::Function("EMPLOYEE.SUMMARY")
    | xll::Result<xll::Expected<xll::String>>()
    | xll::Procedure("EmployeeSummary")
    | xll::Parameter<xll::Any>("Row",
        "A 1x3 array with columns: Name (string), Salary (number), Active (bool)")
    | xll::Category("Tuple Examples")
    | xll::Description(
        "Returns a formatted summary string for an employee row, e.g. \"Alice | 50000.00 | active\". "
        "Returns #VALUE! if the array has the wrong shape or element types.");
XLL_REGISTER(employeeSummaryReg);

XLL_FUNCTION xll::Expected<xll::String>* XLLAPI EmployeeSummary(const xll::Any* rowArg)
{
    static xll::Expected<xll::String> result;

    // cast validates structure AND element types.
    const auto row = xll::cast<EmployeeRow>(*rowArg);
    if (!row) {
        std::cerr << "[EMPLOYEE.SUMMARY] Input is not a valid EmployeeRow\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    const auto name   = xll::get<xll::String>(*row);
    const auto salary = xll::get<xll::Number>(*row);
    const auto   active = xll::get<xll::Bool>(*row);

    const std::string status = static_cast<bool>(active) ? "active" : "inactive";

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(salary));

    result = xll::String(std::string(name) + " | " + buf + " | " + status);
    return &result;
}

// ============================================================================
// Function 4: POINT.DISTANCE
//
// Takes two 1×2 arrays (x, y coordinates), casts each to Point2D, and
// returns the Euclidean distance.  Demonstrates multiple Tuple arguments.
//
// Usage:  =POINT.DISTANCE(A1:B1, A2:B2)
//         where A1:B1 = { 0, 0 } and A2:B2 = { 3, 4 }  →  5
// ============================================================================

auto pointDistanceReg =
    xll::Function("POINT.DISTANCE")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("PointDistance")
    | xll::Parameter<xll::Any>("Point1", "First point as a 1x2 array { x, y }")
    | xll::Parameter<xll::Any>("Point2", "Second point as a 1x2 array { x, y }")
    | xll::Category("Tuple Examples")
    | xll::Description(
        "Returns the Euclidean distance between two points, each given as a 1x2 array { x, y }. "
        "Returns #VALUE! if either argument is not a valid 2-element numeric array.");
XLL_REGISTER(pointDistanceReg);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI
PointDistance(const xll::Any* p1Arg, const xll::Any* p2Arg)
{
    static xll::Expected<xll::Number> result;

    // cast validates structure AND element types.  None if either argument is
    // not a 2-element array whose both elements are Numbers.
    const auto p1 = xll::cast<Point2D>(*p1Arg);
    const auto p2 = xll::cast<Point2D>(*p2Arg);

    if (!p1 || !p2) {
        std::cerr << "[POINT.DISTANCE] One or both arguments are not valid Point2D arrays\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    const double x1 = static_cast<double>(xll::get<0>(*p1));
    const double y1 = static_cast<double>(xll::get<1>(*p1));
    const double x2 = static_cast<double>(xll::get<0>(*p2));
    const double y2 = static_cast<double>(xll::get<1>(*p2));

    const double dx = x2 - x1;
    const double dy = y2 - y1;
    result = xll::Expected<xll::Number>(xll::Number(std::sqrt(dx * dx + dy * dy)));
    return &result;
}

