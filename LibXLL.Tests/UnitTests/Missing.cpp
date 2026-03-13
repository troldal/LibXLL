// ============================================================
// Tests for xll::Missing
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

static_assert(sizeof(xll::Missing) == sizeof(XLOPER12));
static_assert(alignof(xll::Missing) == alignof(XLOPER12));
static_assert(xll::Missing::excel_type == xltypeMissing);

// Missing is NOT constructible from arithmetic types
static_assert(!std::is_constructible_v<xll::Missing, int>);
static_assert(!std::is_constructible_v<xll::Missing, double>);
static_assert(!std::is_constructible_v<xll::Missing, bool>);

// Missing is NOT constructible from Bool, Int, or Number
static_assert(!std::is_constructible_v<xll::Missing, xll::Bool>);
static_assert(!std::is_constructible_v<xll::Missing, xll::Int>);
static_assert(!std::is_constructible_v<xll::Missing, xll::Number>);

// Missing is NOT constructible from Nil (one-way: Missing -> Nil only)
static_assert(!std::is_constructible_v<xll::Missing, xll::Nil>);

// Missing IS implicitly convertible to Nil via operator xll::Nil()
static_assert(std::is_convertible_v<xll::Missing, xll::Nil>);

// Missing is NOT implicitly convertible to arithmetic types
static_assert(!std::is_convertible_v<xll::Missing, int>);
static_assert(!std::is_convertible_v<xll::Missing, double>);
static_assert(!std::is_convertible_v<xll::Missing, bool>);

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Missing - Construction", "[xll::Missing][construction]")
{
    SECTION("Default construction") {
        xll::Missing m;
        REQUIRE(m.xltype == xltypeMissing);
    }

    SECTION("Copy construction") {
        xll::Missing a;
        xll::Missing b = a;
        REQUIRE(b.xltype == xltypeMissing);
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Missing - Assignment", "[xll::Missing][assignment]")
{
    SECTION("Copy assignment") {
        xll::Missing a;
        xll::Missing b;
        b = a;
        REQUIRE(b.xltype == xltypeMissing);
    }

    SECTION("Self-assignment") {
        xll::Missing  m;
        xll::Missing& ref = m;
        m = ref;    // exercises operator=(const Missing&) with self
        REQUIRE(m.xltype == xltypeMissing);
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("Missing - Comparison", "[xll::Missing][comparison]")
{
    SECTION("All Missing values are equal") {
        xll::Missing a;
        xll::Missing b;
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);
    }

    SECTION("Multiple copies all compare equal") {
        xll::Missing a;
        xll::Missing b = a;
        xll::Missing c;
        c = b;
        REQUIRE(a == b);
        REQUIRE(b == c);
        REQUIRE(a == c);
    }
}

// =============================================================================
// CROSS-TYPE TESTS
// =============================================================================

TEST_CASE("Missing - Cross-type Interaction with Nil", "[xll::Missing][cross-type]")
{
    SECTION("Implicit conversion to Nil via operator Nil()") {
        xll::Missing m;
        xll::Nil     n = m;    // Missing::operator Nil() – unambiguous
        REQUIRE(n.xltype == xltypeNil);
        REQUIRE(m.xltype == xltypeMissing);    // original unaffected
    }

    SECTION("Nil copy-constructed from Missing{}") {
        xll::Nil n = xll::Missing{};
        REQUIRE(n.xltype == xltypeNil);
    }

    SECTION("Nil copy-assigned from Missing{}") {
        xll::Nil n;
        n = xll::Missing{};
        REQUIRE(n.xltype == xltypeNil);
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("Missing - Validity and Layout", "[xll::Missing][validity][layout]")
{
    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::Missing) == sizeof(XLOPER12));
    }
}
