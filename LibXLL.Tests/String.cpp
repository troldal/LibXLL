//
// Created by kenne on 24/03/2025.
//

#include <windows.h>
#include <catch2/catch_test_macros.hpp>
#include <xlcall.h>
#include "../Types/String.hpp"

TEST_CASE( "String Construction", "[xll::String]" ) {

    xll::String str1;
    REQUIRE(str1.xltype == xltypeStr);
    REQUIRE(str1.val.str != nullptr);
    REQUIRE(std::wstring(&str1.val.str[1]) == std::wstring(L""));
    REQUIRE(str1.val.str[0] == std::wstring(L"").length());

    xll::String str2 = xll::String("str2");
    REQUIRE(str2.xltype == xltypeStr);
    REQUIRE(str2.val.str != nullptr);
    REQUIRE(std::wstring(&str2.val.str[1]) == std::wstring(L"str2"));
    REQUIRE(str2.val.str[0] == std::wstring(L"str2").length());

    xll::String str3 = str2;
    REQUIRE(str3.xltype == xltypeStr);
    REQUIRE(str3.val.str != str2.val.str);
    REQUIRE(std::wstring(&str2.val.str[1]) == std::wstring(&str3.val.str[1]));
    REQUIRE(str2.val.str[0] == str3.val.str[0]);

    xll::String str4 = "str4";
    REQUIRE(str4.xltype == xltypeStr);
    REQUIRE(str4.val.str != nullptr);
    REQUIRE(std::wstring(&str4.val.str[1]) == std::wstring(L"str4"));
    REQUIRE(str4.val.str[0] == str3.val.str[0]);

    xll::String str5 = std::move(str4);
    REQUIRE(str5.xltype == xltypeStr);
    REQUIRE(str4.val.str == nullptr);
    REQUIRE(str5.val.str != nullptr);
    REQUIRE(std::wstring(&str5.val.str[1]) == std::wstring(L"str4"));
    REQUIRE(str5.val.str[0] == str3.val.str[0]);

    auto str5xloper = static_cast<XLOPER12&>(str5);
    xll::String str6 = xll::String(str5xloper);
    REQUIRE(str6.xltype == xltypeStr);
    REQUIRE(str6.val.str != nullptr);
    REQUIRE(std::wstring(&str6.val.str[1]) == std::wstring(L"str4"));
    REQUIRE(str6.val.str[0] == str5.val.str[0]);

    using namespace xll::literals;
    auto str7 = "str7"_xs;
    REQUIRE(str7.xltype == xltypeStr);
    REQUIRE(str7.val.str != nullptr);
    REQUIRE(std::wstring(&str7.val.str[1]) == std::wstring(L"str7"));
    REQUIRE(str7.val.str[0] == std::wstring(L"str7").length());

}

TEST_CASE( "String Assignment", "[xll::String]" )
{
    xll::String str1 = xll::String("str1");
    xll::String str2 = xll::String("str2");

    //auto str1address = str1.val.str;
    //REQUIRE(str1.val.str == str1address);

    str1 = str2;
    //REQUIRE(str1.val.str != str1address); // The new string may be stored in the same location
    REQUIRE(str1.val.str != str2.val.str);
    REQUIRE(std::wstring(&str1.val.str[1]) == std::wstring(&str2.val.str[1]));
    REQUIRE(str1.val.str[0] == str2.val.str[0]);

    str1 = std::move(str2);
    REQUIRE(str2.val.str == nullptr);
    REQUIRE(str1.val.str != str2.val.str);
    REQUIRE(std::wstring(&str1.val.str[1]) == std::wstring(L"str2"));
}