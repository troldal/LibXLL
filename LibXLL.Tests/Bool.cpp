#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Number.hpp"
#include <catch2/catch_test_macros.hpp>
#include <xlcall.hpp>

TEST_CASE("Bool Construction", "[xll::Bool]")
{
    // Default construction:
    xll::Bool b01;
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    // Construction from bool:
    xll::Bool b02 = true;
    REQUIRE(b02 == true);
    REQUIRE_FALSE(b02 == false);
    REQUIRE(b02.xltype == xltypeBool);
    REQUIRE(b02.val.xbool == true);

    // Construction from int:
    xll::Bool b03 = 1;
    REQUIRE(b03 == true);
    REQUIRE_FALSE(b03 == false);
    REQUIRE(b03.xltype == xltypeBool);
    REQUIRE(b03.val.xbool == true);

    xll::Bool b04 = 0;
    REQUIRE(b04 == false);
    REQUIRE_FALSE(b04 == true);
    REQUIRE(b04.xltype == xltypeBool);
    REQUIRE(b04.val.xbool == false);

    // Construction from float:
    xll::Bool b05 = 3.14;
    REQUIRE(b05 == true);
    REQUIRE_FALSE(b05 == false);
    REQUIRE(b05.xltype == xltypeBool);
    REQUIRE(b05.val.xbool != false);

    xll::Bool b06 = 0.0;
    REQUIRE(b06 == false);
    REQUIRE_FALSE(b06 == true);
    REQUIRE(b06.xltype == xltypeBool);
    REQUIRE(b06.val.xbool == false);

    // XLOPER12 construction
    auto xl01      = XLOPER12();
    xl01.xltype    = xltypeBool;
    xl01.val.xbool = true;
    xll::Bool b07  = xll::Bool(xl01);
    REQUIRE(b07 == true);
    REQUIRE_FALSE(b07 == false);
    REQUIRE(b07.xltype == xltypeBool);
    REQUIRE(b07.val.xbool != false);

    // Invalid XLOPER12 construction
    auto xl02 = XLOPER12();
    REQUIRE_THROWS(xll::Bool(xl02));

    // Copy construction:
    xll::Bool b08 = b02;
    REQUIRE(b08 == b02);
    REQUIRE_FALSE(b08 != b02);
    REQUIRE(b08 == true);
    REQUIRE_FALSE(b08 != true);
    REQUIRE(b08.xltype == xltypeBool);
    REQUIRE(b08.val.xbool != false);

    // Move construction:
    xll::Bool b09 = std::move(b08);
    REQUIRE(b09 == b02);
    REQUIRE_FALSE(b09 != b02);
    REQUIRE(b09 == true);
    REQUIRE_FALSE(b09 != true);
    REQUIRE(b09.xltype == xltypeBool);
    REQUIRE(b09.val.xbool != false);

    // Construction from xll::Int
    xll::Bool b10 = xll::Int(1);
    REQUIRE(b10 == true);
    REQUIRE_FALSE(b10 == false);
    REQUIRE(b10.xltype == xltypeBool);
    REQUIRE(b10.val.xbool != false);

    xll::Bool b11 = xll::Int(0);
    REQUIRE(b11 == false);
    REQUIRE_FALSE(b11 == true);
    REQUIRE(b11.xltype == xltypeBool);
    REQUIRE(b11.val.xbool == false);

    // Construction from xll::Number
    xll::Bool b12 = xll::Number(3.14);
    REQUIRE(b12 == true);
    REQUIRE_FALSE(b12 == false);
    REQUIRE(b12.xltype == xltypeBool);
    REQUIRE(b12.val.xbool != false);

    xll::Bool b13 = xll::Number(0.0);
    REQUIRE(b13 == false);
    REQUIRE_FALSE(b13 == true);
    REQUIRE(b13.xltype == xltypeBool);
    REQUIRE(b13.val.xbool == false);

    xll::Bool invalid;
    invalid.xltype = xltypeNil;

    // Invalid copy construction
    REQUIRE_THROWS(xll::Bool(invalid));
}

TEST_CASE("Bool Assignment", "[xll::Bool]")
{
    xll::Bool b01;

    // Assignment with bool:
    b01 = true;
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Assignment with int:
    b01 = 0;
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    b01 = 42;
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool != false);

    // Assignment with float:
    b01 = 0.0;
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    b01 = 3.14;
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool != false);

    // Assignment of XLOPER12:
    auto xl01      = XLOPER12();
    xl01.xltype    = xltypeBool;
    xl01.val.xbool = true;
    b01            = xll::Bool(xl01);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    xll::Bool to_be_copied = false;

    // Copy assignment:
    b01 = to_be_copied;
    REQUIRE(b01 == to_be_copied);
    REQUIRE_FALSE(b01 != to_be_copied);
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 != false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    xll::Bool to_be_moved = true;

    // Move assignment:
    b01 = std::move(to_be_moved);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Assignment of xll::Int
    b01 = xll::Int(0);
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    b01 = xll::Int(42);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool != false);

    // Assignment of xll::Number
    b01 = xll::Number(0.0);
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    b01 = xll::Number(3.14);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool != false);
}

TEST_CASE("Bool Operations", "[xll::Bool]")
{
    xll::Bool b01(false);
    bool      b01_ = false;

    // Logical NOT
    xll::Bool b02  = !b01;
    auto      b02_ = !b01_;
    REQUIRE(b02 == b02_);
    REQUIRE(b02 == true);
    REQUIRE_FALSE(b02 == false);
    REQUIRE(b02.xltype == xltypeBool);
    REQUIRE(b02.val.xbool == true);

    // Logical AND with xll::Bool
    xll::Bool b03(true);
    b01  = b02 && b03;
    b01_ = b02_ && true;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical AND with bool
    b01  = b02 && true;
    b01_ = b02_ && true;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical AND with int
    b01  = b02 && 1;
    b01_ = b02_ && 1;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical AND with xll::Int
    b01  = b02 && xll::Int(1);
    b01_ = b02_ && 1;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical AND with xll::Number
    b01  = b02 && xll::Number(1.0);
    b01_ = b02_ && true;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical OR with xll::Bool
    b01  = false;
    b01_ = false;
    xll::Bool b04(false);
    b01  = b01 || b04;
    b01_ = b01_ || false;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    // Logical OR with bool
    b01  = b01 || true;
    b01_ = b01_ || true;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical OR with int
    b01  = false;
    b01_ = false;
    b01  = b01 || 1;
    b01_ = b01_ || 1;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Logical OR with xll::Int
    b01  = false;
    b01_ = false;
    b01  = b01 || xll::Int(0);
    b01_ = b01_ || 0;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == false);
    REQUIRE_FALSE(b01 == true);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == false);

    // Logical OR with xll::Number
    b01  = false;
    b01_ = false;
    b01  = b01 || xll::Number(1.0);
    b01_ = b01_ || true;
    REQUIRE(b01 == b01_);
    REQUIRE(b01 == true);
    REQUIRE_FALSE(b01 == false);
    REQUIRE(b01.xltype == xltypeBool);
    REQUIRE(b01.val.xbool == true);

    // Implicit conversion to bool in if statement
    xll::Bool b_true(true);
    xll::Bool b_false(false);
    int       count = 0;

    if (b_true) count++;
    if (b_false) count--;

    REQUIRE(count == 1);

    // Comparison operators
    REQUIRE((b_true == true));
    REQUIRE((b_true != false));
    REQUIRE((b_false == false));
    REQUIRE((b_false != true));
    REQUIRE((b_true == xll::Bool(true)));
    REQUIRE((b_true != xll::Bool(false)));
}