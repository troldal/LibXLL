//
// Created by kenne on 25/03/2025.
//

#include <catch2/catch_test_macros.hpp>
#include <xlcall.hpp>
#include "../Types/Variant.hpp"

TEST_CASE( "Variant Construction", "[xll::Variant]" ) {

    // // Check default construction. Should default to Number with a value of zero.
    // xll::Variant var1;
    // REQUIRE(var1.xltype == xltypeNum);
    // REQUIRE(xll::holds_alternative<xll::Number>(var1) == true);
    // REQUIRE(xll::get<xll::Number>(var1) == 0.0);
    //
    // // Check explicit assignment with xll::Number
    // xll::Variant var2 = xll::Number(3.14);
    // REQUIRE(var2.xltype == xltypeNum);
    // REQUIRE(xll::holds_alternative<xll::Number>(var2) == true);
    // REQUIRE(xll::get<xll::Number>(var2) == 3.14);
    //
    // xll::Variant var3 = xll::Int(42);
    // REQUIRE(var3.xltype == xltypeInt);
    // REQUIRE(xll::holds_alternative<xll::Int>(var3) == true);
    // REQUIRE(xll::get<xll::Int>(var3) == 42);
    //
    // xll::Variant var4 = xll::Bool(true);
    // REQUIRE(var4.xltype == xltypeBool);
    // REQUIRE(xll::holds_alternative<xll::Bool>(var4) == true);
    // REQUIRE(xll::get<xll::Bool>(var4) == true);
    //
    // xll::Variant var5 = xll::ErrName;
    // REQUIRE(var5.xltype == xltypeErr);
    // REQUIRE(xll::holds_alternative<xll::Error>(var5) == true);
    // REQUIRE(xll::get<xll::Error>(var5) == xll::ErrName);
    //
    // xll::Variant var6 = xll::String("Hello World");
    // REQUIRE(var6.xltype == xltypeStr);
    // REQUIRE(xll::holds_alternative<xll::String>(var6) == true);
    // REQUIRE(xll::get<xll::String>(var6) == "Hello World");
    //
    // xll::Variant var7 = xll::Nil();
    // REQUIRE(var7.xltype == xltypeNil);
    // REQUIRE(xll::holds_alternative<xll::Nil>(var7) == true);
    // REQUIRE(xll::get<xll::Nil>(var7) == xll::Nil());
    //
    // xll::Variant var8 = xll::Missing();
    // REQUIRE(var8.xltype == xltypeMissing);
    // REQUIRE(xll::holds_alternative<xll::Missing>(var8) == true);
    // REQUIRE(xll::get<xll::Missing>(var8) == xll::Missing());

}

TEST_CASE( "Variant Assignment", "[xll::Variant]" )
{
    // xll::Variant var1;
    // var1 = xll::Number(3.14);
    // REQUIRE(var1.xltype == xltypeNum);
    // REQUIRE(xll::holds_alternative<xll::Number>(var1) == true);
    // REQUIRE(xll::get<xll::Number>(var1) == 3.14);
    //
    // var1 = xll::Int(42);
    // REQUIRE(var1.xltype == xltypeInt);
    // REQUIRE(xll::holds_alternative<xll::Int>(var1) == true);
    // REQUIRE(xll::get<xll::Int>(var1) == 42);
    //
    // var1 = xll::Bool(true);
    // REQUIRE(var1.xltype == xltypeBool);
    // REQUIRE(xll::holds_alternative<xll::Bool>(var1) == true);
    // REQUIRE(xll::get<xll::Bool>(var1) == true);
    //
    // var1 = xll::ErrName;
    // REQUIRE(var1.xltype == xltypeErr);
    // REQUIRE(xll::holds_alternative<xll::Error>(var1) == true);
    // REQUIRE(xll::get<xll::Error>(var1) == xll::ErrName);
    //
    // var1 = xll::String("Hello World");
    // REQUIRE(var1.xltype == xltypeStr);
    // REQUIRE(xll::holds_alternative<xll::String>(var1) == true);
    // REQUIRE(xll::get<xll::String>(var1) == "Hello World");
    //
    // var1 = xll::Nil();
    // REQUIRE(var1.xltype == xltypeNil);
    // REQUIRE(xll::holds_alternative<xll::Nil>(var1) == true);
    // REQUIRE(xll::get<xll::Nil>(var1) == xll::Nil());
    //
    // var1 = xll::Missing();
    // REQUIRE(var1.xltype == xltypeMissing);
    // REQUIRE(xll::holds_alternative<xll::Missing>(var1) == true);
    // REQUIRE(xll::get<xll::Missing>(var1) == xll::Missing());
}

TEST_CASE( "Variant Visitation", "[xll::Variant]" )
{
    // using namespace std::literals;
    //
    // auto visitor = xll::overload{
    //     [](const xll::Number& arg) { return "NUMBER"s; },
    //     [](const xll::Int& arg) { return "INT"s; },
    //     [](const xll::Bool& arg) { return "BOOL"s; },
    //     [](const xll::String& arg) { return "STRING"s; },
    //     [](const xll::Nil& arg) { return "NIL"s; },
    //     [](const xll::Error& arg) { return "ERROR"s; },
    //     [](const xll::Missing& arg) { return "MISSING"s; },
    // };
    //
    // xll::Variant var;
    //
    // var = xll::Number(3.14);
    // REQUIRE(xll::visit(visitor, var) == "NUMBER");
    //
    // var = xll::Int(42);
    // REQUIRE(xll::visit(visitor, var) == "INT");
    //
    // var = xll::Bool(true);
    // REQUIRE(xll::visit(visitor, var) == "BOOL");
    //
    // var = xll::String("Hello World");
    // REQUIRE(xll::visit(visitor, var) == "STRING");
    //
    // var = xll::Nil();
    // REQUIRE(xll::visit(visitor, var) == "NIL");
    //
    // var = xll::ErrName;
    // REQUIRE(xll::visit(visitor, var) == "ERROR");
    //
    // var = xll::Missing();
    // REQUIRE(xll::visit(visitor, var) == "MISSING");
}