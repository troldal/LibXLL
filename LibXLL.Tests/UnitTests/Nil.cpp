// ============================================================
// Tests for xll::Nil
// ============================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"

#include <type_traits>

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::Nil) == sizeof(XLOPER12));
static_assert(alignof(xll::Nil) == alignof(XLOPER12));
static_assert(xll::Nil::excel_type == xltypeNil);

// Nil is NOT constructible from arithmetic types
static_assert(!std::is_constructible_v<xll::Nil, int>);
static_assert(!std::is_constructible_v<xll::Nil, double>);
static_assert(!std::is_constructible_v<xll::Nil, bool>);

// Nil is NOT constructible from Bool, Int, or Number
static_assert(!std::is_constructible_v<xll::Nil, xll::Bool>);
static_assert(!std::is_constructible_v<xll::Nil, xll::Int>);
static_assert(!std::is_constructible_v<xll::Nil, xll::Number>);

// Nil IS constructible from Missing via Missing::operator Nil()
static_assert(std::is_constructible_v<xll::Nil, xll::Missing>);

// Nil is NOT implicitly convertible to arithmetic types
static_assert(!std::is_convertible_v<xll::Nil, int>);
static_assert(!std::is_convertible_v<xll::Nil, double>);
static_assert(!std::is_convertible_v<xll::Nil, bool>);

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Nil - Construction", "[xll::Nil][construction]")
{
    SECTION("Default construction") {
        xll::Nil n;
        REQUIRE(n.xltype == xltypeNil);
    }

    SECTION("Copy construction") {
        xll::Nil a;
        xll::Nil b = a;
        REQUIRE(b.xltype == xltypeNil);
    }

    SECTION("Construction from Missing via operator Nil()") {
        xll::Missing m;
        xll::Nil     n = static_cast<xll::Nil>(m);
        REQUIRE(n.xltype == xltypeNil);
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Nil - Assignment", "[xll::Nil][assignment]")
{
    SECTION("Copy assignment") {
        xll::Nil a;
        xll::Nil b;
        b = a;
        REQUIRE(b.xltype == xltypeNil);
    }

    SECTION("Self-assignment") {
        xll::Nil  n;
        xll::Nil& ref = n;
        n = ref;    // exercises operator=(const Nil&) with self
        REQUIRE(n.xltype == xltypeNil);
    }

    SECTION("Assignment from Missing via operator Nil()") {
        xll::Nil     n;
        xll::Missing m;
        n = static_cast<xll::Nil>(m);
        REQUIRE(n.xltype == xltypeNil);
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("Nil - Comparison", "[xll::Nil][comparison]")
{
    SECTION("All Nil values are equal") {
        xll::Nil a;
        xll::Nil b;
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);
    }
}

// =============================================================================
// CROSS-TYPE TESTS
// =============================================================================

TEST_CASE("Nil - Cross-type Interaction with Missing", "[xll::Nil][cross-type]")
{
    SECTION("Missing converts to Nil via operator Nil()") {
        xll::Missing m;
        xll::Nil     n = static_cast<xll::Nil>(m);
        REQUIRE(n.xltype == xltypeNil);
    }

    SECTION("Original Missing is not affected by conversion") {
        xll::Missing m;
        xll::Nil     n = static_cast<xll::Nil>(m);
        REQUIRE(n.xltype == xltypeNil);
        REQUIRE(m.xltype == xltypeMissing);
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("Nil - Validity and Layout", "[xll::Nil][validity][layout]")
{
    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::Nil) == sizeof(XLOPER12));
    }
}
