// DemoStringEnum.cpp
// Demonstrates xll::StringEnum — an Excel-compatible, compile-time string-based enum.

#include <Types/StringEnum.hpp>
#include <fxt/utils/Overload.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void separator(const char* title)
{
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================================\n";
}

static std::string index_str(std::size_t idx)
{
    if (idx == Direction::npos) return "npos";
    return std::to_string(idx);
}

static void subsection(const char* title)
{
    std::cout << "\n--- " << title << " ---\n";
}

// ---------------------------------------------------------------------------
// Type aliases used throughout the demo
// ---------------------------------------------------------------------------

// A cardinal direction — four known values.
using Direction = xll::StringEnum<"North", "South", "East", "West">;

// A simple traffic-light enum.
using Light = xll::StringEnum<"Red", "Amber", "Green">;

// A single-element enum — useful for a required keyword argument.
using Keyword = xll::StringEnum<"EXACT">;

// ---------------------------------------------------------------------------
// Section 1: Construction and validity
// ---------------------------------------------------------------------------

void demo_construction()
{
    separator("1. Construction and validity");

    subsection("1a. Default construction (first element)");
    {
        Direction d;
        std::cout << "  value()  = " << d.value().value()    << "\n";
        std::cout << "  valid()  = " << d.valid()            << "\n";
        std::cout << "  index()  = " << index_str(d.index()) << "\n";
    }

    subsection("1b. Construction from known xll::String");
    {
        Direction d { xll::String("East") };
        std::cout << "  value()  = " << d.value().value()    << "\n";
        std::cout << "  valid()  = " << d.valid()            << "\n";
        std::cout << "  index()  = " << index_str(d.index()) << "\n";
    }

    subsection("1c. Construction from unknown xll::String — throws std::invalid_argument");
    {
        try {
            Direction d { xll::String("Up") };
            std::cout << "  (should not reach here)\n";
        } catch (const std::invalid_argument& e) {
            std::cout << "  threw: " << e.what() << "\n";
        }
    }

    subsection("1d. Construction from string literal");
    {
        Direction d { "South" };
        std::cout << "  value()  = " << d.value().value() << "\n";
        std::cout << "  valid()  = " << d.valid()         << "\n";
    }

    subsection("1e. Construction from std::string_view");
    {
        const std::string_view sv = "West";
        Direction d { sv };
        std::cout << "  value()  = " << d.value().value() << "\n";
        std::cout << "  valid()  = " << d.valid()         << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 2: Assignment
// ---------------------------------------------------------------------------

void demo_assignment()
{
    separator("2. Assignment");

    subsection("2a. Assign from xll::String");
    {
        Direction d;
        d = xll::String("South");
        std::cout << "  value() = " << d.value().value() << "  valid() = " << d.valid() << "\n";
    }

    subsection("2b. Assign unknown string — throws std::invalid_argument");
    {
        Direction d { "North" };
        try {
            d = xll::String("Down");
            std::cout << "  (should not reach here)\n";
        } catch (const std::invalid_argument& e) {
            std::cout << "  threw: " << e.what() << "\n";
            std::cout << "  value() still = " << d.value().value() << "\n";
        }
    }

    subsection("2c. Assign from string literal");
    {
        Direction d;
        d = "West";
        std::cout << "  value() = " << d.value().value() << "  valid() = " << d.valid() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 3: Index and element queries
// ---------------------------------------------------------------------------

void demo_queries()
{
    separator("3. Index and element queries");

    subsection("3a. index() on valid value");
    {
        Direction d { "East" };
        const auto idx = d.index();
        std::cout << "  index() = " << index_str(idx) << "\n";
        // IndexOf<> is a compile-time constant
        std::cout << "  IndexOf<\"East\">() = " << Direction::IndexOf<"East">() << "\n";
    }

    subsection("3b. index() for unknown string — construction throws");
    {
        try {
            Direction d { "Up" };
            std::cout << "  (should not reach here)\n";
        } catch (const std::invalid_argument& e) {
            std::cout << "  threw: " << e.what() << "\n";
        }
    }

    subsection("3c. is<Str>()");
    {
        Direction d { "South" };
        std::cout << "  is<\"North\">() = " << d.is<"North">() << "\n";
        std::cout << "  is<\"South\">() = " << d.is<"South">() << "\n";
        std::cout << "  is<\"East\">()  = " << d.is<"East">()  << "\n";
        std::cout << "  is<\"West\">()  = " << d.is<"West">()  << "\n";
    }

    subsection("3d. Static size() and values()");
    {
        std::cout << "  Direction::size() = " << Direction::size() << "\n";
        std::cout << "  Direction::values():";
        for (auto v : Direction::values())
            std::cout << " \"" << v << "\"";
        std::cout << "\n";
    }

    subsection("3e. from_index()");
    {
        for (std::size_t i = 0; i < Direction::size(); ++i) {
            auto d = Direction::from_index(i);
            std::cout << "  from_index(" << i << ") = \"" << d.value().value() << "\"\n";
        }

        try {
            Direction::from_index(99);
        } catch (const std::out_of_range& e) {
            std::cout << "  from_index(99) threw: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Section 4: Visitation
// ---------------------------------------------------------------------------

void demo_visit()
{
    separator("4. Visitation");

    subsection("4a. void visitor — valid value");
    {
        Direction d { "West" };
        d.visit(fxt::overload{
            [](Direction::Type<"North">) { std::cout << "  -> North\n"; },
            [](Direction::Type<"South">) { std::cout << "  -> South\n"; },
            [](Direction::Type<"East">)  { std::cout << "  -> East\n";  },
            [](Direction::Type<"West">)  { std::cout << "  -> West\n";  },
            [](Direction::Unknown)       { std::cout << "  -> Unknown\n"; },
        });
    }

    subsection("4b. valid() is always true for normally constructed objects");
    {
        for (auto sv : Direction::values()) {
            Direction d { sv };
            std::cout << "  \"" << d.value().value() << "\"  valid() = " << d.valid() << "\n";
        }
        std::cout << "  (Unknown{} branch is only reachable via raw XLOPER12 bypass)\n";
    }

    subsection("4c. value-returning visitor — compute opposite direction");
    {
        auto opposite = [](const Direction& d) -> std::string {
            return d.visit(fxt::overload{
                [](Direction::Type<"North">) -> std::string { return "South"; },
                [](Direction::Type<"South">) -> std::string { return "North"; },
                [](Direction::Type<"East">)  -> std::string { return "West";  },
                [](Direction::Type<"West">)  -> std::string { return "East";  },
                [](Direction::Unknown)       -> std::string { return "?";     },
            });
        };

        for (auto sv : Direction::values()) {
            Direction d { sv };
            std::cout << "  opposite(\"" << d.value().value() << "\") = \""
                      << opposite(d) << "\"\n";
        }
    }

    subsection("4d. Traffic light — next state");
    {
        auto next_light = [](const Light& l) -> Light {
            return l.visit(fxt::overload{
                [](Light::Type<"Red">)   -> Light { return Light("Amber"); },
                [](Light::Type<"Amber">) -> Light { return Light("Green"); },
                [](Light::Type<"Green">) -> Light { return Light("Red");   },
                [](Light::Unknown)       -> Light { return Light("Red");   },
            });
        };

        Light l { "Red" };
        for (int i = 0; i < 5; ++i) {
            std::cout << "  " << l.value().value() << " -> ";
            l = next_light(l);
            std::cout << l.value().value() << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Section 5: Comparison and stream output
// ---------------------------------------------------------------------------

void demo_comparison()
{
    separator("5. Comparison and stream output");

    subsection("5a. Equality between StringEnum objects");
    {
        Direction a { "North" };
        Direction b { "North" };
        Direction c { "South" };
        std::cout << "  a == b : " << (a == b) << "\n";
        std::cout << "  a == c : " << (a == c) << "\n";
        std::cout << "  a != c : " << (a != c) << "\n";
    }

    subsection("5b. Equality with string_view");
    {
        Direction d { "East" };
        std::cout << "  d == \"East\"  : " << (d == "East")  << "\n";
        std::cout << "  d == \"West\"  : " << (d == "West")  << "\n";
    }

    subsection("5c. Stream output");
    {
        Direction d { "South" };
        // operator<< streams the raw string regardless of validity
        std::cout << "  std::cout << d : " << d << "\n";
        // to_string() returns Optional<String> — use .value() to get the string
        std::cout << "  to_string(d)   : " << to_string(d).value() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 6: Use in unordered_map (hash support)
// ---------------------------------------------------------------------------

void demo_hash()
{
    separator("6. Use in std::unordered_map (hash support)");

    std::unordered_map<Direction, std::string, std::hash<Direction>> compass;
    compass[Direction{"North"}] = "0\u00b0";
    compass[Direction{"East"}]  = "90\u00b0";
    compass[Direction{"South"}] = "180\u00b0";
    compass[Direction{"West"}]  = "270\u00b0";

    for (auto sv : Direction::values()) {
        Direction d { sv };
        std::cout << "  " << d.value().value() << " -> " << compass[d] << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 7: Simulating an Excel function argument
// ---------------------------------------------------------------------------

void demo_excel_argument()
{
    separator("7. Simulating an Excel function argument");

    auto process = [](const xll::String& cell) {
        try {
            Direction dir { cell };
            std::cout << "  Received: \"" << dir.value().value() << "\"  ";
            dir.visit(fxt::overload{
                [](Direction::Type<"North">) { std::cout << "-> moving North\n"; },
                [](Direction::Type<"South">) { std::cout << "-> moving South\n"; },
                [](Direction::Type<"East">)  { std::cout << "-> moving East\n";  },
                [](Direction::Type<"West">)  { std::cout << "-> moving West\n";  },
                [](Direction::Unknown)       { /* raw-XLOPER12 bypass only */    },
            });
        } catch (const std::invalid_argument& e) {
            std::cout << "  Rejected: " << e.what() << "\n";
        }
    };

    const std::vector<xll::String> cell_values {
        xll::String("North"),
        xll::String("east"),
        xll::String("West"),
        xll::String(""),
        xll::String("South"),
    };

    for (const auto& cell : cell_values)
        process(cell);
}

// ---------------------------------------------------------------------------
// Section 8: Single-element keyword enum
// ---------------------------------------------------------------------------

void demo_keyword()
{
    separator("8. Single-element keyword enum");

    subsection("8a. Valid keyword");
    {
        Keyword kw { "EXACT" };
        std::cout << "  value()  = " << kw.value().value() << "\n";
        std::cout << "  valid()  = " << kw.valid()         << "\n";
        std::cout << "  is<\"EXACT\">() = " << kw.is<"EXACT">() << "\n";
    }

    subsection("8b. Wrong keyword — throws std::invalid_argument");
    {
        try {
            Keyword kw { "APPROXIMATE" };
            std::cout << "  (should not reach here)\n";
        } catch (const std::invalid_argument& e) {
            std::cout << "  threw: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << std::boolalpha;

    demo_construction();
    demo_assignment();
    demo_queries();
    demo_visit();
    demo_comparison();
    demo_hash();
    demo_excel_argument();
    demo_keyword();

    std::cout << "\n========================================================\n";
    std::cout << "  Done.\n";
    std::cout << "========================================================\n\n";
    return 0;
}

