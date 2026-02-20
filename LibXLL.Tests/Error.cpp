// ============================================================
// Tests for xll::Error
// Covers: construction, copy/move, assignment, equality,
//         error_id/error_index, to_string, stream output,
//         explicit integral conversion, swap, and type safety.
// ============================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Error.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"

#include <sstream>
#include <type_traits>

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::Error) == sizeof(XLOPER12));
static_assert(alignof(xll::Error) == alignof(XLOPER12));
static_assert(xll::Error::has_crtp_base);
static_assert(xll::Error::excel_type == xltypeErr);

// Error IS constructible from arithmetic types via the inherited value constructor
// (even though no arithmetic union member exists for xltypeErr – these ctors are
// present because Base<Error, xltypeErr> uses 'using BASE::BASE').
// All will THROW at runtime because xltype validation fails.
static_assert(std::is_constructible_v<xll::Error, int>);
static_assert(std::is_constructible_v<xll::Error, double>);
static_assert(std::is_constructible_v<xll::Error, bool>);

// Error IS constructible from other xll types (all inherit XLOPER12, so the
// explicit Base(const XLOPER12&) constructor is reachable). Will throw at runtime.
static_assert(std::is_constructible_v<xll::Error, xll::Bool>);
static_assert(std::is_constructible_v<xll::Error, xll::Int>);
static_assert(std::is_constructible_v<xll::Error, xll::Number>);
static_assert(std::is_constructible_v<xll::Error, xll::Nil>);
static_assert(std::is_constructible_v<xll::Error, xll::Missing>);

// Error IS constructible from XLOPER12 directly
static_assert(std::is_constructible_v<xll::Error, XLOPER12>);

// Error is NOT implicitly convertible to bool
static_assert(!std::is_convertible_v<xll::Error, bool>);

// Error is NOT implicitly convertible to other xll numeric types
static_assert(!std::is_convertible_v<xll::Error, xll::Bool>);
static_assert(!std::is_convertible_v<xll::Error, xll::Int>);

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Error - Construction", "[xll::Error][construction]")
{
    SECTION("Construction from XLOPER12 with xltypeErr") {
        XLOPER12 xl{};
        xl.xltype  = xltypeErr;
        xl.val.err = xlerrNull;
        xll::Error e(xl);
        REQUIRE(e.xltype == xltypeErr);
        REQUIRE(e.is_valid());
        REQUIRE(e.error_id() == xlerrNull);
    }

    SECTION("Construction from XLOPER12 with wrong type throws") {
        XLOPER12 xl{};
        xl.xltype = xltypeNum;
        REQUIRE_THROWS(xll::Error(xl));
    }

    SECTION("Construction via predefined constants") {
        REQUIRE(xll::ErrNull.xltype  == xltypeErr);
        REQUIRE(xll::ErrDiv0.xltype  == xltypeErr);
        REQUIRE(xll::ErrValue.xltype == xltypeErr);
        REQUIRE(xll::ErrRef.xltype   == xltypeErr);
        REQUIRE(xll::ErrName.xltype  == xltypeErr);
        REQUIRE(xll::ErrNum.xltype   == xltypeErr);
        REQUIRE(xll::ErrNA.xltype    == xltypeErr);
    }

    SECTION("Predefined constants carry correct error codes") {
        REQUIRE(xll::ErrNull.error_id()  == xlerrNull);
        REQUIRE(xll::ErrDiv0.error_id()  == xlerrDiv0);
        REQUIRE(xll::ErrValue.error_id() == xlerrValue);
        REQUIRE(xll::ErrRef.error_id()   == xlerrRef);
        REQUIRE(xll::ErrName.error_id()  == xlerrName);
        REQUIRE(xll::ErrNum.error_id()   == xlerrNum);
        REQUIRE(xll::ErrNA.error_id()    == xlerrNA);
    }

    SECTION("Copy construction") {
        xll::Error src = xll::ErrDiv0;
        xll::Error dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst.error_id() == xlerrDiv0);
        REQUIRE(dst.xltype == xltypeErr);
        REQUIRE(dst.is_valid());
    }

    SECTION("Copy construction from invalid object throws") {
        xll::Error invalid = xll::ErrNull;
        invalid.xltype     = xltypeNil;
        REQUIRE_THROWS(xll::Error(invalid));
    }

    SECTION("Move construction") {
        xll::Error src = xll::ErrValue;
        xll::Error dst = std::move(src);
        REQUIRE(dst.error_id() == xlerrValue);
        REQUIRE(dst.xltype == xltypeErr);
        REQUIRE(dst.is_valid());
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Error - Assignment", "[xll::Error][assignment]")
{
    SECTION("Copy assignment") {
        xll::Error a = xll::ErrNull;
        xll::Error b = xll::ErrDiv0;
        a = b;
        REQUIRE(a == b);
        REQUIRE(a.error_id() == xlerrDiv0);
        REQUIRE(a.xltype == xltypeErr);
    }

    SECTION("Self-assignment") {
        xll::Error  e   = xll::ErrRef;
        xll::Error& ref = e;
        e = ref;
        REQUIRE(e.error_id() == xlerrRef);
        REQUIRE(e.xltype == xltypeErr);
    }

    SECTION("Move assignment") {
        xll::Error src = xll::ErrName;
        xll::Error dst = xll::ErrNull;
        dst = std::move(src);
        REQUIRE(dst.error_id() == xlerrName);
        REQUIRE(dst.xltype == xltypeErr);
    }
}

// =============================================================================
// EQUALITY TESTS
// =============================================================================

TEST_CASE("Error - Equality", "[xll::Error][comparison]")
{
    SECTION("Same error codes are equal") {
        xll::Error a = xll::ErrDiv0;
        xll::Error b = xll::ErrDiv0;
        REQUIRE(a == b);
        REQUIRE_FALSE(a != b);
    }

    SECTION("Different error codes are not equal") {
        REQUIRE_FALSE(xll::ErrNull  == xll::ErrDiv0);
        REQUIRE_FALSE(xll::ErrDiv0  == xll::ErrValue);
        REQUIRE_FALSE(xll::ErrValue == xll::ErrRef);
        REQUIRE_FALSE(xll::ErrRef   == xll::ErrName);
        REQUIRE_FALSE(xll::ErrName  == xll::ErrNum);
        REQUIRE_FALSE(xll::ErrNum   == xll::ErrNA);
    }

    SECTION("All seven predefined errors are distinct") {
        const xll::Error errors[] = {
            xll::ErrNull, xll::ErrDiv0, xll::ErrValue,
            xll::ErrRef,  xll::ErrName, xll::ErrNum, xll::ErrNA
        };
        for (int i = 0; i < 7; ++i)
            for (int j = 0; j < 7; ++j)
                if (i == j) REQUIRE(errors[i] == errors[j]);
                else        REQUIRE(errors[i] != errors[j]);
    }
}

// =============================================================================
// error_id / error_index TESTS
// =============================================================================

TEST_CASE("Error - error_id and error_index", "[xll::Error][accessors]")
{
    SECTION("error_id returns raw Excel error code") {
        REQUIRE(xll::ErrNull.error_id()  == xlerrNull);   // 0
        REQUIRE(xll::ErrDiv0.error_id()  == xlerrDiv0);   // 7
        REQUIRE(xll::ErrValue.error_id() == xlerrValue);  // 15
        REQUIRE(xll::ErrRef.error_id()   == xlerrRef);    // 23
        REQUIRE(xll::ErrName.error_id()  == xlerrName);   // 29
        REQUIRE(xll::ErrNum.error_id()   == xlerrNum);    // 36
        REQUIRE(xll::ErrNA.error_id()    == xlerrNA);     // 42
    }

    SECTION("error_index returns zero-based sequential index") {
        REQUIRE(xll::ErrNull.error_index()  == 0);
        REQUIRE(xll::ErrDiv0.error_index()  == 1);
        REQUIRE(xll::ErrValue.error_index() == 2);
        REQUIRE(xll::ErrRef.error_index()   == 3);
        REQUIRE(xll::ErrName.error_index()  == 4);
        REQUIRE(xll::ErrNum.error_index()   == 5);
        REQUIRE(xll::ErrNA.error_index()    == 6);
    }

    SECTION("error_id on invalid object throws") {
        xll::Error e   = xll::ErrNull;
        e.xltype       = xltypeNil;
        REQUIRE_THROWS(e.error_id());
    }

    SECTION("error_index on invalid object throws") {
        xll::Error e   = xll::ErrNull;
        e.xltype       = xltypeNil;
        REQUIRE_THROWS(e.error_index());
    }
}

// =============================================================================
// EXPLICIT INTEGRAL CONVERSION TESTS
// =============================================================================

TEST_CASE("Error - Explicit Integral Conversion", "[xll::Error][conversion]")
{
    // Error::operator T() is explicit (requires static_cast<int>).
    // error_index() is the underlying implementation; both return the same value.
    SECTION("error_index() returns sequential zero-based index") {
        REQUIRE(xll::ErrNull.error_index()  == 0);
        REQUIRE(xll::ErrDiv0.error_index()  == 1);
        REQUIRE(xll::ErrValue.error_index() == 2);
        REQUIRE(xll::ErrRef.error_index()   == 3);
        REQUIRE(xll::ErrName.error_index()  == 4);
        REQUIRE(xll::ErrNum.error_index()   == 5);
        REQUIRE(xll::ErrNA.error_index()    == 6);
    }
}

// =============================================================================
// to_string TESTS
// =============================================================================

TEST_CASE("Error - to_string", "[xll::Error][to_string]")
{
    SECTION("Returns correct Excel error string for each error") {
        // to_string() returns an xll::String; compare via stream output
        std::ostringstream os;

        auto check = [&](const xll::Error& e, const std::string& expected) {
            os.str("");
            os << e;
            REQUIRE(os.str() == expected);
        };

        check(xll::ErrNull,  "#NULL!");
        check(xll::ErrDiv0,  "#DIV/0!");
        check(xll::ErrValue, "#VALUE!");
        check(xll::ErrRef,   "#REF!");
        check(xll::ErrName,  "#NAME?");
        check(xll::ErrNum,   "#NUM!");
        check(xll::ErrNA,    "#N/A");
    }
}

// =============================================================================
// STREAM OUTPUT TESTS
// =============================================================================

TEST_CASE("Error - Stream Output", "[xll::Error][stream]")
{
    SECTION("Each error streams its Excel string representation") {
        auto str = [](const xll::Error& e) {
            std::ostringstream os;
            os << e;
            return os.str();
        };

        REQUIRE(str(xll::ErrNull)  == "#NULL!");
        REQUIRE(str(xll::ErrDiv0)  == "#DIV/0!");
        REQUIRE(str(xll::ErrValue) == "#VALUE!");
        REQUIRE(str(xll::ErrRef)   == "#REF!");
        REQUIRE(str(xll::ErrName)  == "#NAME?");
        REQUIRE(str(xll::ErrNum)   == "#NUM!");
        REQUIRE(str(xll::ErrNA)    == "#N/A");
    }
}

// =============================================================================
// SWAP TESTS
// =============================================================================

TEST_CASE("Error - Swap", "[xll::Error][swap]")
{
    SECTION("Swap exchanges error codes and preserves xltype") {
        xll::Error a = xll::ErrNull;
        xll::Error b = xll::ErrNA;
        using std::swap;
        swap(a, b);
        REQUIRE(a.error_id() == xlerrNA);
        REQUIRE(b.error_id() == xlerrNull);
        REQUIRE(a.xltype == xltypeErr);
        REQUIRE(b.xltype == xltypeErr);
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("Error - Validity and Layout", "[xll::Error][validity][layout]")
{
    SECTION("is_valid reflects xltype") {
        xll::Error e = xll::ErrDiv0;
        REQUIRE(e.is_valid());
        e.xltype = xltypeNil;
        REQUIRE_FALSE(e.is_valid());
    }

    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::Error) == sizeof(XLOPER12));
    }
}





