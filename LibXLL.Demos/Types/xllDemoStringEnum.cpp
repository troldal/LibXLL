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
// Because Excel passes arguments as raw XLOPER12 pointers, the StringEnum
// parameter may hold any string the user typed.  We check valid() before
// inspecting the value, and return xll::Error(xlerrValue) on bad input.

#include <Auto.hpp>
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
    | xll::Parameter<Direction>("Direction",
        "Cardinal direction: \"North\", \"South\", \"East\", or \"West\"")
    | xll::Category("StringEnum Examples")
    | xll::Description(
        "Returns the opposite cardinal direction. "
        "Returns #VALUE! if the argument is not a recognised direction.");
XLL_REGISTER(directionOppositeReg);

XLL_FUNCTION xll::Expected<xll::String>* XLLAPI DirectionOpposite(const Direction* dir)
{
    static xll::Expected<xll::String> result;

    if (!dir->valid()) {
        std::cerr << "[DIRECTION.OPPOSITE] Invalid input: \"" << *dir << "\"\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    xll::String opposite;
    // if      (dir->is<"North">()) opposite = xll::String("South");
    // else if (dir->is<"South">()) opposite = xll::String("North");
    // else if (dir->is<"East">())  opposite = xll::String("West");
    // else                            opposite = xll::String("East");

    switch (dir->index()) {
        case Direction::IndexOf<"North">(): opposite = xll::String("South"); break;
        case Direction::IndexOf<"South">(): opposite = xll::String("North"); break;
        case Direction::IndexOf<"East">():  opposite = xll::String("West");  break;
        case Direction::IndexOf<"West">():  opposite = xll::String("East");  break;
        default: break;
    }

    result = xll::Expected<xll::String>(opposite);
    return &result;
}

// ============================================================================
// Function 2: TRAFFIC.NEXT
// ============================================================================

auto trafficNextReg =
    xll::Function("TRAFFIC.NEXT")
    | xll::Result<xll::Expected<xll::String>>()
    | xll::Procedure("TrafficNext")
    | xll::Parameter<Light>("State",
        "Traffic-light state: \"Red\", \"Amber\", or \"Green\"")
    | xll::Category("StringEnum Examples")
    | xll::Description(
        "Returns the next traffic-light state (Red→Amber→Green→Red). "
        "Returns #VALUE! if the argument is not a recognised state.");
XLL_REGISTER(trafficNextReg);

XLL_FUNCTION xll::Expected<xll::String>* XLLAPI TrafficNext(const Light* light)
{
    static xll::Expected<xll::String> result;

    if (!light->valid()) {
        std::cerr << "[TRAFFIC.NEXT] Invalid input: \"" << *light << "\"\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    xll::String next;
    if      (light->is<"Red">())   next = xll::String("Amber");
    else if (light->is<"Amber">()) next = xll::String("Green");
    else                            next = xll::String("Red");

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
    | xll::Parameter<Direction>("Direction",
        "Cardinal direction: \"North\", \"South\", \"East\", or \"West\"")
    | xll::Category("StringEnum Examples")
    | xll::Description(
        "Returns the zero-based index of the direction "
        "(0=North, 1=South, 2=East, 3=West). "
        "Returns #VALUE! for unrecognised input.");
XLL_REGISTER(directionIndexReg);

XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI DirectionIndex(const Direction* dir)
{
    static xll::Expected<xll::Number> result;

    const std::size_t idx = dir->index();   // npos if unrecognised
    if (idx == Direction::npos) {
        std::cerr << "[DIRECTION.INDEX] Invalid input: \"" << *dir << "\"\n";
        result = xll::Unexpected(xll::ErrValue);
        return &result;
    }

    result = xll::Expected<xll::Number>(xll::Number(static_cast<double>(idx)));
    return &result;
}
