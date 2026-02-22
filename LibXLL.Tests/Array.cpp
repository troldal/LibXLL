//
// Created by kenne on 25/03/2025.
//

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Array.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Number.hpp"
#include "../Types/String.hpp"

#include <deque>
#include <list>
#include <ranges>
#include <vector>

using NumArr = xll::Array<xll::Number>;
using StrArr = xll::Array<xll::String>;

// =============================================================================
// Static assertions
// =============================================================================

static_assert(sizeof(NumArr) == sizeof(XLOPER12));
static_assert(alignof(NumArr) == alignof(XLOPER12));

// =============================================================================
// Default constructor
// =============================================================================

TEST_CASE("Array - Default construction", "[xll::Array][construction]")
{
    NumArr arr;
    REQUIRE(arr.xltype == xltypeMulti);
    REQUIRE(arr.val.array.lparray == nullptr);
    REQUIRE(arr.val.array.rows == 0);
    REQUIRE(arr.val.array.columns == 0);
    REQUIRE(arr.rows() == 0);
    REQUIRE(arr.cols() == 0);
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.empty());
}

// =============================================================================
// rows/cols/fill constructor
// =============================================================================

TEST_CASE("Array - rows/cols constructor", "[xll::Array][construction]")
{
    SECTION("2x3 default-fill") {
        NumArr arr(2, 3);
        REQUIRE(arr.xltype == xltypeMulti);
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        REQUIRE(arr.size() == 6);
        REQUIRE(!arr.empty());
        REQUIRE(arr.val.array.lparray != nullptr);
        for (size_t i = 0; i < arr.size(); ++i)
            REQUIRE(static_cast<double>(arr[i]) == 0.0);
    }

    SECTION("2x3 with explicit fill") {
        NumArr arr(2, 3, xll::Number(7.0));
        for (size_t i = 0; i < arr.size(); ++i)
            REQUIRE(static_cast<double>(arr[i]) == 7.0);
    }

    SECTION("0 rows produces empty array") {
        NumArr arr(0, 3);
        REQUIRE(arr.empty());
        REQUIRE(arr.val.array.lparray == nullptr);
    }

    SECTION("0 cols produces empty array") {
        NumArr arr(2, 0);
        REQUIRE(arr.empty());
        REQUIRE(arr.val.array.lparray == nullptr);
    }

    SECTION("1x1") {
        NumArr arr(1, 1, xll::Number(42.0));
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 1);
        REQUIRE(arr.size() == 1);
        REQUIRE(static_cast<double>(arr[0]) == 42.0);
    }
}

// =============================================================================
// initializer_list<TValue> constructors
// =============================================================================

TEST_CASE("Array - initializer_list<TValue> constructors", "[xll::Array][construction]")
{
    SECTION("Default shape (Horizontal), auto-fit") {
        NumArr arr { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) };
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 3);
        REQUIRE(static_cast<double>(arr[0]) == 1.0);
        REQUIRE(static_cast<double>(arr[1]) == 2.0);
        REQUIRE(static_cast<double>(arr[2]) == 3.0);
    }

    SECTION("Horizontal{}, auto-fit") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) }, NumArr::Horizontal{});
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 2);
    }

    SECTION("Horizontal(4), padded with default fill") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) }, NumArr::Horizontal(4));
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 4);
        REQUIRE(static_cast<double>(arr[2]) == 0.0);
        REQUIRE(static_cast<double>(arr[3]) == 0.0);
    }

    SECTION("Horizontal(4), padded with custom fill") {
        NumArr arr({ xll::Number(1.0) }, NumArr::Horizontal(4), xll::Number(-1.0));
        REQUIRE(arr.cols() == 4);
        REQUIRE(static_cast<double>(arr[1]) == -1.0);
        REQUIRE(static_cast<double>(arr[3]) == -1.0);
    }

    SECTION("Vertical{}, auto-fit") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) }, NumArr::Vertical{});
        REQUIRE(arr.rows() == 3);
        REQUIRE(arr.cols() == 1);
    }

    SECTION("Vertical(5), padded") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) }, NumArr::Vertical(5), xll::Number(9.0));
        REQUIRE(arr.rows() == 5);
        REQUIRE(arr.cols() == 1);
        REQUIRE(static_cast<double>(arr[2]) == 9.0);
        REQUIRE(static_cast<double>(arr[4]) == 9.0);
    }

    SECTION("TwoDimensional(2,3), exact fit") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0),
                     xll::Number(4.0), xll::Number(5.0), xll::Number(6.0) },
                   NumArr::TwoDimensional(2, 3));
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        REQUIRE(arr.size() == 6);
        REQUIRE(static_cast<double>(arr[0]) == 1.0);
        REQUIRE(static_cast<double>(arr[5]) == 6.0);
    }

    SECTION("TwoDimensional(2,3), padded with custom fill") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) },
                   NumArr::TwoDimensional(2, 3), xll::Number(-1.0));
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        REQUIRE(static_cast<double>(arr[2]) == -1.0);
        REQUIRE(static_cast<double>(arr[5]) == -1.0);
    }

    SECTION("Empty list, Horizontal{} produces empty array") {
        NumArr arr({}, NumArr::Horizontal{});
        REQUIRE(arr.empty());
    }

    SECTION("Horizontal(3), empty list, all fill") {
        NumArr arr({}, NumArr::Horizontal(3), xll::Number(5.0));
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 3);
        for (size_t i = 0; i < 3; ++i)
            REQUIRE(static_cast<double>(arr[i]) == 5.0);
    }

    SECTION("Exception: size smaller than list") {
        REQUIRE_THROWS_AS(
            (NumArr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) }, NumArr::Horizontal(2))),
            std::out_of_range);
    }

    SECTION("Exception: TwoDimensional size smaller than list") {
        REQUIRE_THROWS_AS(
            (NumArr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) }, NumArr::TwoDimensional(1, 2))),
            std::out_of_range);
    }
}

// =============================================================================
// range<TValue> constructors
// =============================================================================

TEST_CASE("Array - range<TValue> constructors", "[xll::Array][construction]")
{
    std::vector<xll::Number> vec { xll::Number(1.0), xll::Number(2.0),
                                   xll::Number(3.0), xll::Number(4.0) };

    SECTION("std::vector, Horizontal{} auto-fit") {
        NumArr arr(vec, NumArr::Horizontal{});
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 4);
        REQUIRE(static_cast<double>(arr[0]) == 1.0);
        REQUIRE(static_cast<double>(arr[3]) == 4.0);
    }

    SECTION("std::vector, Vertical(6), padded") {
        NumArr arr(vec, NumArr::Vertical(6), xll::Number(0.0));
        REQUIRE(arr.rows() == 6);
        REQUIRE(arr.cols() == 1);
        REQUIRE(static_cast<double>(arr[4]) == 0.0);
        REQUIRE(static_cast<double>(arr[5]) == 0.0);
    }

    SECTION("std::deque, TwoDimensional(2,3), padded") {
        std::deque<xll::Number> deq { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) };
        NumArr arr(deq, NumArr::TwoDimensional(2, 3), xll::Number(-1.0));
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        REQUIRE(static_cast<double>(arr[3]) == -1.0);
    }

    SECTION("transform_view, Horizontal{}") {
        auto squares = vec | std::views::transform([](xll::Number n) {
            return xll::Number(static_cast<double>(n) * static_cast<double>(n));
        });
        NumArr arr(squares, NumArr::Horizontal{});
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 4);
        REQUIRE(static_cast<double>(arr[0]) == 1.0);
        REQUIRE(static_cast<double>(arr[1]) == 4.0);
        REQUIRE(static_cast<double>(arr[2]) == 9.0);
        REQUIRE(static_cast<double>(arr[3]) == 16.0);
    }

    SECTION("Empty range produces empty array") {
        std::vector<xll::Number> empty_vec;
        NumArr arr(empty_vec, NumArr::Horizontal{});
        REQUIRE(arr.empty());
    }

    SECTION("Exception: range larger than explicit size") {
        REQUIRE_THROWS_AS(
            NumArr(vec, NumArr::Horizontal(2)),
            std::out_of_range);
    }
}

// =============================================================================
// Converting constructors (U -> TValue)
// =============================================================================

TEST_CASE("Array - converting constructors", "[xll::Array][construction]")
{
    SECTION("initializer_list<double>, default Horizontal") {
        NumArr arr { 1.0, 2.0, 3.0 };
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 3);
        REQUIRE(static_cast<double>(arr[0]) == 1.0);
        REQUIRE(static_cast<double>(arr[2]) == 3.0);
    }

    SECTION("initializer_list<double>, Vertical{}") {
        NumArr arr({ 1.0, 2.0, 3.0 }, NumArr::Vertical{});
        REQUIRE(arr.rows() == 3);
        REQUIRE(arr.cols() == 1);
    }

    SECTION("initializer_list<double>, TwoDimensional(2,3), padded") {
        NumArr arr({ 1.0, 2.0, 3.0 }, NumArr::TwoDimensional(2, 3), xll::Number(0.0));
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        REQUIRE(static_cast<double>(arr[3]) == 0.0);
    }

    SECTION("initializer_list<const char*>, Horizontal{}") {
        StrArr arr({ "foo", "bar", "baz" }, StrArr::Horizontal{});
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 3);
        REQUIRE(std::string(arr[0]) == "foo");
        REQUIRE(std::string(arr[2]) == "baz");
    }

    SECTION("std::vector<double>, Horizontal{}") {
        std::vector<double> v { 10.0, 20.0, 30.0 };
        NumArr arr(v, NumArr::Horizontal{});
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 3);
        REQUIRE(static_cast<double>(arr[1]) == 20.0);
    }

    SECTION("std::vector<double>, TwoDimensional(2,2), padded") {
        std::vector<double> v { 1.0, 2.0 };
        NumArr arr(v, NumArr::TwoDimensional(2, 2), xll::Number(-1.0));
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 2);
        REQUIRE(static_cast<double>(arr[2]) == -1.0);
        REQUIRE(static_cast<double>(arr[3]) == -1.0);
    }

    SECTION("std::deque<double>, Vertical{}") {
        std::deque<double> d { 5.0, 10.0, 15.0 };
        NumArr arr(d, NumArr::Vertical{});
        REQUIRE(arr.rows() == 3);
        REQUIRE(arr.cols() == 1);
    }

    SECTION("std::vector<std::string>, TwoDimensional, padded") {
        std::vector<std::string> sv { "A", "B", "C" };
        StrArr arr(sv, StrArr::TwoDimensional(2, 3), xll::String("-"));
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        REQUIRE(std::string(arr[0]) == "A");
        REQUIRE(std::string(arr[3]) == "-");
        REQUIRE(std::string(arr[5]) == "-");
    }
}

// =============================================================================
// Copy constructor
// =============================================================================

TEST_CASE("Array - copy constructor", "[xll::Array][copy]")
{
    SECTION("Copies non-empty array, independent buffer") {
        NumArr src({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        NumArr dst(src);

        REQUIRE(dst.rows() == src.rows());
        REQUIRE(dst.cols() == src.cols());
        REQUIRE(dst.val.array.lparray != src.val.array.lparray);

        for (size_t i = 0; i < src.size(); ++i)
            REQUIRE(static_cast<double>(dst[i]) == static_cast<double>(src[i]));
    }

    SECTION("Mutation of copy does not affect source") {
        NumArr src({ xll::Number(1.0), xll::Number(2.0) });
        NumArr dst(src);
        dst[0] = xll::Number(99.0);
        REQUIRE(static_cast<double>(src[0]) == 1.0);
    }

    SECTION("Copy of empty array stays empty") {
        NumArr src;
        NumArr dst(src);
        REQUIRE(dst.empty());
        REQUIRE(dst.val.array.lparray == nullptr);
    }

    SECTION("Copy preserves xltype") {
        NumArr src({ xll::Number(1.0) });
        NumArr dst(src);
        REQUIRE(dst.xltype == xltypeMulti);
    }
}

// =============================================================================
// Move constructor
// =============================================================================

TEST_CASE("Array - move constructor", "[xll::Array][move]")
{
    SECTION("Steals buffer from source") {
        NumArr src({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        XLOPER12* original_ptr = src.val.array.lparray;

        NumArr dst(std::move(src));

        REQUIRE(dst.val.array.lparray == original_ptr);
        REQUIRE(dst.rows() == 1);
        REQUIRE(dst.cols() == 3);
        REQUIRE(src.val.array.lparray == nullptr);
        REQUIRE(src.rows() == 0);
        REQUIRE(src.cols() == 0);
    }

    SECTION("Moved-from array is empty and valid") {
        NumArr src({ xll::Number(5.0) });
        NumArr dst(std::move(src));
        REQUIRE(src.empty());
        REQUIRE(src.xltype == xltypeMulti);
    }

    SECTION("Move of empty array stays empty") {
        NumArr src;
        NumArr dst(std::move(src));
        REQUIRE(dst.empty());
    }
}

// =============================================================================
// Copy assignment
// =============================================================================

TEST_CASE("Array - copy assignment", "[xll::Array][copy]")
{
    SECTION("Deep copy, independent buffer") {
        NumArr src({ xll::Number(1.0), xll::Number(2.0) });
        NumArr dst;
        dst = src;

        REQUIRE(dst.rows() == src.rows());
        REQUIRE(dst.cols() == src.cols());
        REQUIRE(dst.val.array.lparray != src.val.array.lparray);
        for (size_t i = 0; i < src.size(); ++i)
            REQUIRE(static_cast<double>(dst[i]) == static_cast<double>(src[i]));
    }

    SECTION("Self-assignment is safe") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) });
        arr = arr;
        REQUIRE(arr.size() == 2);
        REQUIRE(static_cast<double>(arr[0]) == 1.0);
    }

    SECTION("Assignment replaces existing content") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        NumArr other({ xll::Number(9.0) });
        arr = other;
        REQUIRE(arr.size() == 1);
        REQUIRE(static_cast<double>(arr[0]) == 9.0);
    }
}

// =============================================================================
// Move assignment
// =============================================================================

TEST_CASE("Array - move assignment", "[xll::Array][move]")
{
    SECTION("Steals buffer") {
        NumArr src({ xll::Number(1.0), xll::Number(2.0) });
        XLOPER12* original_ptr = src.val.array.lparray;
        NumArr dst;
        dst = std::move(src);

        REQUIRE(dst.val.array.lparray == original_ptr);
        REQUIRE(dst.size() == 2);
        REQUIRE(src.val.array.lparray == nullptr);
        REQUIRE(src.empty());
    }

    SECTION("Self-move is safe") {
        NumArr arr({ xll::Number(1.0) });
        arr = std::move(arr);
        REQUIRE(arr.xltype == xltypeMulti);
    }

    SECTION("Move assignment replaces existing content") {
        NumArr dst({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        NumArr src({ xll::Number(7.0), xll::Number(8.0) });
        dst = std::move(src);
        REQUIRE(dst.size() == 2);
        REQUIRE(static_cast<double>(dst[0]) == 7.0);
    }
}

// =============================================================================
// Missing assignment
// =============================================================================

TEST_CASE("Array - Missing assignment", "[xll::Array]")
{
    SECTION("Resets non-empty array to empty") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) });
        arr = xll::Missing{};
        REQUIRE(arr.xltype == xltypeMulti);
        REQUIRE(arr.empty());
        REQUIRE(arr.val.array.lparray == nullptr);
    }

    SECTION("Resets already-empty array safely") {
        NumArr arr;
        arr = xll::Missing{};
        REQUIRE(arr.empty());
    }
}

// =============================================================================
// shape()
// =============================================================================

TEST_CASE("Array - shape()", "[xll::Array][shape]")
{
    SECTION("Empty") {
        NumArr arr;
        REQUIRE(arr.shape().is<NumArr::Empty>());
    }

    SECTION("Singular") {
        NumArr arr(1, 1, xll::Number(1.0));
        REQUIRE(arr.shape().is<NumArr::Singular>());
    }

    SECTION("Horizontal") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) }, NumArr::Horizontal{});
        REQUIRE(arr.shape().is<NumArr::Horizontal>());
    }

    SECTION("Vertical") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) }, NumArr::Vertical{});
        REQUIRE(arr.shape().is<NumArr::Vertical>());
    }

    SECTION("TwoDimensional") {
        NumArr arr(2, 3);
        REQUIRE(arr.shape().is<NumArr::TwoDimensional>());
    }

    SECTION("Horizontal carries size") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) }, NumArr::Horizontal{});
        arr.shape().visit(fxt::overload{
            [](const NumArr::Horizontal& s) { REQUIRE(s.size == 3); },
            [](const auto&) { FAIL("Unexpected shape"); }
        });
    }

    SECTION("Vertical carries size") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0) }, NumArr::Vertical{});
        arr.shape().visit(fxt::overload{
            [](const NumArr::Vertical& s) { REQUIRE(s.size == 2); },
            [](const auto&) { FAIL("Unexpected shape"); }
        });
    }

    SECTION("TwoDimensional carries rows and cols") {
        NumArr arr(3, 4);
        arr.shape().visit(fxt::overload{
            [](const NumArr::TwoDimensional& s) {
                REQUIRE(s.rows == 3);
                REQUIRE(s.cols == 4);
            },
            [](const auto&) { FAIL("Unexpected shape"); }
        });
    }
}

// =============================================================================
// rows(), cols(), size(), empty()
// =============================================================================

TEST_CASE("Array - accessors", "[xll::Array][accessors]")
{
    SECTION("rows/cols/size on 3x4") {
        NumArr arr(3, 4);
        REQUIRE(arr.rows() == 3);
        REQUIRE(arr.cols() == 4);
        REQUIRE(arr.size() == 12);
        REQUIRE(!arr.empty());
    }

    SECTION("empty on default") {
        NumArr arr;
        REQUIRE(arr.rows() == 0);
        REQUIRE(arr.cols() == 0);
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.empty());
    }
}

// =============================================================================
// reshape()
// =============================================================================

TEST_CASE("Array - reshape()", "[xll::Array][reshape]")
{
    SECTION("Valid reshape preserves elements") {
        NumArr arr({ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 });
        arr.reshape(2, 3);
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
        for (size_t i = 0; i < 6; ++i)
            REQUIRE(static_cast<double>(arr[i]) == static_cast<double>(i + 1));
    }

    SECTION("Reshape to same dimensions is a no-op") {
        NumArr arr(2, 3);
        arr.reshape(2, 3);
        REQUIRE(arr.rows() == 2);
        REQUIRE(arr.cols() == 3);
    }

    SECTION("Reshape to 1xN") {
        NumArr arr(2, 3);
        arr.reshape(1, 6);
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 6);
    }

    SECTION("Exception: element count mismatch") {
        NumArr arr(2, 3);
        REQUIRE_THROWS_AS(arr.reshape(2, 4), std::invalid_argument);
    }
}

// =============================================================================
// begin() / end()
// =============================================================================

TEST_CASE("Array - iterators", "[xll::Array][iterators]")
{
    SECTION("Range-based for loop") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        double sum = 0.0;
        for (const auto& v : arr) sum += static_cast<double>(v);
        REQUIRE(sum == 6.0);
    }

    SECTION("Mutating via range-based for") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        for (auto& v : arr) v = xll::Number(static_cast<double>(v) * 2.0);
        REQUIRE(static_cast<double>(arr[0]) == 2.0);
        REQUIRE(static_cast<double>(arr[1]) == 4.0);
        REQUIRE(static_cast<double>(arr[2]) == 6.0);
    }

    SECTION("begin() == end() for empty array") {
        NumArr arr;
        REQUIRE(arr.begin() == arr.end());
    }

    SECTION("std::distance matches size()") {
        NumArr arr(2, 3);
        REQUIRE(static_cast<size_t>(std::distance(arr.begin(), arr.end())) == arr.size());
    }

    SECTION("const begin/end") {
        const NumArr arr({ xll::Number(1.0), xll::Number(2.0) });
        REQUIRE(static_cast<double>(*arr.begin()) == 1.0);
        REQUIRE(std::distance(arr.begin(), arr.end()) == 2);
    }
}

// =============================================================================
// operator[](index)
// =============================================================================

TEST_CASE("Array - operator[](index)", "[xll::Array][indexing]")
{
    SECTION("Read access") {
        NumArr arr({ xll::Number(10.0), xll::Number(20.0), xll::Number(30.0) });
        REQUIRE(static_cast<double>(arr[0]) == 10.0);
        REQUIRE(static_cast<double>(arr[1]) == 20.0);
        REQUIRE(static_cast<double>(arr[2]) == 30.0);
    }

    SECTION("Write access") {
        NumArr arr(1, 3);
        arr[1] = xll::Number(99.0);
        REQUIRE(static_cast<double>(arr[1]) == 99.0);
    }

    SECTION("Const read access") {
        const NumArr arr({ xll::Number(5.0), xll::Number(6.0) });
        REQUIRE(static_cast<double>(arr[0]) == 5.0);
    }

    SECTION("Exception: index out of range") {
        NumArr arr({ xll::Number(1.0) });
        REQUIRE_THROWS_AS(arr[1], std::out_of_range);
    }
}

// =============================================================================
// operator[](row, col)
// =============================================================================

TEST_CASE("Array - operator[](row, col)", "[xll::Array][indexing]")
{
    SECTION("Read 2D access") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0),
                     xll::Number(4.0), xll::Number(5.0), xll::Number(6.0) },
                   NumArr::TwoDimensional(2, 3));
        REQUIRE(static_cast<double>(arr[0, 0]) == 1.0);
        REQUIRE(static_cast<double>(arr[0, 2]) == 3.0);
        REQUIRE(static_cast<double>(arr[1, 0]) == 4.0);
        REQUIRE(static_cast<double>(arr[1, 2]) == 6.0);
    }

    SECTION("Write 2D access") {
        NumArr arr(2, 3);
        arr[1, 2] = xll::Number(42.0);
        REQUIRE(static_cast<double>(arr[1, 2]) == 42.0);
    }

    SECTION("Const 2D read") {
        const NumArr arr({ xll::Number(1.0), xll::Number(2.0),
                           xll::Number(3.0), xll::Number(4.0) },
                         NumArr::TwoDimensional(2, 2));
        REQUIRE(static_cast<double>(arr[1, 1]) == 4.0);
    }

    SECTION("Exception: row out of range") {
        NumArr arr(2, 2);
        REQUIRE_THROWS_AS((arr[2, 0]), std::out_of_range);
    }

    SECTION("Exception: col out of range") {
        NumArr arr(2, 2);
        REQUIRE_THROWS_AS((arr[0, 2]), std::out_of_range);
    }
}

// =============================================================================
// Conversion operator to container
// =============================================================================

TEST_CASE("Array - container conversion operator", "[xll::Array][conversion]")
{
    SECTION("Implicit conversion to std::vector<xll::Number>") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        std::vector<xll::Number> v = arr;
        REQUIRE(v.size() == 3);
        REQUIRE(static_cast<double>(v[0]) == 1.0);
        REQUIRE(static_cast<double>(v[2]) == 3.0);
    }

    SECTION("Implicit conversion to std::deque<xll::Number>") {
        NumArr arr({ xll::Number(4.0), xll::Number(5.0) });
        std::deque<xll::Number> d = arr;
        REQUIRE(d.size() == 2);
        REQUIRE(static_cast<double>(d[1]) == 5.0);
    }
}

// =============================================================================
// to<TContainer, TElem>() and to<TContainer>()
// =============================================================================

TEST_CASE("Array - to<>()", "[xll::Array][conversion]")
{
    SECTION("to<std::vector>() returns vector of TValue") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        auto v = arr.to<std::vector>();
        REQUIRE(v.size() == 3);
        REQUIRE(static_cast<double>(v[0]) == 1.0);
    }

    SECTION("to<std::vector, double>() converts elements") {
        NumArr arr({ xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) });
        auto v = arr.to<std::vector, double>();
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == 1.0);
        REQUIRE(v[2] == 3.0);
    }

    SECTION("to<std::deque>() returns deque of TValue") {
        NumArr arr({ xll::Number(7.0), xll::Number(8.0) });
        auto d = arr.to<std::deque>();
        REQUIRE(d.size() == 2);
        REQUIRE(static_cast<double>(d[1]) == 8.0);
    }

    SECTION("to<std::vector, double>() on String array") {
        StrArr arr({ xll::String("hello"), xll::String("world") });
        auto v = arr.to<std::vector, std::string>();
        REQUIRE(v.size() == 2);
        REQUIRE(v[0] == "hello");
        REQUIRE(v[1] == "world");
    }
}

// =============================================================================
// String array
// =============================================================================

TEST_CASE("Array - String element type", "[xll::Array][String]")
{
    SECTION("Construction and access") {
        StrArr arr({ xll::String("alpha"), xll::String("beta"), xll::String("gamma") });
        REQUIRE(arr.rows() == 1);
        REQUIRE(arr.cols() == 3);
        REQUIRE(std::string(arr[0]) == "alpha");
        REQUIRE(std::string(arr[2]) == "gamma");
    }

    SECTION("Copy constructor is deep") {
        StrArr src({ xll::String("x"), xll::String("y") });
        StrArr dst(src);
        dst[0] = xll::String("z");
        REQUIRE(std::string(src[0]) == "x");
        REQUIRE(std::string(dst[0]) == "z");
    }

    SECTION("TwoDimensional with padding") {
        StrArr arr({ xll::String("A"), xll::String("B") },
                   StrArr::TwoDimensional(2, 2), xll::String("?"));
        REQUIRE(std::string(arr[0]) == "A");
        REQUIRE(std::string(arr[1]) == "B");
        REQUIRE(std::string(arr[2]) == "?");
        REQUIRE(std::string(arr[3]) == "?");
    }
}

