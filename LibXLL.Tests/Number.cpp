//
// Created by kenne on 04/04/2025.
//
#include <catch2/catch_test_macros.hpp>
#include <xlcall.hpp>
#include "../Types/Number.hpp"
#include "../Types/Int.hpp"
#include "../Types/Bool.hpp"
#include <limits>

TEST_CASE( "Number Construction", "[xll::Number]" )
{
    // Default construction:
    xll::Number n01;
    REQUIRE(n01 == 0.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 0.0);

    // Construction from double:
    xll::Number n02 = 3.14;
    REQUIRE(n02 == 3.14);
    REQUIRE(n02.xltype == xltypeNum);
    REQUIRE(n02.val.num == 3.14);

    // Construction from int:
    xll::Number n03 = 42;
    REQUIRE(n03 == 42.0);
    REQUIRE(n03.xltype == xltypeNum);
    REQUIRE(n03.val.num == 42.0);

    // Construction from XLOPER12
    auto xl01 = XLOPER12();
    xl01.xltype = xltypeNum;
    xl01.val.num = 2.718;
    xll::Number n04 = xll::Number(xl01);
    REQUIRE(n04 == 2.718);
    REQUIRE(n04.xltype == xltypeNum);
    REQUIRE(n04.val.num == 2.718);

    // Invalid XLOPER12 construction
    auto xl02 = XLOPER12();
    REQUIRE_THROWS(xll::Number(xl02));

    // Copy construction:
    xll::Number n05 = n02;
    REQUIRE(n05 == n02);
    REQUIRE_FALSE(n05 != n02);
    REQUIRE(n05 == 3.14);
    REQUIRE(n05.xltype == xltypeNum);
    REQUIRE(n05.val.num == 3.14);

    // Move construction:
    xll::Number n06 = std::move(n05);
    REQUIRE(n06 == n02);
    REQUIRE_FALSE(n06 != n02);
    REQUIRE(n06 == 3.14);
    REQUIRE(n06.xltype == xltypeNum);
    REQUIRE(n06.val.num == 3.14);

    // Construction from xll::Int
    xll::Number n07 = xll::Int(42);
    REQUIRE(n07 == 42.0);
    REQUIRE(n07.xltype == xltypeNum);
    REQUIRE(n07.val.num == 42.0);

    // Construction from xll::Bool
    xll::Number n08 = xll::Bool(true);
    REQUIRE(n08 == 1.0);
    REQUIRE(n08.xltype == xltypeNum);
    REQUIRE(n08.val.num == 1.0);

    xll::Number n09 = xll::Bool(false);
    REQUIRE(n09 == 0.0);
    REQUIRE(n09.xltype == xltypeNum);
    REQUIRE(n09.val.num == 0.0);

    // Extreme values
    xll::Number n10 = std::numeric_limits<double>::max();
    REQUIRE(n10 == std::numeric_limits<double>::max());
    REQUIRE(n10.xltype == xltypeNum);
    REQUIRE(n10.val.num == std::numeric_limits<double>::max());

    xll::Number n11 = std::numeric_limits<double>::min();
    REQUIRE(n11 == std::numeric_limits<double>::min());
    REQUIRE(n11.xltype == xltypeNum);
    REQUIRE(n11.val.num == std::numeric_limits<double>::min());

    xll::Number invalid;
    invalid.xltype = xltypeNil;

    // Invalid copy construction
    REQUIRE_THROWS(xll::Number(invalid));
}

TEST_CASE( "Number Assignment", "[xll::Number]" )
{
    xll::Number n01;

    // Assignment with double:
    n01 = 3.14;
    REQUIRE(n01 == 3.14);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 3.14);

    // Assignment with int:
    n01 = 42;
    REQUIRE(n01 == 42.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 42.0);

    // Assignment of XLOPER12:
    auto xl01 = XLOPER12();
    xl01.xltype = xltypeNum;
    xl01.val.num = 2.718;
    n01 = xll::Number(xl01);
    REQUIRE(n01 == 2.718);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 2.718);

    xll::Number to_be_copied = 1.618;

    // Copy assignment:
    n01 = to_be_copied;
    REQUIRE(n01 == to_be_copied);
    REQUIRE_FALSE(n01 != to_be_copied);
    REQUIRE(n01 == 1.618);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 1.618);

    xll::Number to_be_moved = 1.414;

    // Move assignment:
    n01 = std::move(to_be_moved);
    REQUIRE(n01 == 1.414);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 1.414);

    // Assignment of xll::Int
    n01 = xll::Int(42);
    REQUIRE(n01 == 42.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 42.0);

    // Assignment of xll::Bool
    n01 = xll::Bool(true);
    REQUIRE(n01 == 1.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 1.0);

    n01 = xll::Bool(false);
    REQUIRE(n01 == 0.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 0.0);

    // Assignment of extreme values
    n01 = std::numeric_limits<double>::max();
    REQUIRE(n01 == std::numeric_limits<double>::max());
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == std::numeric_limits<double>::max());
}

TEST_CASE( "Number Operations", "[xll::Number]" )
{
    xll::Number n01(3.5);
    double n01_ = 3.5;

    // Unary plus
    auto n02 = +n01;
    auto n02_ = +n01_;
    REQUIRE(n02 == n02_);
    REQUIRE(n02 == 3.5);
    REQUIRE(n02.xltype == xltypeNum);
    REQUIRE(n02.val.num == 3.5);

    // Unary minus
    auto n03 = -n01;
    auto n03_ = -n01_;
    REQUIRE(n03 == n03_);
    REQUIRE(n03 == -3.5);
    REQUIRE(n03.xltype == xltypeNum);
    REQUIRE(n03.val.num == -3.5);

    // Addition with xll::Number
    xll::Number n04(2.5);
    n01 = n01 + n04;
    n01_ = n01_ + 2.5;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 6.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 6.0);

    // Addition with double
    n01 = n01 + 1.5;
    n01_ = n01_ + 1.5;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 7.5);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 7.5);

    // Addition with int
    n01 = n01 + 2;
    n01_ = n01_ + 2;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 9.5);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 9.5);

    // Addition with xll::Int
    n01 = n01 + xll::Int(3);
    n01_ = n01_ + 3;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 12.5);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 12.5);

    // Addition with xll::Bool
    n01 = n01 + xll::Bool(true);
    n01_ = n01_ + 1;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 13.5);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 13.5);

    // Subtraction with xll::Number
    n01 = n01 - n04;
    n01_ = n01_ - 2.5;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 11.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 11.0);

    // Subtraction with double
    n01 = n01 - 1.0;
    n01_ = n01_ - 1.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 10.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 10.0);

    // Multiplication with xll::Number
    n01 = n01 * xll::Number(1.5);
    n01_ = n01_ * 1.5;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 15.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 15.0);

    // Multiplication with double
    n01 = n01 * 2.0;
    n01_ = n01_ * 2.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 30.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 30.0);

    // Division with xll::Number
    n01 = n01 / xll::Number(3.0);
    n01_ = n01_ / 3.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 10.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 10.0);

    // Division with double
    n01 = n01 / 2.0;
    n01_ = n01_ / 2.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 5.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 5.0);

    // Division by zero returns NaN
    auto v = n01 / 0.0;
    REQUIRE(std::isinf(static_cast<double>(n01 / 0.0)));
    REQUIRE(std::isinf(static_cast<double>(n01 / xll::Number(0.0))));
    REQUIRE(std::isinf(static_cast<double>(n01 / xll::Int(0))));

    // Compound assignment: Addition
    n01 = 5.0;
    n01_ = 5.0;
    n01 += 3.0;
    n01_ += 3.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 8.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 8.0);

    n01 += xll::Number(2.0);
    n01_ += 2.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 10.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 10.0);

    // Compound assignment: Subtraction
    n01 -= 4.0;
    n01_ -= 4.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 6.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 6.0);

    n01 -= xll::Number(1.0);
    n01_ -= 1.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 5.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 5.0);

    // Compound assignment: Multiplication
    n01 *= 2.0;
    n01_ *= 2.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 10.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 10.0);

    n01 *= xll::Number(1.5);
    n01_ *= 1.5;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 15.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 15.0);

    // Compound assignment: Division
    n01 /= 3.0;
    n01_ /= 3.0;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 5.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 5.0);

    n01 /= xll::Number(2.5);
    n01_ /= 2.5;
    REQUIRE(n01 == n01_);
    REQUIRE(n01 == 2.0);
    REQUIRE(n01.xltype == xltypeNum);
    REQUIRE(n01.val.num == 2.0);

    // Compound assignment: Division by zero returns inf
    n01 /= 0.0;
    REQUIRE(std::isinf(static_cast<double>(n01)));
    n01 /= xll::Number(0.0);
    REQUIRE(std::isinf(static_cast<double>(n01)));

    // Comparison operators
    xll::Number n05(5.0);
    xll::Number n06(5.0);
    xll::Number n07(7.0);

    REQUIRE(n05 == n06);
    REQUIRE(n05 != n07);
    REQUIRE(n05 < n07);
    REQUIRE(n07 > n05);
    REQUIRE(n05 <= n06);
    REQUIRE(n05 <= n07);
    REQUIRE(n06 >= n05);
    REQUIRE(n07 >= n05);

    // Comparison with double
    REQUIRE(n05 == 5.0);
    REQUIRE(n05 != 7.0);
    REQUIRE(n05 < 7.0);
    REQUIRE(n07 > 5.0);
    REQUIRE(n05 <= 5.0);
    REQUIRE(n05 <= 7.0);
    REQUIRE(n05 >= 5.0);
    REQUIRE(n07 >= 5.0);

    // Implicit conversion to double
    xll::Number n08(3.14);
    double value = n08;
    REQUIRE(value == 3.14);

    // Conversion to xll::Int (truncation behavior)
    xll::Number n09(3.7);
    xll::Int i01 = n09;
    REQUIRE(i01 == 3);

    // Special cases: NaN and Infinity
    xll::Number n_inf = std::numeric_limits<double>::infinity();
    REQUIRE(std::isinf(n_inf.val.num));

    xll::Number n_neg_inf = -std::numeric_limits<double>::infinity();
    REQUIRE(std::isinf(n_neg_inf.val.num));
    REQUIRE(n_neg_inf < 0);

    // Test validity
    xll::Number valid_number(42.0);
    REQUIRE(valid_number.is_valid());

    // xll::Number invalid_number;
    // invalid_number.invalidate();
    // REQUIRE_FALSE(invalid_number.is_valid());
}