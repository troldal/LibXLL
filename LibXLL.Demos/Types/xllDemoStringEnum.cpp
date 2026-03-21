// xllDemoStringEnum.cpp
//
// Excel add-in demonstrating xll::StringEnum.
//
// Exported functions
// ------------------
//   DIRECTION.OPPOSITE   – takes a cardinal direction ("North", "South",
//                          "East", "West") and returns the opposite direction.
//                          Returns #VALUE! and logs to stderr if the string
//                          is not one of the four allowed values.
//
//   TRAFFIC.NEXT         – takes a traffic-light state ("Red", "Amber",
//                          "Green") and returns the next state in the cycle.
//                          Returns #VALUE! and logs to stderr on bad input.
//
//   DIRECTION.INDEX      – returns the zero-based index of the direction.
//                          Returns #VALUE! and logs to stderr on bad input.
//
// Each function receives an xll::Any argument (the raw XLOPER12 value Excel
// passes in) and uses xll::cast<StringEnum>(any) to validate it.  The cast
// returns xll::None when the argument is not an xltypeStr holding a
// recognised element, so no separate valid() check is needed.  All query
// operations (index(), is<>(), visit()) are called only on the engaged
// Optional value and are therefore guaranteed to succeed.

#include <Auto.hpp>
#include <Commands.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>
#include <Types/StringEnum.hpp>

#include <iostream>

using namespace xll::literals;

// ============================================================================
// Type aliases
// ============================================================================

using Direction = xll::StringEnum<"North", "South", "East", "West">;
using Light     = xll::StringEnum<"Red", "Amber", "Green">;

// ============================================================================
// Add-in lifecycle
// ============================================================================

xll::AddInManagerInfo dllName([] { return xll::String("StringEnum Demo"); });

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { std::cerr << "[StringEnum Demo] xlAutoOpen\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] { std::cerr << "[StringEnum Demo] xlAutoClose\n"; });
XLL_REGISTER(onClose);

// ============================================================================
// Function 1: DIRECTION.OPPOSITE
// ============================================================================

auto directionOppositeReg =
    xll::Function("DIRECTION.OPPOSITE")
    | xll::Result<xll::Expected<xll::String>>()
    | xll::Procedure("DirectionOpposite")
    | xll::Parameter<xll::Any>("Direction",
        "Cardinal direction: \"North\", \"South\", \"East\", or \"West\"")
    | xll::Category("StringEnum Examples")
    | xll::Description(
        "Returns the opposite cardinal direction. "
        "Returns #VALUE! if the argument is not a recognised direction.");
XLL_REGISTER(directionOppositeReg);

XLL_FUNCTION xll::Expected<xll::String>* XLLAPI DirectionOpposite(const xll::Any* arg)
{
    const auto dir = xll::cast<Direction>(*arg);
    if (!dir) {
        std::cerr << "[DIRECTION.OPPOSITE] Invalid input\n";
        return xll::AutoFree()(
            std::make_unique<xll::Expected<xll::String>>(xll::Unexpected(xll::ErrValue)));
    }

    xll::String opposite;
    switch (dir->index()) {
        case Direction::IndexOf<"North">(): opposite = xll::String("South"); break;
        case Direction::IndexOf<"South">(): opposite = xll::String("North"); break;
        case Direction::IndexOf<"East">():  opposite = xll::String("West");  break;
        case Direction::IndexOf<"West">():  opposite = xll::String("East");  break;
        default: break;
    }

    return xll::AutoFree()(std::make_unique<xll::Expected<xll::String>>(opposite));
}

// ============================================================================
// Function 2: TRAFFIC.NEXT
// ============================================================================

auto trafficNextReg =
    xll::Function("TRAFFIC.NEXT")
    | xll::Result<xll::Expected<xll::String>>()
    | xll::Procedure("TrafficNext")
    | xll::Parameter<xll::Any>("State",
        "Traffic-light state: \"Red\", \"Amber\", or \"Green\"")
    | xll::Category("StringEnum Examples")
    | xll::Description(
        "Returns the next traffic-light state (Red→Amber→Green→Red). "
        "Returns #VALUE! if the argument is not a recognised state.");
XLL_REGISTER(trafficNextReg);

XLL_FUNCTION xll::Expected<xll::String>* XLLAPI TrafficNext(const xll::Any* arg)
{
    static xll::Expected<xll::String> result;

    const auto light = xll::cast<Light>(*arg);
    if (!light) {
        std::cerr << "[TRAFFIC.NEXT] Invalid input\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    xll::String next;
    if      (light->is<"Red">())   next = xll::String("Amber");
    else if (light->is<"Amber">()) next = xll::String("Green");
    else                           next = xll::String("Red");

    result = xll::Expected<xll::String>(next);
    return &result;
}

// ============================================================================
// Function 3: DIRECTION.INDEX
// ============================================================================

auto directionIndexReg =
    xll::Function("DIRECTION.INDEX")
    | xll::Result<xll::Expected<xll::Number>>()
    | xll::Procedure("DirectionIndex")
    | xll::Parameter<xll::Any>("Direction",
        "Cardinal direction: \"North\", \"South\", \"East\", or \"West\"")
    | xll::Category("StringEnum Examples")
    | xll::Description(
        "Returns the zero-based index of the direction "
        "(0=North, 1=South, 2=East, 3=West). "
        "Returns #VALUE! for unrecognised input.");
XLL_REGISTER(directionIndexReg);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI DirectionIndex(const xll::Any* arg)
{
    static xll::Expected<xll::Number> result;

    const auto dir = xll::cast<Direction>(*arg);
    if (!dir) {
        std::cerr << "[DIRECTION.INDEX] Invalid input\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    result = xll::Expected<xll::Number>(xll::Number(static_cast<double>(dir->index())));
    return &result;
}

auto inverseBool =
    xll::Function("PRINT.ARRAY")
        .Result<xll::MatrixBuffer>()
        .Procedure("PrintArray")
        .Parameter<xll::MatrixBuffer>("Boolean", "The boolean to be inversed")
        .Category("StringEnum Examples")
        .Hidden()
        .Description(
            "Returns the zero-based index of the direction "
            "(0=North, 1=South, 2=East, 3=West). "
            "Returns #VALUE! for unrecognised input.");
XLL_REGISTER(inverseBool);

XLL_FUNCTION const xll::MatrixBuffer* XLLAPI PrintArray(const xll::MatrixBuffer* arg)
{
    thread_local xll::Matrix res;
    res = *arg;
    std::ranges::transform(res, res.begin(), [](double x) { return x * 2.0; });

    auto caller = xll::caller();

    return res.get();
}

// ============================================================================
// Command: HELLO.CONSOLE
// ============================================================================

auto helloConsoleCmd =
    xll::Command("HELLO.CONSOLE")
    | xll::Procedure("HelloConsole")
    | xll::ShortcutKey("H")
    | xll::Category("StringEnum Examples")
    | xll::Help("https://kinetiq.dev")
    | xll::Description("Writes a greeting message to the console.");
XLL_REGISTER(helloConsoleCmd);

XLL_FUNCTION void XLLAPI HelloConsole()
{
    //std::cerr << "[HELLO.CONSOLE] Hello from the StringEnum demo!\n";
    //xll::alert("Hello from the StringEnum demo!", xll::Alert::Question);

    // xll::Array<xll::Any> arr{
    //     xll::Nil{}, xll::Nil{}, xll::Nil{}, xll::Number{372}, xll::Number{200}, "Logon"_xs, xll::Nil{},
    //     xll::Number{1}, xll::Number{50}, xll::Number{170}, xll::Number{90}, xll::Nil{}, "OK"_xs, xll::Nil{},
    //     xll::Number{2}, xll::Number{150}, xll::Number{170}, xll::Number{90}, xll::Nil{}, "Cancel"_xs, xll::Nil{},
    //     xll::Number{24}, xll::Number{250}, xll::Number{170}, xll::Number{90}, xll::Nil{}, "Help"_xs, "https://kinetiq.dev!0"_xs,
    //     xll::Number{5}, xll::Number{40}, xll::Number{10}, xll::Nil{}, xll::Nil{}, "Please enter your username and password"_xs, xll::Nil{},
    //     xll::Number{14}, xll::Number{40}, xll::Number{35}, xll::Number{290}, xll::Number{100}, xll::Nil{}, xll::Nil{},
    //     xll::Number{5}, xll::Number{50}, xll::Number{53}, xll::Nil{}, xll::Nil{}, "Username"_xs, xll::Nil{},
    //     xll::Number{6}, xll::Number{150}, xll::Number{50}, xll::Nil{}, xll::Nil{}, xll::Nil{}, "MyName"_xs,
    //     xll::Number{5}, xll::Number{50}, xll::Number{73}, xll::Nil{}, xll::Nil{}, "Password"_xs, xll::Nil{},
    //     xll::Number{6}, xll::Number{150}, xll::Number{70}, xll::Nil{}, xll::Nil{}, xll::Nil{}, "**********"_xs,
    //     xll::Number{13}, xll::Number{50}, xll::Number{110}, xll::Nil{}, xll::Nil{}, "Remember username and password"_xs, xll::Bool{true}
    // };
    // arr.val.array.columns = 7;
    // arr.val.array.rows = 11;
    //
    // xll::Array<xll::Any> res;
    // Excel12(xlfDialogBox, &res, 1, &arr);

    auto result = xll::dialog::Dialog("Logon")
        .Size(372, 200)
        .Add(xll::dialog::OkButton("OK").At(50, 140).Size(90, 10))
        .Add(xll::dialog::TextBox("MyName").At(150, 50))
        .show();

    auto hwnd = xll::get_hwnd();
    std::cout << "Excel window handle: " << hwnd << std::endl;
}

