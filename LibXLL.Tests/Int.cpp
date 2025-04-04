//
// Created by kenne on 03/04/2025.
//


#include <catch2/catch_test_macros.hpp>
#include <xlcall.hpp>
#include "../Types/Int.hpp"
#include "../Types/Bool.hpp"
#include "../Types/Number.hpp"

TEST_CASE( "Integer Construction", "[xll::Int]" )
{
    // Default construction:
    xll::Int i01;
    REQUIRE(i01 == 0);
    REQUIRE_FALSE(i01 != 0);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 0);

    // Construction from int:
    xll::Int i02 = 42;
    REQUIRE(i02 == 42);
    REQUIRE_FALSE(i02 != 42);
    REQUIRE(i02.xltype == xltypeInt);
    REQUIRE(i02.val.w == 42);

    // Construction from float:
    xll::Int i03 = 3.14;
    REQUIRE(i03 == 3);
    REQUIRE_FALSE(i03 == 3.14);
    REQUIRE(i03.xltype == xltypeInt);
    REQUIRE(i03.val.w == 3);

    // Construction from bool
    xll::Int i04 = true;
    REQUIRE(i04 == true);
    REQUIRE_FALSE(i04 == false);
    REQUIRE(i04.xltype == xltypeInt);
    REQUIRE(i04.val.w == true);

    // XLOPER12 construction
    auto xl01 = XLOPER12();
    xl01.xltype = xltypeInt;
    xl01.val.w = 42;
    xll::Int i05 = xll::Int(xl01);
    REQUIRE(i05 == 42);
    REQUIRE_FALSE(i05 != 42);
    REQUIRE(i05.xltype == xltypeInt);
    REQUIRE(i05.val.w == 42);

    // Invalid XLOPER12 construction
    auto xl02 = XLOPER12();
    REQUIRE_THROWS(xll::Int(xl02));

    // Copy construction:
    xll::Int i07 = i02;
    REQUIRE(i07 == i02);
    REQUIRE_FALSE(i07 != i02);
    REQUIRE(i07 == 42);
    REQUIRE_FALSE(i07 != 42);
    REQUIRE(i07.xltype == xltypeInt);
    REQUIRE(i07.val.w == 42);

    // Move construction:
    xll::Int i08 = std::move(i07);
    REQUIRE(i08 == i02);
    REQUIRE_FALSE(i08 != i02);
    REQUIRE(i08 == 42);
    REQUIRE_FALSE(i08 != 42);
    REQUIRE(i08.xltype == xltypeInt);
    REQUIRE(i08.val.w == 42);

    // Construction from xll::Bool
    xll::Int i09 = xll::Bool(true);
    REQUIRE(i09 == true);
    REQUIRE_FALSE(i09 == false);
    REQUIRE(i09.xltype == xltypeInt);
    REQUIRE(i09.val.w == true);

    // Construction from xll::Number
    xll::Int i10 = xll::Number(3.14);
    REQUIRE(i10 == 3);
    REQUIRE_FALSE(i10 == 3.14);
    REQUIRE(i10.xltype == xltypeInt);
    REQUIRE(i10.val.w == 3);

    xll::Int invalid;
    invalid.xltype = xltypeNil;

    // Invalid copy construction
    REQUIRE_THROWS(xll::Int(invalid));
}

TEST_CASE( "Integer Assignment", "[xll::Int]" )
{
    xll::Int i01;

    // Assignment with int:
    i01 = 42;
    REQUIRE(i01 == 42);
    REQUIRE_FALSE(i01 != 42);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 42);

    // Assignment with float:
    i01 = 3.14;
    REQUIRE(i01 == 3);
    REQUIRE_FALSE(i01 == 3.14);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 3);

    // Assignment of bool:
    i01 = true;
    REQUIRE_FALSE(i01 == false);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == true);

    // Assignment of XLOPER12:
    auto xl01 = XLOPER12();
    xl01.xltype = xltypeInt;
    xl01.val.w = 42;
    i01 = xll::Int(xl01);
    REQUIRE(i01 == 42);
    REQUIRE_FALSE(i01 != 42);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 42);

    xll::Int to_be_copied = 42;

    // Copy assignment:
    i01 = to_be_copied;
    REQUIRE(i01 == to_be_copied);
    REQUIRE_FALSE(i01 != to_be_copied);
    REQUIRE(i01 == 42);
    REQUIRE_FALSE(i01 != 42);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 42);

    xll::Int to_be_moved = to_be_copied;

    // Move assignment:
    i01 = std::move(to_be_moved);
    REQUIRE(i01 == to_be_copied);
    REQUIRE_FALSE(i01 != to_be_copied);
    REQUIRE(i01 == 42);
    REQUIRE_FALSE(i01 != 42);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 42);

    // Assignment of xll::Bool
    i01 = xll::Bool(true);
    REQUIRE(i01 == true);
    REQUIRE_FALSE(i01 == false);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == true);

    // Assignment of xll::Number
    i01 = xll::Number(3.14);
    REQUIRE(i01 == 3);
    REQUIRE_FALSE(i01 == 3.14);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 3);

}

TEST_CASE( "Integer Operations", "[xll::Int]" )
{
    xll::Int i01;
    int i01_;

    // Post-increment
    auto i02 = i01++;
    REQUIRE(i01 == 1);
    REQUIRE_FALSE(i01 != 1);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 1);
    REQUIRE(i02 == 0);
    REQUIRE_FALSE(i02 != 0);
    REQUIRE(i02.xltype == xltypeInt);
    REQUIRE(i02.val.w == 0);

    // Pre-increment
    ++i01;
    REQUIRE(i01 == 2);
    REQUIRE_FALSE(i01 != 2);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 2);

    // Post-decrement
    auto i03 = i01--;
    REQUIRE(i01 == 1);
    REQUIRE_FALSE(i01 != 1);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 1);
    REQUIRE(i03 == 2);
    REQUIRE_FALSE(i03 != 2);
    REQUIRE(i03.xltype == xltypeInt);
    REQUIRE(i03.val.w == 2);

    // Pre-decrement
    --i01;
    REQUIRE(i01 == 0);
    REQUIRE_FALSE(i01 != 0);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == 0);

    // Addition
    i01 = i01 + xll::Int(1);
    REQUIRE(i01 == (0 + 1));
    REQUIRE_FALSE(i01 != (0 + 1));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (0 + 1));

    // Addition with int
    i01 = i01 + 1;
    REQUIRE(i01 == (1 + 1));
    REQUIRE_FALSE(i01 != (1 + 1));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (1 + 1));

    // Addition with xll::Bool
    i01 = i01 + xll::Bool(true);
    REQUIRE(i01 == (2 + true));
    REQUIRE_FALSE(i01 != (2 + true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (2 + true));

    // Addition with bool
    i01 = i01 + true;
    REQUIRE(i01 == (3 + true));
    REQUIRE_FALSE(i01 != (3 + true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (3 + true));

    // Addition with xll::Number
    i01 = i01 + xll::Number(3.14);
    REQUIRE(i01 == (int)(4 + 3.14));
    REQUIRE_FALSE(i01 != (int)(4 + 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(4 + 3.14));

    // Addition with double
    i01 = i01 + 3.14;
    REQUIRE(i01 == (int)(7 + 3.14));
    REQUIRE_FALSE(i01 != (int)(7 + 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(7 + 3.14));

    i01 = 10;
    // Subtraction
    i01 = i01 - xll::Int(1);
    REQUIRE(i01 == (10 - 1));
    REQUIRE_FALSE(i01 != (10 - 1));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (10 - 1));

    // Subtraction with int
    i01 = i01 - 1;
    REQUIRE(i01 == (9 - 1));
    REQUIRE_FALSE(i01 != (9 - 1));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (9 - 1));

    // Subtraction with xll::Bool
    i01 = i01 - xll::Bool(true);
    REQUIRE(i01 == (8 - true));
    REQUIRE_FALSE(i01 != (8 - true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (8 - true));

    // Subtraction with bool
    i01 = i01 - true;
    REQUIRE(i01 == (7 - true));
    REQUIRE_FALSE(i01 != (7 - true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (7 - true));

    // Subtraction with xll::Number
    i01 = i01 - xll::Number(3.14);
    REQUIRE(i01 == (int)(6 - 3.14));
    REQUIRE_FALSE(i01 != (int)(6 - 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(6 - 3.14));

    // Subtraction with double
    i01 = i01 - 3.14;
    REQUIRE(i01 == (int)(2 - 3.14));
    REQUIRE_FALSE(i01 != (int)(2 - 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(2 - 3.14));


    i01 = 1;

    // Multiplication
    i01 = i01 * xll::Int(2);
    REQUIRE(i01 == (1 * 2));
    REQUIRE_FALSE(i01 != (1 * 2));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (1 * 2));

    // Multiplication with int
    i01 = i01 * 2;
    REQUIRE(i01 == (2 * 2));
    REQUIRE_FALSE(i01 != (2 * 2));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (2 * 2));

    // Multiplication with xll::Bool
    i01 = i01 * xll::Bool(true);
    REQUIRE(i01 == (4 * true));
    REQUIRE_FALSE(i01 != (4 * true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (4 * true));

    // Multiplication with bool
    i01 = i01 * true;
    REQUIRE(i01 == (4 * true));
    REQUIRE_FALSE(i01 != (4 * true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (4 * true));

    // Multiplication with xll::Number
    i01 = i01 * xll::Number(3.14);
    REQUIRE(i01 == (int)(4 * 3.14));
    REQUIRE_FALSE(i01 != (int)(4 * 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(4 * 3.14));

    // Multiplication with double
    i01 = i01 * 3.14;
    REQUIRE(i01 == (int)(12 * 3.14));
    REQUIRE_FALSE(i01 != (int)(12 * 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(12 * 3.14));

    i01 = 80;

    // Division
    i01 = i01 / xll::Int(2);
    REQUIRE(i01 == (80 / 2));
    REQUIRE_FALSE(i01 != (80 / 2));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (80 / 2));

    // Division with int
    i01 = i01 / 2;
    REQUIRE(i01 == (40 / 2));
    REQUIRE_FALSE(i01 != (40 / 2));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (40 / 2));

    // Division with xll::Bool
    i01 = i01 / xll::Bool(true);
    REQUIRE(i01 == (20 / true));
    REQUIRE_FALSE(i01 != (20 / true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (20 / true));

    // Division with bool
    i01 = i01 / true;
    REQUIRE(i01 == (20 / true));
    REQUIRE_FALSE(i01 != (20 / true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (20 / true));

    // Division with xll::Number
    i01 = i01 / xll::Number(3.14);
    REQUIRE(i01 == (int)(20 / 3.14));
    REQUIRE_FALSE(i01 != (int)(20 / 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(20 / 3.14));

    // Division with bool
    i01 = i01 / 3.14;
    REQUIRE(i01 == (int)(6 / 3.14));
    REQUIRE_FALSE(i01 != (int)(6 / 3.14));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (int)(6 / 3.14));

    i01 = 10;
    // Modulo
    i01 = i01 % xll::Int(3);
    REQUIRE(i01 == (10 % 3));
    REQUIRE_FALSE(i01 != (10 % 3));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (10 % 3));

    i01 = 10;
    // Modulo with int
    i01 = i01 % 3;
    REQUIRE(i01 == (10 % 3));
    REQUIRE_FALSE(i01 != (10 % 3));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (10 % 3));

    i01 = 10;
    // Modulo with xll::Bool
    i01 = i01 % xll::Bool(true);
    REQUIRE(i01 == (10 % true));
    REQUIRE_FALSE(i01 != (10 % true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (10 % true));

    i01 = 10;
    // Modulo with bool
    i01 = i01 % true;
    REQUIRE(i01 == (10 % true));
    REQUIRE_FALSE(i01 != (10 % true));
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == (10 % true));

    i01 = 0;
    i01_ = 0;
    // Addition assignment
    i01 += xll::Int(1);
    i01_ += 1;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Addition assignment with int
    i01 += 1;
    i01_ += 1;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Addition assignment with xll::Bool
    i01 += xll::Bool(true);
    i01_ += true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Addition assignment with bool
    i01 += true;
    i01_ += true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Addition assignment with xll::Number
    i01 += xll::Number(3.14);
    i01_ += 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Addition assignment with double
    i01 += 3.14;
    i01_ += 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);


    i01 = 10;
    i01_ = 10;
    // Subtraction assignment
    i01 -= xll::Int(1);
    i01_ -= 1;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Subtraction assignment with int
    i01 -= 1;
    i01_ -= 1;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Subtraction assignment with xll::Bool
    i01 -= xll::Bool(true);
    i01_ -= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Subtraction assignment with bool
    i01 -= true;
    i01_ -= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Subtraction assignment with xll::Number
    i01 -= xll::Number(3.14);
    i01_ -= 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Subtraction assignment with double
    i01 -= 3.14;
    i01_ -= 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    i01 = 1;
    i01_ = 1;
    // Multiplication assignment
    i01 *= xll::Int(2);
    i01_ *= 2;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Multiplication assignment with int
    i01 *= 2;
    i01_ *= 2;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Multiplication assignment with xll::Bool
    i01 *= xll::Bool(true);
    i01_ *= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Multiplication assignment with bool
    i01 *= true;
    i01_ *= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Multiplication assignment with xll::Number
    i01 *= xll::Number(3.14);
    i01_ *= 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Multiplication assignment with double
    i01 *= 3.14;
    i01_ *= 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);


    i01 = 80;
    i01_ = 80;
    // Division assignment
    i01 /= xll::Int(2);
    i01_ /= 2;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Division assignment with int
    i01 /= 2;
    i01_ /= 2;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Division assignment with xll::Bool
    i01 /= xll::Bool(true);
    i01_ /= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Division assignment with bool
    i01 /= true;
    i01_ /= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Division assignment with xll::Number
    i01 /= xll::Number(3.14);
    i01_ /= 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    // Division assignment with double
    i01 /= 3.14;
    i01_ /= 3.14;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);


    i01 = 10;
    i01_ = 10;
    // Modulo assignment
    i01 %= xll::Int(3);
    i01_ %= 3;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    i01 = 10;
    i01_ = 10;
    // Modulo assignment with int
    i01 %= 3;
    i01_ %= 3;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    i01 = 10;
    i01_ = 10;
    // Modulo assignment with xll::Bool
    i01 %= xll::Bool(true);
    i01_ %= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);

    i01 = 10;
    i01_ = 10;
    // Modulo assignment with bool
    i01 %= true;
    i01_ %= true;
    REQUIRE(i01 == i01_);
    REQUIRE_FALSE(i01 != i01_);
    REQUIRE(i01.xltype == xltypeInt);
    REQUIRE(i01.val.w == i01_);
}