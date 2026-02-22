//
// Demonstrates the shape-tagged constructors for xll::Array, including
// initializer_list<TValue>, initializer_list<U>, range<TValue>, and range<U> overloads.
//

#include <Types/Array.hpp>
#include <Types/Number.hpp>
#include <Types/String.hpp>

#include <deque>
#include <iostream>
#include <ranges>
#include <vector>

using NumberArray = xll::Array<xll::Number>;
using StringArray = xll::Array<xll::String>;

void print_array(const char* label, const auto& arr)
{
    using Array = std::remove_cvref_t<decltype(arr)>;

    std::cout << label << "\n";
    std::cout << "  shape : ";
    switch (arr.shape()) {
        case Array::Shape::template IndexOf<typename Array::Empty>():         std::cout << "Empty\n";         break;
        case Array::Shape::template IndexOf<typename Array::Singular>():      std::cout << "Singular\n";      break;
        case Array::Shape::template IndexOf<typename Array::Horizontal>():    std::cout << "Horizontal\n";    break;
        case Array::Shape::template IndexOf<typename Array::Vertical>():      std::cout << "Vertical\n";      break;
        case Array::Shape::template IndexOf<typename Array::TwoDimensional>():std::cout << "TwoDimensional\n";break;
    }

    std::cout << "  rows  : " << arr.rows() << "\n";
    std::cout << "  cols  : " << arr.cols() << "\n";
    std::cout << "  values: ";
    for (size_t i = 0; i < arr.size(); ++i)
        std::cout << static_cast<double>(arr[i]) << " ";
    std::cout << "\n\n";
}

void print_string_array(const char* label, const StringArray& arr)
{
    std::cout << label << "\n";
    std::cout << "  shape : ";
    switch (arr.shape()) {
        case StringArray::Shape::IndexOf<StringArray::Empty>():         std::cout << "Empty\n";         break;
        case StringArray::Shape::IndexOf<StringArray::Singular>():      std::cout << "Singular\n";      break;
        case StringArray::Shape::IndexOf<StringArray::Horizontal>():    std::cout << "Horizontal\n";    break;
        case StringArray::Shape::IndexOf<StringArray::Vertical>():      std::cout << "Vertical\n";      break;
        case StringArray::Shape::IndexOf<StringArray::TwoDimensional>():std::cout << "TwoDimensional\n";break;
    }
    std::cout << "  rows  : " << arr.rows() << "\n";
    std::cout << "  cols  : " << arr.cols() << "\n";
    std::cout << "  values: ";
    for (size_t i = 0; i < arr.size(); ++i)
        std::cout << std::string(arr[i]) << " ";
    std::cout << "\n\n";
}

int main()
{
    // -----------------------------------------------------------------------
    // 1. No explicit size: array is sized to fit the initializer list exactly
    // -----------------------------------------------------------------------

    // Untagged: defaults to Horizontal (1 row, N columns)
    NumberArray untagged { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) };
    print_array("1a. Untagged – auto-fit Horizontal:", untagged);

    // Explicit Horizontal, no size
    NumberArray h { { xll::Number(10.0), xll::Number(20.0), xll::Number(30.0) },
                    NumberArray::Horizontal{} };
    print_array("1b. Horizontal{} – auto-fit:", h);

    // Explicit Vertical, no size
    NumberArray v { { xll::Number(10.0), xll::Number(20.0), xll::Number(30.0) },
                    NumberArray::Vertical{} };
    print_array("1c. Vertical{} – auto-fit:", v);

    // -----------------------------------------------------------------------
    // 2. Explicit size == values.size(): same as auto-fit
    // -----------------------------------------------------------------------

    NumberArray h_exact { { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) },
                          NumberArray::Horizontal(3) };
    print_array("2a. Horizontal(3) – exact size match:", h_exact);

    NumberArray v_exact { { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0) },
                          NumberArray::Vertical(3) };
    print_array("2b. Vertical(3) – exact size match:", v_exact);

    // -----------------------------------------------------------------------
    // 3. Explicit size > values.size(): remainder padded with fill value
    // -----------------------------------------------------------------------

    // Horizontal, padded with 0.0 (default TValue{})
    NumberArray h_padded { { xll::Number(1.0), xll::Number(2.0) },
                           NumberArray::Horizontal(5) };
    print_array("3a. Horizontal(5), 2 values, default fill (0.0):", h_padded);

    // Vertical, padded with a custom fill value
    NumberArray v_padded { { xll::Number(7.0), xll::Number(8.0) },
                           NumberArray::Vertical(5),
                           xll::Number(-1.0) };
    print_array("3b. Vertical(5), 2 values, fill = -1.0:", v_padded);

    // String array padded with a custom fill string
    StringArray s_padded { { xll::String("alpha"), xll::String("beta") },
                           StringArray::Horizontal(5),
                           xll::String("(empty)") };
    print_string_array("3c. Horizontal(5) strings, 2 values, fill = \"(empty)\":", s_padded);

    // -----------------------------------------------------------------------
    // 4. Edge cases
    // -----------------------------------------------------------------------

    // Empty initializer list with explicit size – all elements are the fill
    NumberArray all_fill { {}, NumberArray::Horizontal(3), xll::Number(99.0) };
    print_array("4a. Horizontal(3), empty list, fill = 99.0:", all_fill);

    // Empty initializer list with size 0 – results in an empty array
    NumberArray empty { {}, NumberArray::Horizontal() };
    print_array("4b. Horizontal{}, empty list:", empty);

    // Single element, Vertical
    NumberArray singular { { xll::Number(42.0) }, NumberArray::Vertical{} };
    print_array("4c. Vertical{}, single element:", singular);

    // -----------------------------------------------------------------------
    // 5. TwoDimensional shape
    // -----------------------------------------------------------------------

    // Exact fit: 2 rows × 3 cols = 6 elements
    NumberArray td_exact { { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0),
                              xll::Number(4.0), xll::Number(5.0), xll::Number(6.0) },
                           NumberArray::TwoDimensional(2, 3) };
    print_array("5a. TwoDimensional(2,3) – exact fit (2×3):", td_exact);

    // Padded: 2 rows × 4 cols = 8 slots, only 5 values supplied, rest filled with 0.0
    NumberArray td_padded { { xll::Number(10.0), xll::Number(20.0), xll::Number(30.0),
                              xll::Number(40.0), xll::Number(50.0) },
                            NumberArray::TwoDimensional(2, 4) };
    print_array("5b. TwoDimensional(2,4), 5 values, default fill (0.0):", td_padded);

    // Padded with custom fill
    NumberArray td_fill { { xll::Number(1.0), xll::Number(2.0) },
                          NumberArray::TwoDimensional(3, 3),
                          xll::Number(-1.0) };
    print_array("5c. TwoDimensional(3,3), 2 values, fill = -1.0:", td_fill);

    // String 2D array
    StringArray td_str { { xll::String("A"), xll::String("B"), xll::String("C"),
                           xll::String("D") },
                         StringArray::TwoDimensional(2, 3),
                         xll::String("?") };
    print_string_array("5d. TwoDimensional(2,3) strings, 4 values, fill = \"?\":", td_str);

    // -----------------------------------------------------------------------
    // 6. Exception: explicit size smaller than the initializer list
    // -----------------------------------------------------------------------

    std::cout << "6a. Horizontal(2) with 4 values (expect out_of_range):\n";
    try {
        NumberArray too_small { { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0), xll::Number(4.0) },
                                NumberArray::Horizontal(2) };
        std::cout << "  ERROR: no exception thrown!\n\n";
    }
    catch (const std::out_of_range& e) {
        std::cout << "  Caught std::out_of_range: " << e.what() << "\n\n";
    }

    std::cout << "6b. TwoDimensional(2,2) with 5 values (expect out_of_range):\n";
    try {
        NumberArray td_too_small { { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0),
                                     xll::Number(4.0), xll::Number(5.0) },
                                   NumberArray::TwoDimensional(2, 2) };
        std::cout << "  ERROR: no exception thrown!\n\n";
    }
    catch (const std::out_of_range& e) {
        std::cout << "  Caught std::out_of_range: " << e.what() << "\n\n";
    }

    // -----------------------------------------------------------------------
    // 7. Generic range constructor
    // -----------------------------------------------------------------------

    // std::vector – Horizontal, auto-fit
    std::vector<xll::Number> vec { xll::Number(1.0), xll::Number(2.0), xll::Number(3.0), xll::Number(4.0) };
    NumberArray from_vec(vec, NumberArray::Horizontal{});
    print_array("7a. From std::vector, Horizontal{}:", from_vec);

    // std::vector – Vertical, explicit size with padding
    NumberArray from_vec_v(vec, NumberArray::Vertical(6), xll::Number(0.0));
    print_array("7b. From std::vector, Vertical(6), fill = 0.0:", from_vec_v);

    // std::deque – TwoDimensional, exact fit
    std::deque<xll::Number> deq { xll::Number(10.0), xll::Number(20.0),
                                  xll::Number(30.0), xll::Number(40.0),
                                  xll::Number(50.0), xll::Number(60.0) };
    NumberArray from_deq(deq, NumberArray::TwoDimensional(2, 3));
    print_array("7c. From std::deque, TwoDimensional(2,3):", from_deq);

    // std::ranges::transform_view – square each element, Horizontal auto-fit
    auto squares = vec | std::views::transform([](xll::Number n) {
        return xll::Number(static_cast<double>(n) * static_cast<double>(n));
    });
    NumberArray from_transform(squares, NumberArray::Horizontal{});
    print_array("7d. From transform_view (squares), Horizontal{}:", from_transform);

    // std::ranges::filter_view – keep only values > 2.0, Vertical auto-fit
    auto filtered = vec | std::views::filter([](xll::Number n) {
        return static_cast<double>(n) > 2.0;
    });
    NumberArray from_filter(filtered, NumberArray::Vertical{});
    print_array("7e. From filter_view (values > 2.0), Vertical{}:", from_filter);

    // std::vector<std::string> – String array, TwoDimensional with padding
    std::vector<xll::String> str_vec { xll::String("foo"), xll::String("bar"), xll::String("baz") };
    StringArray from_str_vec(str_vec, StringArray::TwoDimensional(2, 3), xll::String("-"));
    print_string_array("7f. From std::vector<String>, TwoDimensional(2,3), fill = \"-\":", from_str_vec);

    // -----------------------------------------------------------------------
    // 8. Converting constructors (U → TValue)
    // -----------------------------------------------------------------------

    // initializer_list<double> → NumberArray, default Horizontal, auto-fit
    NumberArray from_doubles { 1.0, 2.0, 3.0, 4.0 };
    print_array("8a. initializer_list<double>, default Horizontal{}:", from_doubles);

    // initializer_list<double> → NumberArray, Vertical, auto-fit
    NumberArray from_doubles_v({ 1.0, 2.0, 3.0 }, NumberArray::Vertical{});
    print_array("8b. initializer_list<double>, Vertical{}:", from_doubles_v);

    // initializer_list<double> → NumberArray, TwoDimensional, padded
    NumberArray from_doubles_td({ 1.0, 2.0, 3.0 }, NumberArray::TwoDimensional(2, 3), 0.0);
    print_array("8c. initializer_list<double>, TwoDimensional(2,3), fill = 0.0:", from_doubles_td);

    // initializer_list<const char*> → StringArray, Horizontal, auto-fit
    StringArray from_cstrings({ "foo", "bar", "baz" }, StringArray::Horizontal{});
    print_string_array("8d. initializer_list<const char*>, Horizontal{}:", from_cstrings);

    // std::vector<double> → NumberArray, Horizontal, auto-fit
    std::vector<double> dbl_vec { 10.0, 20.0, 30.0, 40.0 };
    NumberArray from_dbl_vec(dbl_vec, NumberArray::Horizontal{});
    print_array("8e. std::vector<double>, Horizontal{}:", from_dbl_vec);

    // std::vector<double> → NumberArray, TwoDimensional, padded
    NumberArray from_dbl_vec_td(dbl_vec, NumberArray::TwoDimensional(2, 3), xll::Number(-1.0));
    print_array("8f. std::vector<double>, TwoDimensional(2,3), fill = -1.0:", from_dbl_vec_td);

    // std::deque<double> → NumberArray, Vertical, auto-fit
    std::deque<double> dbl_deq { 5.0, 10.0, 15.0 };
    NumberArray from_dbl_deq(dbl_deq, NumberArray::Vertical{});
    print_array("8g. std::deque<double>, Vertical{}:", from_dbl_deq);

    // transform_view<double> → NumberArray (converting range of non-TValue)
    auto dbl_squares = dbl_vec | std::views::transform([](double d) { return d * d; });
    NumberArray from_dbl_squares(dbl_squares, NumberArray::Horizontal{});
    print_array("8h. transform_view<double> (squares), Horizontal{}:", from_dbl_squares);

    // std::vector<std::string> → StringArray, TwoDimensional, padded
    std::vector<std::string> std_str_vec { "alpha", "beta", "gamma" };
    StringArray from_std_str(std_str_vec, StringArray::TwoDimensional(2, 2), xll::String("-"));
    print_string_array("8i. std::vector<std::string>, TwoDimensional(2,2), fill = \"-\":", from_std_str);

    return 0;
}

