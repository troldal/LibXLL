//
// Created by kenne on 25/03/2025.
//

#include <catch2/catch_test_macros.hpp>
#include <xlcall.hpp>
#include "../Types/Array.hpp"

TEST_CASE( "Array Construction", "[xll::Array]" ) {

    // Check default construction. Should default to Number with a value of zero.
    xll::Array arr1;
    REQUIRE(arr1.xltype == xltypeMulti);
    REQUIRE(arr1.val.array.lparray == nullptr);
    REQUIRE(arr1.val.array.rows == 0);
    REQUIRE(arr1.val.array.columns == 0);


    xll::Array arr2 = xll::Array(2,2);
    REQUIRE(arr2.xltype == xltypeMulti);
    REQUIRE(arr2.val.array.lparray != nullptr);
    REQUIRE(arr2.val.array.rows == 2);
    REQUIRE(arr2.val.array.columns == 2);

    auto temp = xll::String("ITEM ") + std::to_string(9);

    for (int index = 0; auto& item : arr2) item = xll::String("ITEM ") + std::to_string(index++);
    REQUIRE(xll::get<xll::String>(arr2[0]) == "ITEM 0");
    REQUIRE(xll::get<xll::String>(arr2[1]) == "ITEM 1");
    REQUIRE(xll::get<xll::String>(arr2[2]) == "ITEM 2");
    REQUIRE(xll::get<xll::String>(arr2[3]) == "ITEM 3");

    xll::Array arr3 = arr2;
    REQUIRE(arr3.xltype == xltypeMulti);
    REQUIRE(arr3.val.array.lparray != arr2.val.array.lparray);
    REQUIRE(arr3.val.array.lparray != nullptr);
    REQUIRE(arr3.val.array.rows == 2);
    REQUIRE(arr3.val.array.columns == 2);

    REQUIRE(xll::get<xll::String>(arr3[0]) == "ITEM 0");
    REQUIRE(xll::get<xll::String>(arr3[1]) == "ITEM 1");
    REQUIRE(xll::get<xll::String>(arr3[2]) == "ITEM 2");
    REQUIRE(xll::get<xll::String>(arr3[3]) == "ITEM 3");


}