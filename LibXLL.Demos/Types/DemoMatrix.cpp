// DemoMatrix.cpp
//
// Demonstrates xll::Matrix (owning, value-semantic, MatrixBuffer-backed) and
// xll::MatrixBuffer (non-constructible by client code, FP12-compatible buffer).
//
// Sections
// --------
//   1. xll::Matrix construction
//   2. Element access and modification
//   3. Value semantics (copy, move, assignment)
//   4. xll::MatrixBuffer — reading and writing FP12-compatible data
//   5. Matrix::get() — direct access to the underlying MatrixBuffer
//   6. MatrixBuffer → Matrix implicit conversion
//   7. std::ranges — algorithms and views over flat element storage
//   8. Row and column views — row() and col()

#include <Types/Matrix.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void separator(const char* title)
{
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================================\n";
}

static void subsection(const char* title)
{
    std::cout << "\n--- " << title << " ---\n";
}

static void print_matrix(const char* label, const xll::Matrix& m)
{
    std::cout << label << " (" << m.rows() << "×" << m.cols() << ")\n";
    if (m.empty()) {
        std::cout << "  <empty>\n";
        return;
    }
    for (std::size_t r = 0; r < static_cast<std::size_t>(m.rows()); ++r) {
        std::cout << "  [";
        for (std::size_t c = 0; c < static_cast<std::size_t>(m.cols()); ++c) {
            if (c > 0) std::cout << ", ";
            std::cout << std::setw(6) << std::fixed << std::setprecision(2) << m[r, c];
        }
        std::cout << "]\n";
    }
}

static void print_view(const char* label, const xll::MatrixBuffer& v)
{
    std::cout << label << " (" << v.rows() << "×" << v.cols() << ")\n";
    if (v.empty()) {
        std::cout << "  <empty>\n";
        return;
    }
    for (std::size_t r = 0; r < static_cast<std::size_t>(v.rows()); ++r) {
        std::cout << "  [";
        for (std::size_t c = 0; c < static_cast<std::size_t>(v.cols()); ++c) {
            if (c > 0) std::cout << ", ";
            std::cout << std::setw(6) << std::fixed << std::setprecision(2) << v[r, c];
        }
        std::cout << "]\n";
    }
}

// ---------------------------------------------------------------------------
// Section 1: xll::Matrix construction
// ---------------------------------------------------------------------------

void demo_matrix_construction()
{
    separator("1. xll::Matrix construction");

    subsection("1a. Default construction — empty (0×0)");
    {
        xll::Matrix m;
        std::cout << "  rows()  = " << m.rows()  << "\n";
        std::cout << "  cols()  = " << m.cols()  << "\n";
        std::cout << "  size()  = " << m.size()  << "\n";
        std::cout << "  empty() = " << std::boolalpha << m.empty() << "\n";
    }

    subsection("1b. Construction with dimensions, default fill (0.0)");
    {
        xll::Matrix m(3, 4);
        print_matrix("  3×4, fill=0.0:", m);
    }

    subsection("1c. Construction with dimensions and explicit fill");
    {
        xll::Matrix m(2, 5, 7.0);
        print_matrix("  2×5, fill=7.0:", m);
    }

    subsection("1d. 1×1 matrix (scalar)");
    {
        xll::Matrix m(1, 1, 42.0);
        print_matrix("  1×1, fill=42.0:", m);
        std::cout << "  size() = " << m.size() << "\n";
    }

    subsection("1e. Non-positive dimensions — matrix stays empty");
    {
        xll::Matrix m(0, 5);
        std::cout << "  Matrix(0, 5):  empty() = " << m.empty() << "\n";

        xll::Matrix n(-1, 3);
        std::cout << "  Matrix(-1, 3): empty() = " << n.empty() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 2: Element access and modification
// ---------------------------------------------------------------------------

void demo_element_access()
{
    separator("2. Element access and modification");

    subsection("2a. Read and write with operator[](row, col)");
    {
        // Build a 3×3 identity matrix
        xll::Matrix m(3, 3, 0.0);
        for (std::size_t i = 0; i < 3; ++i)
            m[i, i] = 1.0;
        print_matrix("  3×3 identity:", m);
    }

    subsection("2b. Row-major layout — filling via data() pointer");
    {
        xll::Matrix m(2, 3, 0.0);
        double* p = m.data();
        for (std::size_t i = 0; i < m.size(); ++i)
            p[i] = static_cast<double>(i + 1);
        print_matrix("  2×3 filled 1..6 via data():", m);
        // Verify operator[] reads the same values
        std::cout << "  m[0,0]=" << m[0uz, 0uz]
                  << "  m[0,2]=" << m[0uz, 2uz]
                  << "  m[1,2]=" << m[1uz, 2uz] << "\n";
    }

    subsection("2c. const element access");
    {
        const xll::Matrix m(2, 2, 3.14);
        std::cout << "  const m[0,0] = " << m[0uz, 0uz] << "\n";
        std::cout << "  const m[1,1] = " << m[1uz, 1uz] << "\n";
    }

    subsection("2d. Out-of-range access throws std::out_of_range");
    {
        xll::Matrix m(2, 3);
        try {
            (void)m[5uz, 0uz];
            std::cout << "  (should not reach here)\n";
        } catch (const std::out_of_range& e) {
            std::cout << "  caught: " << e.what() << "\n";
        }

        try {
            (void)m[0uz, 10uz];
            std::cout << "  (should not reach here)\n";
        } catch (const std::out_of_range& e) {
            std::cout << "  caught: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Section 3: Value semantics
// ---------------------------------------------------------------------------

void demo_value_semantics()
{
    separator("3. Value semantics");

    subsection("3a. Copy construction — deep copy");
    {
        xll::Matrix a(2, 2, 5.0);
        xll::Matrix b = a;         // deep copy
        b[0uz, 0uz] = 99.0;       // modify the copy
        print_matrix("  original a (unaffected):", a);
        print_matrix("  modified copy b:", b);
        std::cout << "  Same data pointer? "
                  << std::boolalpha << (a.data() == b.data()) << "\n";
    }

    subsection("3b. Move construction — O(1), no element copy");
    {
        xll::Matrix a(3, 3, 2.0);
        const double* original_data = a.data();
        xll::Matrix   b             = std::move(a);
        std::cout << "  After move: a.empty()               = " << a.empty()                   << "\n";
        std::cout << "  b holds the original allocation:      " << (b.data() == original_data) << "\n";
        print_matrix("  moved-into b:", b);
    }

    subsection("3c. Copy assignment");
    {
        xll::Matrix a(2, 2, 1.0);
        xll::Matrix b(4, 4, 9.0);
        b = a;
        std::cout << "  After b = a: b.rows()=" << b.rows()
                  << "  b.cols()=" << b.cols() << "\n";
        print_matrix("  b after copy-assignment:", b);
    }

    subsection("3d. Move assignment");
    {
        xll::Matrix a(2, 3, 6.0);
        xll::Matrix b;
        b = std::move(a);
        std::cout << "  After b = move(a): a.empty()=" << a.empty()
                  << "  b.size()=" << b.size() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 4: xll::MatrixBuffer — reading and writing FP12-compatible data
//
// MatrixBuffer objects cannot be constructed directly; they are obtained
// either from Excel (as FP12* cast to MatrixBuffer*) or from Matrix::get().
// This section simulates the "received from Excel" scenario by using a Matrix
// as the source of the MatrixBuffer pointer.
// ---------------------------------------------------------------------------

void demo_matrix_view_create()
{
    separator("4. xll::MatrixBuffer — reading and writing FP12-compatible data");

    subsection("4a. Obtain a MatrixBuffer via Matrix::get() and inspect it");
    {
        xll::Matrix mat(3, 2, 1.0);
        xll::MatrixBuffer* buf = mat.get();    // as if received from Excel
        print_view("  3×2, fill=1.0:", *buf);
    }

    subsection("4b. Write elements via operator[](row, col) on a MatrixBuffer");
    {
        xll::Matrix mat(2, 4, 0.0);
        xll::MatrixBuffer* buf = mat.get();
        for (std::size_t r = 0; r < static_cast<std::size_t>(buf->rows()); ++r)
            for (std::size_t c = 0; c < static_cast<std::size_t>(buf->cols()); ++c)
                (*buf)[r, c] = static_cast<double>(r * static_cast<std::size_t>(buf->cols()) + c + 1);
        print_view("  2×4, filled 1..8:", *buf);
        // Writes through buf are immediately visible via mat
        std::cout << "  mat[0,0]=" << mat[0uz, 0uz]
                  << "  mat[1,3]=" << mat[1uz, 3uz] << "\n";
    }

    subsection("4c. Out-of-range access on MatrixBuffer throws std::out_of_range");
    {
        xll::Matrix mat(2, 3, 0.0);
        try {
            (void)(*mat.get())[0uz, 99uz];
        } catch (const std::out_of_range& e) {
            std::cout << "  caught: " << e.what() << "\n";
        }
    }

    subsection("4d. Empty Matrix — get() returns nullptr");
    {
        xll::Matrix m(0, 5);
        std::cout << "  Matrix(0,  5).get() == nullptr: " << std::boolalpha << (m.get() == nullptr) << "\n";

        xll::Matrix n(-1, 3);
        std::cout << "  Matrix(-1, 3).get() == nullptr: " << std::boolalpha << (n.get() == nullptr) << "\n";
    }

    subsection("4e. Binary layout is identical to FP12");
    {
        xll::Matrix mat(2, 3, 9.0);

        // MatrixBuffer replicates the FP12 layout exactly, so a
        // reinterpret_cast between the two pointer types is safe.
        auto* fp12 = reinterpret_cast<const FP12*>(mat.get());
        std::cout << "  FP12.rows    = " << fp12->rows    << "\n";
        std::cout << "  FP12.columns = " << fp12->columns << "\n";
        std::cout << "  FP12.array[0]= " << fp12->array[0] << "\n";

        // Cast back the other way (simulates Excel handing the pointer to an XLL function)
        auto* roundtrip = reinterpret_cast<const xll::MatrixBuffer*>(fp12);
        std::cout << "  roundtrip rows()  = " << roundtrip->rows()  << "\n";
        std::cout << "  roundtrip cols()  = " << roundtrip->cols()  << "\n";
        std::cout << "  roundtrip [0,0]   = " << (*roundtrip)[0uz, 0uz] << "\n";
    }

    subsection("4f. Simulating an XLL function that receives and returns FP12*");
    {
        // Simulate the input that Excel would pass as FP12* → MatrixBuffer*
        xll::Matrix input_owner(2, 3, 0.0);
        xll::MatrixBuffer* input = input_owner.get();
        for (std::size_t r = 0; r < static_cast<std::size_t>(input->rows()); ++r)
            for (std::size_t c = 0; c < static_cast<std::size_t>(input->cols()); ++c)
                (*input)[r, c] = static_cast<double>(r * static_cast<std::size_t>(input->cols()) + c + 1);

        // Build the result — in a real XLL this would be thread_local
        thread_local xll::Matrix result;
        result = xll::Matrix(input->rows(), input->cols());
        for (std::size_t r = 0; r < static_cast<std::size_t>(input->rows()); ++r)
            for (std::size_t c = 0; c < static_cast<std::size_t>(input->cols()); ++c)
                result[r, c] = (*input)[r, c] * 2.0;

        print_view("  input:", *input);
        print_view("  result (×2):", *result.get());
        // In a real XLL: return reinterpret_cast<FP12*>(result.get());
    }
}

// ---------------------------------------------------------------------------
// Section 5: Matrix::get() — direct access to the underlying MatrixBuffer
// ---------------------------------------------------------------------------

void demo_get()
{
    separator("5. Matrix::get() — direct access to the underlying MatrixBuffer");

    subsection("5a. get() returns a pointer to the existing buffer — no copy");
    {
        xll::Matrix m(3, 3, 1.0);
        xll::MatrixBuffer* buf = m.get();
        std::cout << "  Same data pointer? "
                  << std::boolalpha << (m.data() == buf->data()) << "\n";

        // Modifications through get() are immediately visible in m
        (*buf)[0uz, 0uz] = 99.0;
        std::cout << "  m[0,0] after modifying via get(): " << m[0uz, 0uz] << "\n";
    }

    subsection("5b. FP12 binary compatibility — reinterpret_cast to FP12*");
    {
        xll::Matrix m(2, 3, 7.0);
        auto* fp12 = reinterpret_cast<const FP12*>(m.get());
        std::cout << "  FP12.rows    = " << fp12->rows    << "\n";
        std::cout << "  FP12.columns = " << fp12->columns << "\n";
        std::cout << "  FP12.array[0]= " << fp12->array[0] << "\n";
    }

    subsection("5c. get() returns nullptr for an empty matrix");
    {
        xll::Matrix m;
        std::cout << "  empty matrix get() == nullptr: "
                  << std::boolalpha << (m.get() == nullptr) << "\n";
    }

    subsection("5d. Thread-local pattern for returning to Excel");
    {
        // In a real XLL function result would be thread_local.
        // The matrix owns the MatrixBuffer directly, so get() returns
        // an FP12-compatible pointer with no intermediate copy.
        thread_local xll::Matrix result;
        result = xll::Matrix(2, 2, 0.0);
        result[0uz, 0uz] = 1.0;  result[0uz, 1uz] = 2.0;
        result[1uz, 0uz] = 3.0;  result[1uz, 1uz] = 4.0;

        // In a real XLL function this would be:
        // return reinterpret_cast<FP12*>(result.get());
        print_view("  Buffer ready to return to Excel:", *result.get());
    }
}

// ---------------------------------------------------------------------------
// Section 6: MatrixBuffer → Matrix implicit conversion
//
// In a real XLL, a MatrixBuffer* arrives as Excel's FP12* argument.
// Here we simulate that by obtaining a MatrixBuffer* from a source Matrix.
// ---------------------------------------------------------------------------

void demo_implicit_conversion()
{
    separator("6. MatrixBuffer -> Matrix implicit conversion");

    subsection("6a. Implicit conversion deep-copies all elements");
    {
        // Simulate a MatrixBuffer received from Excel
        xll::Matrix source(2, 3, 5.0);
        const xll::MatrixBuffer& from_excel = *source.get();

        xll::Matrix m = from_excel;   // implicit MatrixBuffer → Matrix deep copy
        m[0uz, 0uz] = 99.0;           // modify the copy

        print_view("  Original buffer (unchanged):", from_excel);
        print_matrix("  Converted + modified Matrix:", m);
        std::cout << "  Same memory? "
                  << std::boolalpha << (from_excel.data() == m.data()) << "\n";
    }

    subsection("6b. Function accepting xll::Matrix receives MatrixBuffer implicitly");
    {
        auto process = [](xll::Matrix mat) -> double {
            double sum = 0.0;
            for (std::size_t r = 0; r < static_cast<std::size_t>(mat.rows()); ++r)
                for (std::size_t c = 0; c < static_cast<std::size_t>(mat.cols()); ++c)
                    sum += mat[r, c];
            return sum;
        };

        xll::Matrix source(3, 3, 2.0);
        const double total = process(*source.get());  // implicit conversion at the call site
        std::cout << "  Sum of 3×3 matrix filled with 2.0 = " << total
                  << "  (expected 18.0)\n";
    }

    subsection("6c. Dimensions and data are preserved through the conversion");
    {
        xll::Matrix source(4, 5, 0.0);
        for (std::size_t r = 0; r < static_cast<std::size_t>(source.rows()); ++r)
            for (std::size_t c = 0; c < static_cast<std::size_t>(source.cols()); ++c)
                source[r, c] = static_cast<double>(r * static_cast<std::size_t>(source.cols()) + c);

        const xll::Matrix m = *source.get();   // implicit deep copy
        std::cout << "  rows=" << m.rows() << "  cols=" << m.cols()
                  << "  size=" << m.size() << "\n";
        std::cout << "  m[3,4] = " << m[3uz, 4uz]
                  << "  (expected " << (3 * 5 + 4) << ".0)\n";
    }
}

// ---------------------------------------------------------------------------
// Section 7: std::ranges — algorithms and views over flat element storage
//
// xll::Matrix satisfies std::ranges::contiguous_range.  Its iterator type is
// a raw double*, which is a std::contiguous_iterator.  Iteration is over all
// elements in row-major (flat) order.
// ---------------------------------------------------------------------------

void demo_ranges()
{
    separator("7. std::ranges — algorithms and views");

    subsection("7a. Range-based for loop");
    {
        xll::Matrix m(2, 3, 0.0);
        double v = 1.0;
        for (double& e : m)
            e = v++;
        print_matrix("  2×3, filled 1..6 via range-for:", m);
    }

    subsection("7b. std::ranges::fill");
    {
        xll::Matrix m(2, 4, 0.0);
        std::ranges::fill(m, 7.0);
        print_matrix("  2×4, filled 7.0 via ranges::fill:", m);
    }

    subsection("7c. std::ranges::transform — scale every element in-place");
    {
        xll::Matrix m(2, 3, 1.0);
        double v = 1.0;
        for (double& e : m) e = v++;         // fill 1..6
        std::ranges::transform(m, m.begin(), [](double x) { return x * 2.0; });
        print_matrix("  2×3, every element doubled:", m);
    }

    subsection("7d. std::ranges::sort — sort elements in ascending order");
    {
        xll::Matrix m(2, 3, 0.0);
        m[0uz, 0uz] = 5.0;  m[0uz, 1uz] = 1.0;  m[0uz, 2uz] = 4.0;
        m[1uz, 0uz] = 2.0;  m[1uz, 1uz] = 9.0;  m[1uz, 2uz] = 3.0;
        print_matrix("  before sort:", m);
        std::ranges::sort(m);
        print_matrix("  after  sort:", m);
    }

    subsection("7e. std::ranges::min_element and max_element");
    {
        xll::Matrix m(2, 3, 0.0);
        double v = 6.0;
        for (double& e : m) e = v--;         // fill 6..1
        const double lo = *std::ranges::min_element(m);
        const double hi = *std::ranges::max_element(m);
        std::cout << "  min = " << lo << "  (expected 1.0)\n";
        std::cout << "  max = " << hi << "  (expected 6.0)\n";
    }

    subsection("7f. std::ranges::fold_left — sum all elements (C++23)");
    {
        xll::Matrix m(3, 3, 1.0);          // nine ones
        const double sum = std::ranges::fold_left(m, 0.0, std::plus<>{});
        std::cout << "  sum of 3×3 matrix of 1.0 = " << sum
                  << "  (expected 9.0)\n";
    }

    subsection("7g. std::ranges::count_if");
    {
        xll::Matrix m(2, 4, 0.0);
        double v = 1.0;
        for (double& e : m) e = v++;        // fill 1..8
        const auto n = std::ranges::count_if(m, [](double x) { return x > 4.0; });
        std::cout << "  elements > 4.0 in 1..8: " << n
                  << "  (expected 4)\n";
    }

    subsection("7h. std::ranges::copy — to std::vector");
    {
        xll::Matrix m(2, 3, 0.0);
        double v = 1.0;
        for (double& e : m) e = v++;
        std::vector<double> vec;
        vec.reserve(m.size());
        std::ranges::copy(m, std::back_inserter(vec));
        std::cout << "  vector contents: ";
        for (double x : vec) std::cout << x << " ";
        std::cout << "\n";
    }

    subsection("7i. std::span — zero-copy view over matrix elements");
    {
        xll::Matrix m(2, 3, 0.0);
        double v = 1.0;
        for (double& e : m) e = v++;
        std::span<double> sp(m);            // constructed from begin()/end()
        std::cout << "  span size = " << sp.size() << "\n";
        std::cout << "  span[4]   = " << sp[4] << "  (expected 5.0)\n";

        // Modifications through the span are visible in the matrix
        sp[0] = 99.0;
        std::cout << "  m[0,0] after sp[0]=99: " << m[0uz, 0uz] << "\n";
    }

    subsection("7j. std::views pipeline — transform then filter");
    {
        xll::Matrix m(2, 4, 0.0);
        double v = 1.0;
        for (double& e : m) e = v++;        // fill 1..8

        // Double every element, then keep only those > 10
        auto pipeline = m
            | std::views::transform([](double x) { return x * 2.0; })
            | std::views::filter([](double x)    { return x > 10.0; });

        std::cout << "  elements after ×2 that are > 10: ";
        for (double x : pipeline) std::cout << x << " ";
        std::cout << "  (expected 12 14 16)\n";
    }
}

// ---------------------------------------------------------------------------
// Section 8: Row and column views — row() and col()
//
// row(r) returns a std::span<double> — a contiguous range over the cols()
// elements of that row.  col(c) returns a lazy transform range that maps
// row indices to the corresponding element via pointer arithmetic, giving
// exactly rows() elements.  Both are fully compatible with std::ranges.
// ---------------------------------------------------------------------------

void demo_row_col_views()
{
    separator("8. Row and column views — row() and col()");

    // Build a 4×5 matrix whose element [r,c] = r*5 + c + 1  (values 1 … 20)
    xll::Matrix m(4, 5, 0.0);
    for (std::size_t r = 0; r < 4; ++r)
        for (std::size_t c = 0; c < 5; ++c)
            m[r, c] = static_cast<double>(r * 5 + c + 1);
    print_matrix("  4×5 matrix (values 1…20):", m);

    subsection("8a. Iterate over a single row — range-based for");
    {
        std::cout << "  row(1): ";
        for (const double v : m.row(1))
            std::cout << v << " ";
        std::cout << "  (expected 6 7 8 9 10)\n";
    }

    subsection("8b. Mutate a row in-place");
    {
        // Zero out row 2 by writing through the span
        std::ranges::fill(m.row(2), 0.0);
        print_matrix("  after fill(row(2), 0):", m);

        // Restore row 2
        for (std::size_t c = 0; c < 5; ++c)
            m[2uz, c] = static_cast<double>(2 * 5 + c + 1);
    }

    subsection("8c. std::ranges algorithms on a row");
    {
        // row() returns std::span, which is a contiguous_range, so every
        // algorithm that works on the full matrix works on a single row too.
        const double sum = std::ranges::fold_left(m.row(0), 0.0, std::plus<>{});
        std::cout << "  sum of row(0) = " << sum << "  (expected 15.0)\n";

        const double hi = *std::ranges::max_element(m.row(3));
        std::cout << "  max of row(3) = " << hi  << "  (expected 20.0)\n";
    }

    subsection("8d. const row access");
    {
        const xll::Matrix& cm = m;
        std::cout << "  const row(1): ";
        for (const double v : cm.row(1))
            std::cout << v << " ";
        std::cout << "\n";
    }

    subsection("8e. Iterate over a single column — range-based for");
    {
        std::cout << "  col(0): ";
        for (const double v : m.col(0))
            std::cout << v << " ";
        std::cout << "  (expected 1 6 11 16)\n";
    }

    subsection("8f. Mutate a column in-place");
    {
        // Write through the range returned by col()
        double val = 0.0;
        for (double& v : m.col(4))
            v = val += 100.0;
        print_matrix("  after writing 100,200,300,400 into col(4):", m);

        // Restore col 4
        for (std::size_t r = 0; r < 4; ++r)
            m[r, 4uz] = static_cast<double>(r * 5 + 5);
    }

    subsection("8g. std::ranges algorithms on a column");
    {
        const double sum = std::ranges::fold_left(m.col(2), 0.0, std::plus<>{});
        std::cout << "  sum of col(2) = " << sum << "  (expected 3+8+13+18 = 42.0)\n";

        const auto n = std::ranges::count_if(m.col(1), [](double x) { return x > 9.0; });
        // Cast to ptrdiff_t: count_if returns range_difference_t, which for
        // iota_view<size_t> is __int128 on 64-bit Linux — ambiguous with <<.
        std::cout << "  elements > 9 in col(1): " << static_cast<std::ptrdiff_t>(n) << "  (expected 2)\n";
    }

    subsection("8h. const column access");
    {
        const xll::Matrix& cm = m;
        std::cout << "  const col(0): ";
        for (const double v : cm.col(0))
            std::cout << v << " ";
        std::cout << "\n";
    }

    subsection("8i. row() and col() throw on out-of-range index");
    {
        try {
            (void)m.row(99);
        } catch (const std::out_of_range& e) {
            std::cout << "  row(99) caught: " << e.what() << "\n";
        }
        try {
            (void)m.col(99);
        } catch (const std::out_of_range& e) {
            std::cout << "  col(99) caught: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    demo_matrix_construction();
    demo_element_access();
    demo_value_semantics();
    demo_matrix_view_create();
    demo_get();
    demo_implicit_conversion();
    demo_ranges();
    demo_row_col_views();

    std::cout << "\n=== Demo complete ===\n";
    return 0;
}

