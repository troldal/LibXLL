// =============================================================================
// Tests for xll::StringEnum
// =============================================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/StringEnum.hpp"
#include "../Types/String.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Type aliases used throughout
// ---------------------------------------------------------------------------

using Direction = xll::StringEnum<"North", "South", "East", "West">;
using Light     = xll::StringEnum<"Red", "Amber", "Green">;
using Single    = xll::StringEnum<"EXACT">;

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(Direction) == sizeof(XLOPER12));
static_assert(alignof(Direction) == alignof(XLOPER12));

static_assert(Direction::size() == 4);
static_assert(Light::size()     == 3);
static_assert(Single::size()    == 1);

static_assert(Direction::IndexOf<"North">() == 0);
static_assert(Direction::IndexOf<"South">() == 1);
static_assert(Direction::IndexOf<"East">()  == 2);
static_assert(Direction::IndexOf<"West">()  == 3);

static_assert(Direction::npos == std::numeric_limits<std::size_t>::max());

// =============================================================================
// 1. Construction
// =============================================================================

TEST_CASE("StringEnum - Default construction", "[xll::StringEnum][construction]")
{
    Direction d;
    REQUIRE(d.valid());
    REQUIRE(d.index() == 0);
    REQUIRE(d.value().has_value());
    REQUIRE(d.value().value().to_string() == "North");
}

TEST_CASE("StringEnum - Construction from xll::String", "[xll::StringEnum][construction]")
{
    SECTION("Known string")
    {
        Direction d { xll::String("East") };
        REQUIRE(d.valid());
        REQUIRE(d.index() == Direction::IndexOf<"East">());
        REQUIRE(d.value().value().to_string() == "East");
    }

    SECTION("Unknown string throws std::invalid_argument")
    {
        REQUIRE_THROWS_AS(Direction { xll::String("Up") }, std::invalid_argument);
    }

    SECTION("Move from xll::String")
    {
        xll::String s("South");
        Direction d { std::move(s) };
        REQUIRE(d.valid());
        REQUIRE(d.index() == Direction::IndexOf<"South">());
    }
}

TEST_CASE("StringEnum - Construction from string literal", "[xll::StringEnum][construction]")
{
    Direction d { "West" };
    REQUIRE(d.valid());
    REQUIRE(d.index() == Direction::IndexOf<"West">());
    REQUIRE(d.value().value().to_string() == "West");
}

TEST_CASE("StringEnum - Construction from std::string_view", "[xll::StringEnum][construction]")
{
    std::string_view sv = "South";
    Direction d { sv };
    REQUIRE(d.valid());
    REQUIRE(d.index() == Direction::IndexOf<"South">());
}

TEST_CASE("StringEnum - Construction throws on unknown string", "[xll::StringEnum][construction]")
{
    REQUIRE_THROWS_AS((Direction { "Up" }),          std::invalid_argument);
    REQUIRE_THROWS_AS((Direction { "" }),            std::invalid_argument);
    REQUIRE_THROWS_AS((Direction { "north" }),       std::invalid_argument);  // case-sensitive
    REQUIRE_THROWS_AS((Direction { "NORTH" }),       std::invalid_argument);
    REQUIRE_THROWS_AS((Direction { " North" }),      std::invalid_argument);  // leading space
}

TEST_CASE("StringEnum - Copy construction", "[xll::StringEnum][construction]")
{
    Direction a { "East" };
    Direction b { a };
    REQUIRE(b.valid());
    REQUIRE(b.index() == a.index());
    REQUIRE(b.value().value().to_string() == "East");
}

TEST_CASE("StringEnum - Move construction", "[xll::StringEnum][construction]")
{
    Direction a { "West" };
    Direction b { std::move(a) };
    REQUIRE(b.valid());
    REQUIRE(b.index() == Direction::IndexOf<"West">());
}

// =============================================================================
// 2. Assignment
// =============================================================================

TEST_CASE("StringEnum - Assignment from xll::String", "[xll::StringEnum][assignment]")
{
    Direction d;
    d = xll::String("South");
    REQUIRE(d.index() == Direction::IndexOf<"South">());

    SECTION("Unknown string leaves object unchanged and throws")
    {
        const std::size_t before = d.index();
        REQUIRE_THROWS_AS(d = xll::String("Down"), std::invalid_argument);
        REQUIRE(d.index() == before);
    }
}

TEST_CASE("StringEnum - Assignment from string_view", "[xll::StringEnum][assignment]")
{
    Direction d;
    d = std::string_view("East");
    REQUIRE(d.index() == Direction::IndexOf<"East">());
    REQUIRE_THROWS_AS(d = std::string_view("bad"), std::invalid_argument);
}

TEST_CASE("StringEnum - Assignment from string literal", "[xll::StringEnum][assignment]")
{
    Direction d;
    d = "West";
    REQUIRE(d.index() == Direction::IndexOf<"West">());
    REQUIRE_THROWS_AS(d = "nowhere", std::invalid_argument);
}

TEST_CASE("StringEnum - Copy assignment", "[xll::StringEnum][assignment]")
{
    Direction a { "East" };
    Direction b;
    b = a;
    REQUIRE(b.index() == a.index());
    REQUIRE(b.value().value().to_string() == "East");
}

TEST_CASE("StringEnum - Move assignment", "[xll::StringEnum][assignment]")
{
    Direction a { "West" };
    Direction b;
    b = std::move(a);
    REQUIRE(b.index() == Direction::IndexOf<"West">());
}

TEST_CASE("StringEnum - Self-assignment is safe", "[xll::StringEnum][assignment]")
{
    Direction d { "North" };
    Direction& ref = d;
    REQUIRE_NOTHROW(d = ref);
    REQUIRE(d.index() == Direction::IndexOf<"North">());
}

// =============================================================================
// 3. valid() and raw-XLOPER12 bypass
// =============================================================================

TEST_CASE("StringEnum - valid() is true for normally constructed objects",
          "[xll::StringEnum][valid]")
{
    for (auto sv : Direction::values()) {
        Direction d { sv };
        REQUIRE(d.valid());
    }
}

TEST_CASE("StringEnum - valid() is false for raw-XLOPER12 bypass with unknown string",
          "[xll::StringEnum][valid]")
{
    // Simulate what Excel does: write an arbitrary string directly into the
    // XLOPER12 payload, bypassing all constructors.
    Direction d { "North" };    // start valid
    REQUIRE(d.valid());

    // Overwrite the underlying xll::String with an unknown value via the
    // base-class reference — no validation runs.
    static_cast<xll::String&>(d) = xll::String("Nowhere");

    REQUIRE_FALSE(d.valid());
    REQUIRE(d.index() == Direction::npos);
    REQUIRE_FALSE(d.value().has_value());
}

// =============================================================================
// 4. index()
// =============================================================================

TEST_CASE("StringEnum - index() returns correct zero-based index",
          "[xll::StringEnum][index]")
{
    REQUIRE(Direction("North").index() == 0);
    REQUIRE(Direction("South").index() == 1);
    REQUIRE(Direction("East").index()  == 2);
    REQUIRE(Direction("West").index()  == 3);
}

TEST_CASE("StringEnum - index() returns npos for invalid value",
          "[xll::StringEnum][index]")
{
    Direction d { "North" };
    static_cast<xll::String&>(d) = xll::String("Bad");
    REQUIRE(d.index() == Direction::npos);
}

TEST_CASE("StringEnum - npos equals numeric_limits max", "[xll::StringEnum][index]")
{
    STATIC_REQUIRE(Direction::npos == std::numeric_limits<std::size_t>::max());
}

// =============================================================================
// 5. is<Str>()
// =============================================================================

TEST_CASE("StringEnum - is<Str>() returns true only for matching element",
          "[xll::StringEnum][is]")
{
    Direction d { "East" };
    REQUIRE      (d.is<"East">());
    REQUIRE_FALSE(d.is<"North">());
    REQUIRE_FALSE(d.is<"South">());
    REQUIRE_FALSE(d.is<"West">());
}

TEST_CASE("StringEnum - is<Str>() returns false for invalid bypass value",
          "[xll::StringEnum][is]")
{
    Direction d { "North" };
    static_cast<xll::String&>(d) = xll::String("Bad");
    REQUIRE_FALSE(d.is<"North">());
    REQUIRE_FALSE(d.is<"South">());
}

// =============================================================================
// 6. value()
// =============================================================================

TEST_CASE("StringEnum - value() returns engaged Optional for valid element",
          "[xll::StringEnum][value]")
{
    Direction d { "South" };
    const auto v = d.value();
    REQUIRE(v.has_value());
    REQUIRE(v.value().to_string() == "South");
}

TEST_CASE("StringEnum - value() returns None for invalid bypass value",
          "[xll::StringEnum][value]")
{
    Direction d { "North" };
    static_cast<xll::String&>(d) = xll::String("Bad");
    REQUIRE_FALSE(d.value().has_value());
}

// =============================================================================
// 7. Static queries: size(), values(), IndexOf<>()
// =============================================================================

TEST_CASE("StringEnum - size()", "[xll::StringEnum][static]")
{
    STATIC_REQUIRE(Direction::size() == 4);
    STATIC_REQUIRE(Light::size()     == 3);
    STATIC_REQUIRE(Single::size()    == 1);
}

TEST_CASE("StringEnum - values()", "[xll::StringEnum][static]")
{
    constexpr auto v = Direction::values();
    STATIC_REQUIRE(v.size() == 4);
    REQUIRE(v[0] == "North");
    REQUIRE(v[1] == "South");
    REQUIRE(v[2] == "East");
    REQUIRE(v[3] == "West");
}

TEST_CASE("StringEnum - IndexOf<>()", "[xll::StringEnum][static]")
{
    STATIC_REQUIRE(Direction::IndexOf<"North">() == 0);
    STATIC_REQUIRE(Direction::IndexOf<"South">() == 1);
    STATIC_REQUIRE(Direction::IndexOf<"East">()  == 2);
    STATIC_REQUIRE(Direction::IndexOf<"West">()  == 3);

    STATIC_REQUIRE(Light::IndexOf<"Red">()   == 0);
    STATIC_REQUIRE(Light::IndexOf<"Amber">() == 1);
    STATIC_REQUIRE(Light::IndexOf<"Green">() == 2);
}

// =============================================================================
// 8. from_index()
// =============================================================================

TEST_CASE("StringEnum - from_index() constructs correct element",
          "[xll::StringEnum][from_index]")
{
    REQUIRE(Direction::from_index(0).index() == 0);
    REQUIRE(Direction::from_index(1).index() == 1);
    REQUIRE(Direction::from_index(2).index() == 2);
    REQUIRE(Direction::from_index(3).index() == 3);

    REQUIRE(Direction::from_index(0).value().value().to_string() == "North");
    REQUIRE(Direction::from_index(3).value().value().to_string() == "West");
}

TEST_CASE("StringEnum - from_index() throws std::out_of_range for out-of-range index",
          "[xll::StringEnum][from_index]")
{
    REQUIRE_THROWS_AS(Direction::from_index(4),  std::out_of_range);
    REQUIRE_THROWS_AS(Direction::from_index(99), std::out_of_range);
    REQUIRE_THROWS_AS(Direction::from_index(Direction::npos), std::out_of_range);
}

// =============================================================================
// 9. visit()
// =============================================================================

TEST_CASE("StringEnum - visit() dispatches to correct branch (void visitor)",
          "[xll::StringEnum][visit]")
{
    std::string called;
    Direction d { "East" };
    d.visit([&](auto tag) {
        using T = decltype(tag);
        if constexpr (std::is_same_v<T, Direction::Type<"North">>) called = "North";
        else if constexpr (std::is_same_v<T, Direction::Type<"South">>) called = "South";
        else if constexpr (std::is_same_v<T, Direction::Type<"East">>)  called = "East";
        else if constexpr (std::is_same_v<T, Direction::Type<"West">>)  called = "West";
        else                                                              called = "Unknown";
    });
    REQUIRE(called == "East");
}

TEST_CASE("StringEnum - visit() returns value from value-returning visitor",
          "[xll::StringEnum][visit]")
{
    auto opposite = [](const Direction& d) -> std::string {
        return d.visit([](auto tag) -> std::string {
            using T = decltype(tag);
            if constexpr (std::is_same_v<T, Direction::Type<"North">>) return "South";
            else if constexpr (std::is_same_v<T, Direction::Type<"South">>) return "North";
            else if constexpr (std::is_same_v<T, Direction::Type<"East">>)  return "West";
            else if constexpr (std::is_same_v<T, Direction::Type<"West">>)  return "East";
            else return "?";
        });
    };

    REQUIRE(opposite(Direction("North")) == "South");
    REQUIRE(opposite(Direction("South")) == "North");
    REQUIRE(opposite(Direction("East"))  == "West");
    REQUIRE(opposite(Direction("West"))  == "East");
}

TEST_CASE("StringEnum - visit() dispatches to Unknown for invalid bypass value",
          "[xll::StringEnum][visit]")
{
    Direction d { "North" };
    static_cast<xll::String&>(d) = xll::String("Bad");

    bool got_unknown = false;
    d.visit([&](auto tag) {
        using T = decltype(tag);
        if constexpr (std::is_same_v<T, Direction::Unknown>)
            got_unknown = true;
    });
    REQUIRE(got_unknown);
}

TEST_CASE("StringEnum - visit() cycles through all elements",
          "[xll::StringEnum][visit]")
{
    auto next_light = [](const Light& l) -> Light {
        return l.visit([](auto tag) -> Light {
            using T = decltype(tag);
            if constexpr (std::is_same_v<T, Light::Type<"Red">>)   return Light("Amber");
            else if constexpr (std::is_same_v<T, Light::Type<"Amber">>) return Light("Green");
            else if constexpr (std::is_same_v<T, Light::Type<"Green">>) return Light("Red");
            else return Light("Red");
        });
    };

    Light l { "Red" };
    l = next_light(l); REQUIRE(l.index() == Light::IndexOf<"Amber">());
    l = next_light(l); REQUIRE(l.index() == Light::IndexOf<"Green">());
    l = next_light(l); REQUIRE(l.index() == Light::IndexOf<"Red">());
}

// =============================================================================
// 10. switch on index()
// =============================================================================

TEST_CASE("StringEnum - index() can be used in a switch statement",
          "[xll::StringEnum][index][switch]")
{
    for (std::size_t i = 0; i < Direction::size(); ++i) {
        Direction d = Direction::from_index(i);
        std::string result;
        switch (d.index()) {
            case Direction::IndexOf<"North">(): result = "N"; break;
            case Direction::IndexOf<"South">(): result = "S"; break;
            case Direction::IndexOf<"East">():  result = "E"; break;
            case Direction::IndexOf<"West">():  result = "W"; break;
            default: result = "?"; break;
        }
        REQUIRE(result != "?");
    }
}

// =============================================================================
// 11. Comparison operators
// =============================================================================

TEST_CASE("StringEnum - operator== between two StringEnums",
          "[xll::StringEnum][comparison]")
{
    Direction a { "North" };
    Direction b { "North" };
    Direction c { "South" };
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE(a != c);
}

TEST_CASE("StringEnum - operator== with string_view",
          "[xll::StringEnum][comparison]")
{
    Direction d { "East" };
    REQUIRE(d == std::string_view("East"));
    REQUIRE_FALSE(d == std::string_view("West"));
}

TEST_CASE("StringEnum - operator== with string literal",
          "[xll::StringEnum][comparison]")
{
    Direction d { "West" };
    REQUIRE(d == "West");
    REQUIRE_FALSE(d == "East");
}

TEST_CASE("StringEnum - operator== with xll::String",
          "[xll::StringEnum][comparison]")
{
    Direction d { "South" };
    REQUIRE(d == xll::String("South"));
    REQUIRE_FALSE(d == xll::String("North"));
}

// =============================================================================
// 12. operator<< and to_string()
// =============================================================================

TEST_CASE("StringEnum - operator<< streams raw string regardless of validity",
          "[xll::StringEnum][stream]")
{
    SECTION("Valid value")
    {
        Direction d { "North" };
        std::ostringstream oss;
        oss << d;
        REQUIRE(oss.str() == "North");
    }

    SECTION("Invalid bypass value still streams without throwing")
    {
        Direction d { "North" };
        static_cast<xll::String&>(d) = xll::String("Bad");
        std::ostringstream oss;
        REQUIRE_NOTHROW(oss << d);
        REQUIRE(oss.str() == "Bad");
    }
}

TEST_CASE("StringEnum - to_string() returns engaged Optional for valid element",
          "[xll::StringEnum][to_string]")
{
    Direction d { "East" };
    const auto s = to_string(d);
    REQUIRE(s.has_value());
    REQUIRE(s.value().to_string() == "East");
}

TEST_CASE("StringEnum - to_string() returns None for invalid bypass value",
          "[xll::StringEnum][to_string]")
{
    Direction d { "North" };
    static_cast<xll::String&>(d) = xll::String("Bad");
    REQUIRE_FALSE(to_string(d).has_value());
}

// =============================================================================
// 13. std::hash and use in unordered_map
// =============================================================================

TEST_CASE("StringEnum - usable as key in std::unordered_map",
          "[xll::StringEnum][hash]")
{
    std::unordered_map<Direction, int> m;
    m[Direction{"North"}] = 0;
    m[Direction{"East"}]  = 90;
    m[Direction{"South"}] = 180;
    m[Direction{"West"}]  = 270;

    REQUIRE(m.at(Direction{"North"}) == 0);
    REQUIRE(m.at(Direction{"East"})  == 90);
    REQUIRE(m.at(Direction{"South"}) == 180);
    REQUIRE(m.at(Direction{"West"})  == 270);
}

// =============================================================================
// 14. Single-element StringEnum
// =============================================================================

TEST_CASE("StringEnum - single-element enum",
          "[xll::StringEnum][single]")
{
    Single k { "EXACT" };
    REQUIRE(k.valid());
    REQUIRE(k.index() == 0);
    REQUIRE(k.is<"EXACT">());
    REQUIRE(k.value().value().to_string() == "EXACT");

    REQUIRE_THROWS_AS(Single { "APPROXIMATE" }, std::invalid_argument);
    REQUIRE_THROWS_AS(Single { "" },            std::invalid_argument);
}

// =============================================================================
// 15. Memory layout
// =============================================================================

TEST_CASE("StringEnum - sizeof equals sizeof(XLOPER12)",
          "[xll::StringEnum][layout]")
{
    STATIC_REQUIRE(sizeof(Direction) == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(Light)     == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(Single)    == sizeof(XLOPER12));
}


