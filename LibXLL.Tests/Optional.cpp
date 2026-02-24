//
// Tests for xll::Optional, xll::Nullopt, and xll::None
//

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Optional.hpp"
#include "../Types/Number.hpp"
#include "../Types/Int.hpp"
#include "../Types/Bool.hpp"
#include "../Types/String.hpp"
#include "../Types/Error.hpp"

// =============================================================================
// CONSTRUCTION
// =============================================================================

TEST_CASE("Optional - Default Construction (disengaged)", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE_FALSE(opt.has_value());
    REQUIRE_FALSE(static_cast<bool>(opt));
    REQUIRE(opt.xltype == xltypeNil);
}

TEST_CASE("Optional - Construct from xll::None", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt = xll::None;
    REQUIRE_FALSE(opt.has_value());
    REQUIRE(opt == xll::None);
}

TEST_CASE("Optional - Construct from TValue (copy)", "[xll::Optional][construction]")
{
    xll::Number n = 42.0;
    xll::Optional<xll::Number> opt(n);
    REQUIRE(opt.has_value());
    REQUIRE(opt.xltype == xltypeNum);
    REQUIRE(opt.value() == 42.0);
}

TEST_CASE("Optional - Construct from TValue (move)", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt(xll::Number(3.14));
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 3.14);
}

TEST_CASE("Optional - Construct from convertible type (double -> Number)", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt = 99.0;
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 99.0);
}

TEST_CASE("Optional - Construct Optional<String>", "[xll::Optional][construction]")
{
    xll::Optional<xll::String> opt = xll::String("hello");
    REQUIRE(opt.has_value());
    REQUIRE(opt.xltype == xltypeStr);
}

// =============================================================================
// COPY / MOVE
// =============================================================================

TEST_CASE("Optional - Copy constructor (engaged)", "[xll::Optional][copy]")
{
    xll::Optional<xll::Number> a = 7.0;
    xll::Optional<xll::Number> b = a;
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 7.0);
}

TEST_CASE("Optional - Copy constructor (disengaged)", "[xll::Optional][copy]")
{
    xll::Optional<xll::Number> a;
    xll::Optional<xll::Number> b = a;
    REQUIRE_FALSE(b.has_value());
}

TEST_CASE("Optional - Move constructor (engaged)", "[xll::Optional][move]")
{
    xll::Optional<xll::Number> a = 5.0;
    xll::Optional<xll::Number> b = std::move(a);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 5.0);
}

TEST_CASE("Optional - Move constructor (disengaged)", "[xll::Optional][move]")
{
    xll::Optional<xll::Number> a;
    xll::Optional<xll::Number> b = std::move(a);
    REQUIRE_FALSE(b.has_value());
}

// =============================================================================
// ASSIGNMENT
// =============================================================================

TEST_CASE("Optional - Assign xll::None (reset)", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> opt = 1.0;
    REQUIRE(opt.has_value());
    opt = xll::None;
    REQUIRE_FALSE(opt.has_value());
    REQUIRE(opt == xll::None);
}

TEST_CASE("Optional - Assign TValue", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> opt;
    opt = xll::Number(2.0);
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 2.0);
}

TEST_CASE("Optional - Copy assignment", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> a = 8.0;
    xll::Optional<xll::Number> b;
    b = a;
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 8.0);
}

TEST_CASE("Optional - Move assignment", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> a = 9.0;
    xll::Optional<xll::Number> b;
    b = std::move(a);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 9.0);
}

TEST_CASE("Optional - Self assignment", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> opt = 3.0;
    opt = opt;   // NOLINT (intentional self-assign)
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 3.0);
}

// =============================================================================
// VALUE ACCESS
// =============================================================================

TEST_CASE("Optional - value() throws on disengaged", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE_THROWS_AS(opt.value(), std::bad_optional_access);
}

TEST_CASE("Optional - operator* (engaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt = 11.0;
    REQUIRE(*opt == 11.0);
}

TEST_CASE("Optional - operator-> (engaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt = 12.0;
    REQUIRE(opt->val.num == 12.0);
}

TEST_CASE("Optional - value_or (engaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt = 5.0;
    REQUIRE(opt.value_or(xll::Number(0.0)) == 5.0);
}

TEST_CASE("Optional - value_or (disengaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE(opt.value_or(xll::Number(-1.0)) == -1.0);
}

// =============================================================================
// MODIFIERS
// =============================================================================

TEST_CASE("Optional - reset()", "[xll::Optional][modifiers]")
{
    xll::Optional<xll::Number> opt = 1.0;
    opt.reset();
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Optional - emplace()", "[xll::Optional][modifiers]")
{
    xll::Optional<xll::Number> opt;
    auto& ref = opt.emplace(xll::Number(77.0));
    REQUIRE(opt.has_value());
    REQUIRE(ref == 77.0);
}

TEST_CASE("Optional - emplace replaces existing value", "[xll::Optional][modifiers]")
{
    xll::Optional<xll::Number> opt = 1.0;
    opt.emplace(xll::Number(2.0));
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 2.0);
}

// =============================================================================
// SWAP
// =============================================================================

TEST_CASE("Optional - swap (both engaged)", "[xll::Optional][swap]")
{
    xll::Optional<xll::Number> a = 1.0;
    xll::Optional<xll::Number> b = 2.0;
    a.swap(b);
    REQUIRE(a.value() == 2.0);
    REQUIRE(b.value() == 1.0);
}

TEST_CASE("Optional - swap (engaged with disengaged)", "[xll::Optional][swap]")
{
    xll::Optional<xll::Number> a = 3.0;
    xll::Optional<xll::Number> b;
    a.swap(b);
    REQUIRE_FALSE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 3.0);
}

TEST_CASE("Optional - non-member swap", "[xll::Optional][swap]")
{
    xll::Optional<xll::Number> a = 4.0;
    xll::Optional<xll::Number> b = 5.0;
    swap(a, b);
    REQUIRE(a.value() == 5.0);
    REQUIRE(b.value() == 4.0);
}

// =============================================================================
// EQUALITY
// =============================================================================

TEST_CASE("Optional - equality: two disengaged", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a;
    xll::Optional<xll::Number> b;
    REQUIRE(a == b);
}

TEST_CASE("Optional - equality: engaged with same value", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a = 6.0;
    xll::Optional<xll::Number> b = 6.0;
    REQUIRE(a == b);
}

TEST_CASE("Optional - equality: engaged with different value", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a = 6.0;
    xll::Optional<xll::Number> b = 7.0;
    REQUIRE_FALSE(a == b);
}

TEST_CASE("Optional - equality: engaged vs disengaged", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a = 1.0;
    xll::Optional<xll::Number> b;
    REQUIRE_FALSE(a == b);
}

TEST_CASE("Optional - equality with TValue", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> opt = 3.0;
    REQUIRE(opt == xll::Number(3.0));
    REQUIRE_FALSE(opt == xll::Number(4.0));
}

TEST_CASE("Optional - equality with xll::None", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> engaged = 1.0;
    xll::Optional<xll::Number> empty;
    REQUIRE(empty == xll::None);
    REQUIRE_FALSE(engaged == xll::None);
}

// =============================================================================
// MONADIC OPERATIONS
// =============================================================================

TEST_CASE("Optional - transform (engaged)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 10.0;
    auto result = opt.transform([](xll::Number& n) { return xll::Number(n.val.num * 2.0); });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 20.0);
}

TEST_CASE("Optional - transform (disengaged)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt;
    auto result = opt.transform([](xll::Number& n) { return xll::Number(n.val.num * 2.0); });
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Optional - and_then (engaged, returns value)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 4.0;
    auto result = opt.and_then([](xll::Number& n) -> xll::Optional<xll::Number> {
        if (n.val.num == 0.0) return xll::None;
        return xll::Number(1.0 / n.val.num);
    });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0.25);
}

TEST_CASE("Optional - and_then (engaged, returns empty)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 0.0;
    auto result = opt.and_then([](xll::Number& n) -> xll::Optional<xll::Number> {
        if (n.val.num == 0.0) return xll::None;
        return xll::Number(1.0 / n.val.num);
    });
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Optional - and_then (disengaged)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt;
    bool called = false;
    auto result = opt.and_then([&called](xll::Number& n) -> xll::Optional<xll::Number> {
        called = true;
        return n;
    });
    REQUIRE_FALSE(called);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Optional - or_else (disengaged, provides fallback)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt;
    auto result = opt.or_else([]() -> xll::Optional<xll::Number> { return xll::Number(99.0); });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 99.0);
}

TEST_CASE("Optional - or_else (engaged, skips fallback)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 5.0;
    bool called = false;
    auto result = opt.or_else([&called]() -> xll::Optional<xll::Number> {
        called = true;
        return xll::None;
    });
    REQUIRE_FALSE(called);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 5.0);
}

// =============================================================================
// PIPE OPERATOR
// =============================================================================

TEST_CASE("Optional - pipe operator with opt_transform", "[xll::Optional][pipe]")
{
    xll::Optional<xll::Number> opt = 3.0;
    auto result = opt
        | xll::opt_transform([](xll::Number& n) { return xll::Number(n.val.num + 1.0); })
        | xll::opt_transform([](xll::Number& n) { return xll::Number(n.val.num * 2.0); });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 8.0);   // (3+1)*2
}

TEST_CASE("Optional - pipe operator propagates empty", "[xll::Optional][pipe]")
{
    xll::Optional<xll::Number> opt;
    bool called = false;
    auto result = opt
        | xll::opt_transform([&called](xll::Number& n) {
            called = true;
            return xll::Number(n.val.num * 2.0);
        });
    REQUIRE_FALSE(called);
    REQUIRE_FALSE(result.has_value());
}

// =============================================================================
// XLOPER12 MEMORY LAYOUT
// =============================================================================

TEST_CASE("Optional - sizeof equals XLOPER12", "[xll::Optional][layout]")
{
    REQUIRE(sizeof(xll::Optional<xll::Number>) == sizeof(XLOPER12));
    REQUIRE(sizeof(xll::Optional<xll::String>) == sizeof(XLOPER12));
    REQUIRE(sizeof(xll::Optional<xll::Int>)    == sizeof(XLOPER12));
    REQUIRE(sizeof(xll::Optional<xll::Bool>)   == sizeof(XLOPER12));
}

TEST_CASE("Optional - disengaged xltype is xltypeNil", "[xll::Optional][layout]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE((opt.xltype & ~(xlbitDLLFree | xlbitXLFree)) == xltypeNil);
}

TEST_CASE("Optional - engaged xltype matches TValue", "[xll::Optional][layout]")
{
    xll::Optional<xll::Number> opt = 1.0;
    REQUIRE((opt.xltype & ~(xlbitDLLFree | xlbitXLFree)) == xltypeNum);
}

// =============================================================================
// STRING OPTIONAL (exercises non-trivial destructor path)
// =============================================================================

TEST_CASE("Optional - String value copy/move", "[xll::Optional][string]")
{
    xll::Optional<xll::String> a = xll::String("world");
    REQUIRE(a.has_value());

    xll::Optional<xll::String> b = a;
    REQUIRE(b.has_value());

    xll::Optional<xll::String> c = std::move(a);
    REQUIRE(c.has_value());
}

TEST_CASE("Optional - String reset", "[xll::Optional][string]")
{
    xll::Optional<xll::String> opt = xll::String("test");
    REQUIRE(opt.has_value());
    opt.reset();
    REQUIRE_FALSE(opt.has_value());
}

// =============================================================================
// ITERATOR / RANGE TESTS  (C++26-style single-element range)
// =============================================================================

#include <algorithm>
#include <ranges>
#include <vector>

TEST_CASE("Optional - iterator types satisfy contiguous_iterator", "[xll::Optional][iterator]")
{
    STATIC_REQUIRE(std::contiguous_iterator<xll::Optional<xll::Number>::iterator>);
    STATIC_REQUIRE(std::contiguous_iterator<xll::Optional<xll::Number>::const_iterator>);
    STATIC_REQUIRE(std::random_access_iterator<xll::Optional<xll::Number>::iterator>);
    STATIC_REQUIRE(std::random_access_iterator<xll::Optional<xll::Number>::const_iterator>);
}

TEST_CASE("Optional - range concepts satisfied", "[xll::Optional][iterator]")
{
    STATIC_REQUIRE(std::ranges::range<xll::Optional<xll::Number>>);
    STATIC_REQUIRE(std::ranges::sized_range<xll::Optional<xll::Number>>);
    STATIC_REQUIRE(std::ranges::contiguous_range<xll::Optional<xll::Number>>);
    STATIC_REQUIRE(std::ranges::view<xll::Optional<xll::Number>>);
}

TEST_CASE("Optional - engaged: begin/end span one element", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(42.0)};

    SECTION("size() == 1") {
        REQUIRE(opt.size() == 1u);
        REQUIRE_FALSE(opt.empty());
    }

    SECTION("begin != end") {
        REQUIRE(opt.begin() != opt.end());
    }

    SECTION("dereference begin gives value") {
        REQUIRE(*opt.begin() == 42.0);
    }

    SECTION("pre-increment reaches end") {
        auto it = opt.begin();
        ++it;
        REQUIRE(it == opt.end());
    }

    SECTION("post-increment reaches end") {
        auto it = opt.begin();
        it++;
        REQUIRE(it == opt.end());
    }

    SECTION("operator+ by 1 reaches end") {
        REQUIRE(opt.begin() + 1 == opt.end());
    }

    SECTION("distance is 1") {
        REQUIRE(opt.end() - opt.begin() == 1);
    }

    SECTION("data() is not null and equals &value") {
        REQUIRE(opt.data() != nullptr);
        REQUIRE(opt.data() == &opt.value());
    }
}

TEST_CASE("Optional - disengaged: begin == end", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt;

    SECTION("size() == 0") {
        REQUIRE(opt.size() == 0u);
        REQUIRE(opt.empty());
    }

    SECTION("begin == end") {
        REQUIRE(opt.begin() == opt.end());
    }

    SECTION("data() is nullptr") {
        REQUIRE(opt.data() == nullptr);
    }
}

TEST_CASE("Optional - const iterator on engaged value", "[xll::Optional][iterator]")
{
    const xll::Optional<xll::Number> opt{xll::Number(7.0)};

    REQUIRE(opt.size() == 1u);
    REQUIRE(opt.begin() != opt.end());
    REQUIRE(*opt.begin() == 7.0);
    REQUIRE(opt.cbegin() != opt.cend());
    REQUIRE(*opt.cbegin() == 7.0);
    REQUIRE(opt.data() != nullptr);
}

TEST_CASE("Optional - const iterator on disengaged", "[xll::Optional][iterator]")
{
    const xll::Optional<xll::Number> opt;

    REQUIRE(opt.size() == 0u);
    REQUIRE(opt.begin() == opt.end());
    REQUIRE(opt.cbegin() == opt.cend());
    REQUIRE(opt.data() == nullptr);
}

TEST_CASE("Optional - iterator mutation via begin()", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(1.0)};
    *opt.begin() = xll::Number(99.0);
    REQUIRE(opt.value() == 99.0);
}

TEST_CASE("Optional - range-for loop on engaged value", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(3.14)};

    int count = 0;
    double seen = 0.0;
    for (auto& n : opt) {
        ++count;
        seen = n.val.num;
    }
    REQUIRE(count == 1);
    REQUIRE(seen == Catch::Approx(3.14));
}

TEST_CASE("Optional - range-for loop on disengaged visits nothing", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt;

    int count = 0;
    for ([[maybe_unused]] auto& n : opt)
        ++count;
    REQUIRE(count == 0);
}

TEST_CASE("Optional - range-for loop toggles engaged/disengaged", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(1.0)};

    int engaged_count = 0;
    for ([[maybe_unused]] auto& n : opt) ++engaged_count;
    REQUIRE(engaged_count == 1);

    opt = xll::None;

    int disengaged_count = 0;
    for ([[maybe_unused]] auto& n : opt) ++disengaged_count;
    REQUIRE(disengaged_count == 0);
}

TEST_CASE("Optional - std::ranges::for_each on engaged", "[xll::Optional][iterator][ranges]")
{
    xll::Optional<xll::Number> opt{xll::Number(5.0)};

    double sum = 0.0;
    std::ranges::for_each(opt, [&](xll::Number& n) { sum += n.val.num; });
    REQUIRE(sum == Catch::Approx(5.0));
}

TEST_CASE("Optional - std::ranges::for_each on disengaged visits nothing", "[xll::Optional][iterator][ranges]")
{
    xll::Optional<xll::Number> opt;

    int count = 0;
    std::ranges::for_each(opt, [&](xll::Number&) { ++count; });
    REQUIRE(count == 0);
}

TEST_CASE("Optional - std::ranges::find on engaged", "[xll::Optional][iterator][ranges]")
{
    xll::Optional<xll::Number> opt{xll::Number(42.0)};
    auto it = std::ranges::find(opt, xll::Number(42.0));
    REQUIRE(it != opt.end());
    REQUIRE(*it == 42.0);
}

TEST_CASE("Optional - std::ranges::find on disengaged returns end", "[xll::Optional][iterator][ranges]")
{
    xll::Optional<xll::Number> opt;
    auto it = std::ranges::find(opt, xll::Number(42.0));
    REQUIRE(it == opt.end());
}

TEST_CASE("Optional - std::ranges::distance", "[xll::Optional][iterator][ranges]")
{
    xll::Optional<xll::Number> engaged{xll::Number(1.0)};
    xll::Optional<xll::Number> empty;

    REQUIRE(std::ranges::distance(engaged) == 1);
    REQUIRE(std::ranges::distance(empty)   == 0);
}

TEST_CASE("Optional - collect into vector via iterator pair", "[xll::Optional][iterator][ranges]")
{
    xll::Optional<xll::Number> opt{xll::Number(9.0)};
    xll::Optional<xll::Number> empty;

    std::vector<xll::Number> v1(opt.begin(), opt.end());
    REQUIRE(v1.size() == 1u);
    REQUIRE(v1[0] == 9.0);

    std::vector<xll::Number> v2(empty.begin(), empty.end());
    REQUIRE(v2.empty());
}

TEST_CASE("Optional - views::join flattens vector of Optionals", "[xll::Optional][iterator][ranges]")
{
    std::vector<xll::Optional<xll::Number>> vec;
    vec.emplace_back(xll::Number(1.0));
    vec.emplace_back(xll::None);
    vec.emplace_back(xll::Number(3.0));
    vec.emplace_back(xll::None);
    vec.emplace_back(xll::Number(5.0));

    auto values = vec | std::views::join;
    std::vector<xll::Number> result(values.begin(), values.end());

    REQUIRE(result.size() == 3u);
    REQUIRE(result[0] == 1.0);
    REQUIRE(result[1] == 3.0);
    REQUIRE(result[2] == 5.0);
}

TEST_CASE("Optional - iterator ordering operators", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(1.0)};
    auto begin = opt.begin();
    auto end   = opt.end();

    REQUIRE(begin < end);
    REQUIRE(end   > begin);
    REQUIRE_FALSE(begin > end);
    REQUIRE_FALSE(end < begin);
    REQUIRE(begin <= end);
    REQUIRE(end >= begin);
}

TEST_CASE("Optional - iterator arithmetic", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(7.0)};
    auto it = opt.begin();

    SECTION("begin + 1 == end") {
        REQUIRE(it + 1 == opt.end());
    }
    SECTION("1 + begin == end") {
        REQUIRE(1 + it == opt.end());
    }
    SECTION("end - begin == 1") {
        REQUIRE(opt.end() - opt.begin() == 1);
    }
    SECTION("operator[] on begin") {
        REQUIRE(it[0] == 7.0);
    }
}

TEST_CASE("Optional - String iterator (non-trivial value type)", "[xll::Optional][iterator]")
{
    xll::Optional<xll::String> opt{xll::String("hello")};

    REQUIRE(opt.size() == 1u);
    REQUIRE(opt.begin() != opt.end());
    REQUIRE(*opt.begin() == xll::String("hello"));

    int count = 0;
    for (auto& s : opt) {
        ++count;
        REQUIRE(s == xll::String("hello"));
    }
    REQUIRE(count == 1);
}

TEST_CASE("Optional - enable_view specialisation", "[xll::Optional][iterator][ranges]")
{
    STATIC_REQUIRE(std::ranges::view<xll::Optional<xll::Number>>);
    STATIC_REQUIRE(std::ranges::view<xll::Optional<xll::String>>);
    STATIC_REQUIRE(std::ranges::view<xll::Optional<xll::Int>>);
}

TEST_CASE("Optional - iterator not invalidated by reset then emplace", "[xll::Optional][iterator]")
{
    xll::Optional<xll::Number> opt{xll::Number(1.0)};

    // Disengage — range becomes empty
    opt.reset();
    REQUIRE(opt.begin() == opt.end());

    // Reengage — range contains one element again
    opt.emplace(xll::Number(2.0));
    REQUIRE(opt.begin() != opt.end());
    REQUIRE(*opt.begin() == 2.0);
}

TEST_CASE("Optional - std::ranges::transform view over Optionals", "[xll::Optional][iterator][ranges]")
{
    // Build a vector of OptNumbers, some engaged, some not.
    std::vector<xll::Optional<xll::Number>> opts;
    opts.emplace_back(2.0);
    opts.emplace_back(xll::None);
    opts.emplace_back(4.0);

    // Flatten and double each value
    auto doubled = opts
        | std::views::join
        | std::views::transform([](xll::Number& n) { return n.val.num * 2.0; });

    std::vector<double> result(doubled.begin(), doubled.end());
    REQUIRE(result.size() == 2u);
    REQUIRE(result[0] == Catch::Approx(4.0));
    REQUIRE(result[1] == Catch::Approx(8.0));
}
