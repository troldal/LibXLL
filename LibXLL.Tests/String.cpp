// ============================================================
// Tests for xll::String
// Covers: construction, assignment, concatenation, comparison,
//         conversion, stream output, size/empty/length, clear,
//         trim, to_upper, literal operator, and type safety.
// ============================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Error.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"
#include "../Types/String.hpp"

#include <sstream>
#include <stdexcept>
#include <type_traits>

using namespace xll::literals;


// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::String) == sizeof(XLOPER12));
static_assert(alignof(xll::String) == alignof(XLOPER12));
static_assert(xll::String::has_crtp_base);
static_assert(xll::String::excel_type == xltypeStr);

// String IS constructible from const char*, std::string_view, std::string
static_assert(std::is_constructible_v<xll::String, const char*>);
static_assert(std::is_constructible_v<xll::String, std::string_view>);
static_assert(std::is_constructible_v<xll::String, std::string>);

// String is NOT constructible from arithmetic types
static_assert(!std::is_constructible_v<xll::String, int>);
static_assert(!std::is_constructible_v<xll::String, double>);
static_assert(!std::is_constructible_v<xll::String, bool>);

// String IS constructible from other xll types via the inherited explicit
// Base(const XLOPER12&) constructor — but throws at runtime (xltype mismatch).
static_assert(std::is_constructible_v<xll::String, xll::Int>);
static_assert(std::is_constructible_v<xll::String, xll::Number>);
static_assert(std::is_constructible_v<xll::String, xll::Bool>);
static_assert(std::is_constructible_v<xll::String, xll::Error>);
static_assert(std::is_constructible_v<xll::String, xll::Nil>);
static_assert(std::is_constructible_v<xll::String, xll::Missing>);

// String IS implicitly convertible to std::string
static_assert(std::is_convertible_v<xll::String, std::string>);

// String is NOT implicitly convertible to arithmetic types
static_assert(!std::is_convertible_v<xll::String, int>);
static_assert(!std::is_convertible_v<xll::String, double>);
static_assert(!std::is_convertible_v<xll::String, bool>);

// String is NOT implicitly convertible to other xll types
static_assert(!std::is_convertible_v<xll::String, xll::Int>);
static_assert(!std::is_convertible_v<xll::String, xll::Number>);
static_assert(!std::is_convertible_v<xll::String, xll::Bool>);

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("String - Construction", "[xll::String][construction]")
{
    SECTION("Default construction yields empty string") {
        xll::String s;
        REQUIRE(s.xltype == xltypeStr);
        REQUIRE(s.is_valid());
        REQUIRE(s.empty());
        REQUIRE(s.size() == 0);
        REQUIRE(s == "");
    }

    SECTION("Construction from const char*") {
        xll::String s("hello");
        REQUIRE(s.xltype == xltypeStr);
        REQUIRE(s.is_valid());
        REQUIRE_FALSE(s.empty());
        REQUIRE(s == "hello");
    }

    SECTION("Construction from std::string_view") {
        std::string_view sv = "world";
        xll::String s(sv);
        REQUIRE(s.xltype == xltypeStr);
        REQUIRE(s == "world");
    }

    SECTION("Construction from std::string") {
        std::string str = "hello world";
        xll::String s(str);
        REQUIRE(s.xltype == xltypeStr);
        REQUIRE(s == "hello world");
    }

    SECTION("Construction from empty const char*") {
        xll::String s("");
        REQUIRE(s.empty());
        REQUIRE(s.size() == 0);
        REQUIRE(s.xltype == xltypeStr);
    }

    SECTION("Construction from XLOPER12 with correct type") {
        xll::String src("test");
        const XLOPER12& xl = static_cast<const XLOPER12&>(src);
        xll::String dst(xl);
        REQUIRE(dst == "test");
        REQUIRE(dst.xltype == xltypeStr);
    }

    SECTION("Construction from XLOPER12 with wrong type throws") {
        XLOPER12 xl{};
        xl.xltype = xltypeNum;
        REQUIRE_THROWS(xll::String(xl));
    }

    SECTION("Construction from other xll types throws at runtime (xltype mismatch)") {
        REQUIRE_THROWS(xll::String(xll::Int(42)));
        REQUIRE_THROWS(xll::String(xll::Number(3.14)));
        REQUIRE_THROWS(xll::String(xll::Bool(true)));
    }

    SECTION("Copy construction") {
        xll::String src("copy me");
        xll::String dst(src);
        REQUIRE(dst == src);
        REQUIRE(dst == "copy me");
        REQUIRE(dst.xltype == xltypeStr);
        REQUIRE(dst.val.str != src.val.str);    // independent allocation
    }

    SECTION("Copy construction from empty string") {
        xll::String src;
        xll::String dst(src);
        REQUIRE(dst.empty());
        REQUIRE(dst.xltype == xltypeStr);
    }

    SECTION("Move construction") {
        xll::String src("move me");
        XCHAR* original_ptr = src.val.str;
        xll::String dst(std::move(src));
        REQUIRE(dst == "move me");
        REQUIRE(dst.xltype == xltypeStr);
        REQUIRE(dst.val.str == original_ptr);    // pointer was stolen
        REQUIRE(src.val.str == nullptr);         // source is nulled
    }

    SECTION("Literal operator _xs") {
        auto s = "literal"_xs;
        REQUIRE(s.xltype == xltypeStr);
        REQUIRE(s == "literal");
    }

    SECTION("Literal operator with empty string") {
        auto s = ""_xs;
        REQUIRE(s.empty());
        REQUIRE(s.xltype == xltypeStr);
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("String - Assignment", "[xll::String][assignment]")
{
    SECTION("Copy assignment") {
        xll::String a("hello");
        xll::String b("world");
        a = b;
        REQUIRE(a == "world");
        REQUIRE(b == "world");            // source unchanged
        REQUIRE(a.val.str != b.val.str);    // independent allocation
        REQUIRE(a.xltype == xltypeStr);
    }

    SECTION("Self copy assignment") {
        xll::String  s("self");
        xll::String& ref = s;
        s = ref;
        REQUIRE(s == "self");
        REQUIRE(s.xltype == xltypeStr);
    }

    SECTION("Move assignment") {
        xll::String a("hello");
        xll::String b("world");
        XCHAR* a_ptr = a.val.str;
        XCHAR* b_ptr = b.val.str;
        a = std::move(b);
        REQUIRE(a == "world");
        REQUIRE(a.val.str == b_ptr);    // a now holds b's old buffer
        REQUIRE(b.val.str == a_ptr);    // b holds a's old buffer (swap semantics)
        REQUIRE(a.xltype == xltypeStr);
    }

    SECTION("Self move assignment") {
        xll::String s("self");
        XCHAR* ptr = s.val.str;
        s = std::move(s);    // swap with self: pointer unchanged
        REQUIRE(s.xltype == xltypeStr);
        REQUIRE(s.val.str == ptr);
        REQUIRE(s == "self");
    }

    SECTION("Assignment from temporary") {
        xll::String s("old");
        s = xll::String("new");
        REQUIRE(s == "new");
        REQUIRE(s.xltype == xltypeStr);
    }

    SECTION("Copy assignment replaces existing content") {
        xll::String a("long string with content");
        xll::String b("x");
        a = b;
        REQUIRE(a == "x");
        REQUIRE(a.xltype == xltypeStr);
    }
}

// =============================================================================
// CONCATENATION TESTS
// =============================================================================

TEST_CASE("String - Concatenation", "[xll::String][arithmetic]")
{
    SECTION("String + String") {
        xll::String a("hello");
        xll::String b(" world");
        auto r = a + b;
        REQUIRE(r == "hello world");
        REQUIRE(r.xltype == xltypeStr);
        REQUIRE(a == "hello");      // operands unchanged
        REQUIRE(b == " world");
    }

    SECTION("String + const char*") {
        xll::String a("hello");
        auto r = a + " world";
        REQUIRE(r == "hello world");
        REQUIRE(r.xltype == xltypeStr);
    }

    SECTION("String + std::string") {
        xll::String a("hello");
        std::string b(" world");
        auto r = a + b;
        REQUIRE(r == "hello world");
        REQUIRE(r.xltype == xltypeStr);
    }

    SECTION("String + std::string_view") {
        xll::String      a("hello");
        std::string_view b(" world");
        auto r = a + b;
        REQUIRE(r == "hello world");
        REQUIRE(r.xltype == xltypeStr);
    }

    SECTION("Self-concatenation") {
        xll::String s("ab");
        auto r = s + s;
        REQUIRE(r == "abab");
        REQUIRE(s == "ab");    // original unchanged
    }

    SECTION("Concatenation with empty string") {
        xll::String a("hello");
        xll::String b;
        REQUIRE(a + b == "hello");
        REQUIRE(b + a == "hello");
        REQUIRE((b + b).empty());
    }

    SECTION("Chained concatenation") {
        xll::String a("a"), b("b"), c("c");
        auto r = a + b + c;
        REQUIRE(r == "abc");
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("String - Comparison", "[xll::String][comparison]")
{
    SECTION("Equality between two xll::String values") {
        xll::String a("hello");
        xll::String b("hello");
        xll::String c("world");
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
    }

    SECTION("Equality with std::string") {
        xll::String s("hello");
        REQUIRE(s.to_string() == std::string("hello"));
        REQUIRE(s.to_string() != std::string("world"));
    }

    SECTION("Equality with std::string_view") {
        xll::String      s("hello");
        std::string_view sv("hello");
        REQUIRE(s.to_string() == std::string(sv));
    }

    SECTION("Self-equality") {
        xll::String s("self");
        REQUIRE(s == s);
    }

    SECTION("Empty string equality") {
        xll::String a;
        xll::String b("");
        REQUIRE(a == b);
        REQUIRE(a.empty());
    }

    SECTION("Three-way comparison - xll::String vs xll::String") {
        xll::String a("apple");
        xll::String b("banana");
        xll::String c("apple");
        REQUIRE(bool((a <=> b) < 0));
        REQUIRE(bool((b <=> a) > 0));
        REQUIRE(bool((a <=> c) == 0));
        // Use to_string() for relational operators to avoid Catch2 decomposer issues
        REQUIRE(a.to_string() < b.to_string());
        REQUIRE(b.to_string() > a.to_string());
        REQUIRE(a.to_string() <= c.to_string());
        REQUIRE(a.to_string() >= c.to_string());
    }

    SECTION("Three-way comparison with std::string_view") {
        xll::String s("hello");
        REQUIRE(bool((s <=> std::string_view("hello")) == 0));
        REQUIRE(bool((s <=> std::string_view("world")) < 0));
        REQUIRE(bool((s <=> std::string_view("apple")) > 0));
    }

    SECTION("Three-way comparison with std::string") {
        xll::String s("hello");
        REQUIRE(bool((s <=> std::string("hello")) == 0));
        REQUIRE(bool((s <=> std::string("world")) < 0));
    }
}

// =============================================================================
// CONVERSION TESTS
// =============================================================================

TEST_CASE("String - Conversion", "[xll::String][conversion]")
{
    SECTION("Implicit conversion to std::string") {
        xll::String s("hello");
        std::string t = s;
        REQUIRE(t == "hello");
    }

    SECTION("to_string() member function") {
        xll::String s("world");
        REQUIRE(s.to_string() == "world");
    }

    SECTION("to_string() on empty string") {
        xll::String s;
        REQUIRE(s.to_string().empty());
    }

    SECTION("Round-trip: std::string -> xll::String -> std::string") {
        std::string original = "round trip";
        xll::String xs(original);
        std::string back = xs.to_string();
        REQUIRE(back == original);
    }

    SECTION("Round-trip preserves ASCII content") {
        for (const char* text : {"a", "hello world", "123", "!@#$%"}) {
            xll::String s(text);
            REQUIRE(s.to_string() == text);
        }
    }
}

// =============================================================================
// SIZE / EMPTY / LENGTH TESTS
// =============================================================================

TEST_CASE("String - Size, Empty, Length", "[xll::String][size]")
{
    SECTION("empty() on default-constructed string") {
        xll::String s;
        REQUIRE(s.empty());
    }

    SECTION("empty() on non-empty string") {
        xll::String s("hello");
        REQUIRE_FALSE(s.empty());
    }

    SECTION("empty() on empty literal") {
        xll::String s("");
        REQUIRE(s.empty());
    }

    SECTION("size() matches wide character count for ASCII") {
        xll::String s("hello");
        REQUIRE(s.size() == 5);
    }

    SECTION("size() on empty string returns 0") {
        xll::String s;
        REQUIRE(s.size() == 0);
    }

    SECTION("length() always equals size()") {
        xll::String s("hello");
        REQUIRE(s.length() == s.size());
        xll::String e;
        REQUIRE(e.length() == e.size());
    }

    SECTION("size() after concatenation") {
        xll::String a("hello");
        xll::String b(" world");
        auto r = a + b;
        REQUIRE(r.size() == 11);
    }

    SECTION("size() reflects content length") {
        REQUIRE(xll::String("a").size()   == 1);
        REQUIRE(xll::String("ab").size()  == 2);
        REQUIRE(xll::String("abc").size() == 3);
    }
}

// =============================================================================
// CLEAR TESTS
// =============================================================================

TEST_CASE("String - Clear", "[xll::String][clear]")
{
    SECTION("clear() leaves object empty") {
        xll::String s("hello");
        s.clear();
        REQUIRE(s.empty());
        REQUIRE(s.size() == 0);
        REQUIRE(s.val.str == nullptr);
    }

    SECTION("clear() on default-constructed string is safe") {
        xll::String s;
        REQUIRE_NOTHROW(s.clear());
        REQUIRE(s.empty());
    }

    SECTION("clear() twice is safe (no double-free)") {
        xll::String s("hello");
        s.clear();
        REQUIRE_NOTHROW(s.clear());
        REQUIRE(s.empty());
    }

    SECTION("clear() on moved-from object is safe") {
        xll::String a("hello");
        xll::String b(std::move(a));
        REQUIRE_NOTHROW(a.clear());    // a.val.str == nullptr
    }

    SECTION("Destructor after clear() is safe (no double-free)") {
        {
            xll::String s("hello");
            s.clear();
        }    // ~String() runs: delete[] nullptr is a no-op
        REQUIRE(true);
    }
}

// =============================================================================
// STREAM OUTPUT TESTS
// =============================================================================

TEST_CASE("String - Stream Output", "[xll::String][stream]")
{
    SECTION("Non-empty string streams correctly") {
        std::ostringstream os;
        os << xll::String("hello");
        REQUIRE(os.str() == "hello");
    }

    SECTION("Empty string streams empty") {
        std::ostringstream os;
        os << xll::String();
        REQUIRE(os.str().empty());
    }

    SECTION("Stream chaining works") {
        std::ostringstream os;
        os << xll::String("hello") << " " << xll::String("world");
        REQUIRE(os.str() == "hello world");
    }
}

// =============================================================================
// TRIM TESTS
// =============================================================================

TEST_CASE("String - trim()", "[xll::String][trim]")
{
    SECTION("Trims leading spaces") {
        REQUIRE(xll::trim(xll::String("   hello")) == "hello");
    }

    SECTION("Trims trailing spaces") {
        REQUIRE(xll::trim(xll::String("hello   ")) == "hello");
    }

    SECTION("Trims both ends") {
        REQUIRE(xll::trim(xll::String("   hello   ")) == "hello");
    }

    SECTION("No-op on already-trimmed string") {
        REQUIRE(xll::trim(xll::String("hello")) == "hello");
    }

    SECTION("All-whitespace string becomes empty") {
        REQUIRE(xll::trim(xll::String("   ")).empty());
    }

    SECTION("Empty string remains empty") {
        REQUIRE(xll::trim(xll::String()).empty());
    }

    SECTION("Trims tabs and newlines") {
        REQUIRE(xll::trim(xll::String("\t\nhello\n\t")) == "hello");
    }

    SECTION("Interior whitespace is preserved") {
        REQUIRE(xll::trim(xll::String("  hello world  ")) == "hello world");
    }
}

// =============================================================================
// TO_UPPER TESTS
// =============================================================================

TEST_CASE("String - to_upper()", "[xll::String][to_upper]")
{
    SECTION("Uppercases ASCII letters") {
        REQUIRE(xll::to_upper(xll::String("hello")) == "HELLO");
    }

    SECTION("Already uppercase is unchanged") {
        REQUIRE(xll::to_upper(xll::String("HELLO")) == "HELLO");
    }

    SECTION("Mixed case") {
        REQUIRE(xll::to_upper(xll::String("Hello World")) == "HELLO WORLD");
    }

    SECTION("Empty string") {
        REQUIRE(xll::to_upper(xll::String()).empty());
    }

    SECTION("Non-alpha characters unchanged") {
        REQUIRE(xll::to_upper(xll::String("hello 123!")) == "HELLO 123!");
    }

    SECTION("Does not modify original") {
        xll::String s("hello");
        auto r = xll::to_upper(s);
        REQUIRE(s == "hello");    // original unchanged
        REQUIRE(r == "HELLO");
    }
}

// =============================================================================
// SWAP TESTS
// =============================================================================

TEST_CASE("String - Swap", "[xll::String][swap]")
{
    SECTION("Swap exchanges content") {
        xll::String a("hello");
        xll::String b("world");
        XCHAR* a_ptr = a.val.str;
        XCHAR* b_ptr = b.val.str;
        using std::swap;
        swap(a, b);
        REQUIRE(a == "world");
        REQUIRE(b == "hello");
        REQUIRE(a.val.str == b_ptr);    // pointers were exchanged
        REQUIRE(b.val.str == a_ptr);
        REQUIRE(a.xltype == xltypeStr);
        REQUIRE(b.xltype == xltypeStr);
    }

    SECTION("Swap with empty string") {
        xll::String a("hello");
        xll::String b;
        using std::swap;
        swap(a, b);
        REQUIRE(a.empty());
        REQUIRE(b == "hello");
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("String - Validity and Layout", "[xll::String][validity][layout]")
{
    SECTION("is_valid() is true for all constructed objects") {
        REQUIRE(xll::String("hello").is_valid());
        REQUIRE(xll::String().is_valid());
        REQUIRE(xll::String("").is_valid());
    }

    SECTION("is_valid() reflects xltype") {
        xll::String s("hello");
        REQUIRE(s.is_valid());
        s.xltype = xltypeNil;
        REQUIRE_FALSE(s.is_valid());
        s.xltype = xltypeStr;    // restore before destructor
    }

    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::String) == sizeof(XLOPER12));
    }
}

// =============================================================================
// EXCEL LENGTH LIMIT TESTS
// =============================================================================

TEST_CASE("String - Excel Length Limit", "[xll::String][limits]")
{
    SECTION("String at exactly 65535 characters is accepted") {
        std::string s(65535, 'a');
        REQUIRE_NOTHROW(xll::String(s));
        xll::String xs(s);
        REQUIRE(xs.size() == 65535);
    }

    SECTION("String exceeding 65535 characters throws std::length_error") {
        std::string s(65536, 'a');
        REQUIRE_THROWS_AS(xll::String(s), std::length_error);
    }
}
