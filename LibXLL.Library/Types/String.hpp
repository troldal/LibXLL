/**
 * @file String.hpp
 * @brief Excel-compatible UTF-8/wide-character string type for the xll library.
 *
 * Defines `xll::String`, a concrete Excel primitive type that wraps the
 * Pascal-style wide-character string format used by the Excel SDK (`xltypeStr`).
 *
 * **Memory layout**
 *
 * `xll::String` inherits from `impl::Base<String, xltypeStr>`, which in turn
 * inherits from `XLOPER12`. The object occupies exactly `sizeof(XLOPER12)` bytes
 * and can be passed directly to and from the Excel C API without any conversion.
 *
 * The internal buffer pointed to by `XLOPER12::val.str` follows the Excel
 * Pascal-string convention:
 * ```
 * val.str → [ length (XCHAR) | wide chars ... | NUL terminator (XCHAR) ]
 *              index 0          index 1 …          index length+1
 * ```
 * The length prefix counts wide characters (not bytes) and is capped at 65 535
 * per the modern Excel SDK specification.
 *
 * **Encoding**
 *
 * The public API accepts and returns UTF-8 encoded `std::string` / `std::string_view`.
 * Internally the buffer stores UTF-16 wide characters on Windows (`wchar_t` = 2 bytes)
 * and UTF-32 wide characters on other platforms (`wchar_t` = 4 bytes). Conversion is
 * performed transparently by `make_string()` and `to_string()`.
 *
 * **Ownership**
 *
 * `String` owns its wide-character buffer via a raw `new[]`/`delete[]` pair.
 * Move operations steal the pointer and null the source; copy operations allocate
 * a fresh independent buffer. The destructor always calls `delete[] val.str`,
 * which is a no-op when `val.str` is `nullptr`.
 *
 * **std::string-like interface**
 *
 * `xll::String` intentionally mirrors the `std::wstring` interface for size
 * semantics (`size()` and `length()` return wide-character counts) while
 * accepting and producing UTF-8 `std::string` values at its boundaries.
 *
 * @see impl::Base
 * @see xll::trim()
 * @see xll::to_upper()
 */

#pragma once

#include "Base.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <xlcall.hpp>
#include <ranges>
#include <algorithm>

#ifndef _WIN32
#include <utf8.h>
#endif

namespace xll
{
    /**
     * @brief Excel-compatible string type (`xltypeStr`).
     *
     * Represents an Excel string value stored in the Pascal wide-character format
     * required by the Excel C API. The class:
     * - Inherits from `impl::Base<String, xltypeStr>` (and therefore from `XLOPER12`)
     *   so its address can be reinterpret-cast to `XLOPER12*` without any offset.
     * - Owns a heap-allocated wide-character buffer whose layout matches what Excel
     *   expects: `[length][chars...][NUL]`.
     * - Exposes a UTF-8 (`std::string`) interface at all public boundaries.
     * - Supports concatenation, comparison, streaming, and trimming in a style
     *   analogous to `std::wstring`.
     *
     * @note `sizeof(xll::String) == sizeof(XLOPER12)` is a hard invariant enforced
     *       by a `static_assert` in the test suite.
     *
     * @note Excel's maximum string length is 65 535 wide characters. Attempting to
     *       construct a `String` from a longer value throws `std::length_error`.
     */
    class String : public impl::Base<String, xltypeStr>
    {
        using BASE = impl::Base<String, xltypeStr>;

    public:
        using BASE::BASE;

        // =====================================================================
        // Constructors
        // =====================================================================

        /**
         * @brief Default constructor — constructs an empty string.
         *
         * Delegates to `String("")`, producing a valid object whose Pascal buffer
         * contains a zero-length prefix and a NUL terminator. `empty()` returns
         * `true` on a default-constructed `String`.
         *
         * @post `xltype == xltypeStr`
         * @post `is_valid() == true`
         * @post `empty() == true`
         * @post `size() == 0`
         */
        constexpr String() : String("") {}

        /**
         * @brief Constructs from a null-terminated C string.
         *
         * Delegates to `String(std::string_view(str))`. The input is interpreted
         * as UTF-8.
         *
         * @param str Null-terminated UTF-8 string. Must not be `nullptr`.
         *
         * @throws std::runtime_error if the UTF-8 → wide-character conversion fails.
         * @throws std::length_error if the resulting wide-character count exceeds 65 535.
         */
        constexpr String(const char* str) : String(std::string_view(str)) {}    // NOLINT

        /**
         * @brief Constructs from a UTF-8 `std::string_view`.
         *
         * Calls `Base()` to zero-initialize the `XLOPER12` base and set
         * `xltype = xltypeStr`, then allocates a Pascal wide-character buffer via
         * `make_string()` and stores the raw pointer in `val.str`.
         *
         * @param str UTF-8 encoded string view. Need not be null-terminated;
         *            `str.size()` is used as the byte count.
         *
         * @throws std::runtime_error if the UTF-8 → wide-character conversion fails.
         * @throws std::length_error if the resulting wide-character count exceeds 65 535.
         *
         * @post `xltype == xltypeStr`
         * @post `is_valid() == true`
         * @post `to_string() == str` (for well-formed UTF-8 input)
         */
        constexpr String(std::string_view str) : Base()    // NOLINT
        {
            value() = make_string(str).release();
        }

        /**
         * @brief Constructs from a raw `XLOPER12` whose type must be `xltypeStr`.
         *
         * This constructor is `explicit` to prevent accidental implicit conversion
         * from arbitrary `XLOPER12` values. It is intended for use within the
         * library's internals when receiving raw Excel values known to hold strings.
         *
         * @param v Source `XLOPER12`. `v.xltype` must equal `xltypeStr`.
         *
         * @throws std::runtime_error (via `ensure`) if `v.xltype != xltypeStr`.
         * @throws std::runtime_error if the wide → UTF-8 → wide round-trip conversion fails.
         *
         * @post `xltype == xltypeStr`
         * @post `to_string()` returns the UTF-8 representation of `v.val.str`.
         */
        constexpr explicit String(const XLOPER12& v) : Base()
        {
            ensure(v.xltype == xltypeStr, "XLOPER12 type not convertible to String");
            value() = make_string(to_string(v.val.str)).release();
        }

        /**
         * @brief Copy constructor — deep-copies the wide-character buffer.
         *
         * Allocates a fresh independent buffer from `other.val.str`, so the two
         * objects do not share any heap memory after construction.
         *
         * @param other Source `String`. Must satisfy `other.is_valid()`.
         *
         * @throws std::runtime_error (via `ensure`) if `other` is not valid.
         * @throws std::runtime_error if the wide → UTF-8 → wide round-trip conversion fails.
         *
         * @post `to_string() == other.to_string()`
         * @post `val.str != other.val.str` (independent allocation)
         * @post `xltype == xltypeStr`
         */
        constexpr String(const String& other) : Base() // NOLINT
        {
            ensure(other.is_valid());
            ensure(base_xltype(xltype) == base_xltype(other.xltype));
            value() = make_string(to_string(other.val.str)).release();
        }

        /**
         * @brief Move constructor — steals the wide-character buffer.
         *
         * Transfers ownership of `other.val.str` to `*this` and sets
         * `other.val.str = nullptr`, leaving `other` in a valid, destructible
         * empty-like state.
         *
         * @param other Source `String` to move from.
         *
         * @post `val.str` is the pointer that was previously `other.val.str`.
         * @post `other.val.str == nullptr`
         * @post `xltype == xltypeStr`
         *
         * @note `noexcept` — no allocation is performed.
         */
        constexpr String(String&& other) noexcept : Base()
        {
            val.str = other.val.str;
            other.val.str = nullptr;
        }

        // =====================================================================
        // Destructor
        // =====================================================================

        /**
         * @brief Destructor — frees the wide-character buffer.
         *
         * Calls `delete[] val.str`. Because `delete[]` on a `nullptr` is a
         * well-defined no-op, this is safe for default-constructed, moved-from,
         * and `clear()`-ed objects alike.
         *
         * @note Does **not** check `is_valid()` — the buffer must always be freed
         *       regardless of `xltype` state to prevent leaks.
         */
        constexpr ~String()
        {
            delete[] val.str;
            val.str = nullptr;
        }

        // =====================================================================
        // Assignment operators
        // =====================================================================

        /**
         * @brief Copy assignment — deep-copies the wide-character buffer.
         *
         * Uses the copy-and-swap idiom: copies `other` into a temporary, then
         * swaps with `*this`. This provides strong exception safety — if the copy
         * throws, `*this` is unchanged.
         *
         * Self-assignment (`s = s`) is detected and short-circuited.
         *
         * @param other Source `String`. Must satisfy `is_valid()`.
         * @return Reference to `*this`.
         *
         * @throws std::runtime_error (via `ensure`) if either operand is not valid.
         * @throws std::runtime_error if the buffer copy conversion fails.
         */
        constexpr String& operator=(const String& other)
        {
            if (this == &other) return *this;

            ensure(is_valid());
            ensure(other.is_valid());
            ensure(base_xltype(xltype) == base_xltype(other.xltype));

            using xll::impl::swap;
            auto lhs = other;
            swap(*this, lhs);
            return *this;
        }

        /**
         * @brief Move assignment — exchanges buffers via swap.
         *
         * Swaps `*this` and `other`, so after the call `*this` holds `other`'s
         * old buffer and `other` holds `*this`'s old buffer (which is then
         * destroyed when `other` goes out of scope).
         *
         * Self-move-assignment is safe: swapping an object with itself is a no-op.
         *
         * @param other Source `String` to move from.
         * @return Reference to `*this`.
         *
         * @note `noexcept` — only swaps POD members of `XLOPER12`.
         * @note The moved-from object is left in a valid, destructible state
         *       holding the old buffer of `*this` (not `nullptr`).
         */
        constexpr String& operator=(String&& other) noexcept
        {
            using xll::impl::swap;
            swap(*this, other);
            return *this;
        }

        // =====================================================================
        // Conversion
        // =====================================================================

        /**
         * @brief Returns the string content as a UTF-8 `std::string`.
         *
         * Decodes the internal Pascal wide-character buffer via `to_string(val.str)`.
         * Returns an empty string for default-constructed or `clear()`-ed objects
         * (`val.str == nullptr`).
         *
         * @return UTF-8 encoded copy of the string content.
         *
         * @throws std::runtime_error if the wide → UTF-8 conversion fails.
         */
        [[nodiscard]] constexpr std::string to_string() const
        {
            return to_string(val.str);
        }

        /**
         * @brief Implicit conversion to `std::string`.
         *
         * Delegates to `to_string()`. Allows `xll::String` to be used wherever
         * a `std::string` is expected, e.g.:
         * ```cpp
         * std::string s = xll::String("hello");
         * ```
         *
         * @return UTF-8 encoded copy of the string content.
         */
        constexpr operator std::string() const
        {
            return to_string();
        }

        // =====================================================================
        // Concatenation operators
        // =====================================================================

        /**
         * @brief Concatenates two `String` values.
         *
         * Both operands are decoded to UTF-8, concatenated as `std::string`, and
         * a new `String` is constructed from the result. Neither operand is modified.
         * Self-concatenation (`s + s`) is safe.
         *
         * @param lhs Left operand.
         * @param rhs Right operand.
         * @return New `String` containing the concatenated content.
         *
         * @throws std::length_error if the result exceeds 65 535 wide characters.
         */
        constexpr friend String operator+(const String& lhs, const String& rhs)
        {
            ensure(lhs.is_valid());
            ensure(rhs.is_valid());
            const std::string result = to_string(lhs.value()) + to_string(rhs.value());
            return String(result); // NOLINT
        }

        /**
         * @brief Concatenates a `String` with a value convertible to `std::string`.
         *
         * `TOther` must be convertible to `std::string` and must not itself be an
         * `xll::String` or a type derived from it (those are handled by the
         * `String + String` overload above).
         *
         * Typical `TOther` types: `const char*`, `std::string`, `std::string_view`.
         *
         * @tparam TOther Right-hand operand type.
         * @param lhs Left operand (`xll::String`).
         * @param rhs Right operand (convertible to `std::string`).
         * @return New `String` containing the concatenated content.
         *
         * @throws std::length_error if the result exceeds 65 535 wide characters.
         */
        template<typename TOther>
            requires (!std::same_as<std::remove_cvref_t<TOther>, String>) &&
                     (!std::derived_from<std::remove_cvref_t<TOther>, String>) &&
                     std::convertible_to<TOther, std::string>
        constexpr friend String operator+(const String& lhs, TOther&& rhs)
        {
            ensure(lhs.is_valid());
            const std::string result = to_string(lhs.value()) + std::string(std::forward<TOther>(rhs));
            return String(result); // NOLINT
        }

        /**
         * @brief Concatenates a value convertible to `std::string` with a `String`.
         *
         * Reversed form of the templated `operator+` above, enabling expressions
         * such as `"prefix" + xll_string`. The same type constraints apply to
         * `TOther`.
         *
         * @tparam TOther Left-hand operand type.
         * @param lhs Left operand (convertible to `std::string`).
         * @param rhs Right operand (`xll::String`).
         * @return New `String` containing the concatenated content.
         *
         * @throws std::length_error if the result exceeds 65 535 wide characters.
         */
        template<typename TOther>
            requires (!std::same_as<std::remove_cvref_t<TOther>, String>) &&
                     (!std::derived_from<std::remove_cvref_t<TOther>, String>) &&
                     std::convertible_to<TOther, std::string>
        constexpr friend String operator+(TOther&& lhs, const String& rhs)
        {
            ensure(rhs.is_valid());
            const std::string result = std::string(std::forward<TOther>(lhs)) + to_string(rhs.value());
            return String(result); // NOLINT
        }

        // =====================================================================
        // Stream output
        // =====================================================================

        /**
         * @brief Writes the UTF-8 content of a `String` to an output stream.
         *
         * Decodes the Pascal wide-character buffer and writes the resulting UTF-8
         * text to `os`. Supports chaining (`os << a << b`).
         *
         * @param os Destination output stream.
         * @param str Source `String`. Must satisfy `str.is_valid()`.
         * @return Reference to `os`.
         *
         * @throws std::runtime_error (via `ensure`) if `str` is not valid.
         */
        friend std::ostream& operator<<(std::ostream& os, const String& str)
        {
            ensure(str.is_valid());
            os << to_string(str.val.str);
            return os;
        }

        // =====================================================================
        // Capacity
        // =====================================================================

        /**
         * @brief Returns `true` if the string contains no characters.
         *
         * Checks `val.str == nullptr` (moved-from / `clear()`-ed state) first,
         * then tests the Pascal length byte `val.str[0] == 0`.
         *
         * @return `true` if the string is empty, `false` otherwise.
         *
         * @throws std::runtime_error (via `ensure`) if `is_valid()` is `false`.
         *
         * @note Equivalent to `std::wstring::empty()`.
         */
        [[nodiscard]] constexpr bool empty() const
        {
            ensure(is_valid());
            if (val.str == nullptr) return true;
            return val.str[0] == 0;
            //return std::wstring_view(&val.str[1]).empty();
        }

        /**
         * @brief Returns the number of wide characters in the string.
         *
         * Returns `val.str[0]` — the Pascal length prefix — which counts
         * wide characters (not bytes). For ASCII content this equals the number
         * of visible characters; for non-ASCII content it is the number of
         * `wchar_t` code units.
         *
         * @return Number of wide characters, in the range [0, 65 535].
         *
         * @throws std::runtime_error (via `ensure`) if `is_valid()` is `false`.
         *
         * @note Returns 0 for `nullptr` buffers (moved-from / `clear()`-ed state).
         * @note Mirrors `std::wstring::size()`, not `std::string::size()`.
         */
        [[nodiscard]] constexpr size_t size() const
        {
            ensure(is_valid());
            if (val.str == nullptr) return 0;
            return static_cast<size_t>(val.str[0]);
            //return std::wstring_view(&val.str[1]).size();
        }

        /**
         * @brief Returns the number of wide characters in the string.
         *
         * Identical to `size()`. Provided for `std::wstring` interface parity,
         * where `length() == size()` is a guaranteed invariant.
         *
         * @return Number of wide characters, in the range [0, 65 535].
         *
         * @see size()
         */
        [[nodiscard]] constexpr size_t length() const
        {
            return size();
        }

        // =====================================================================
        // Modifiers
        // =====================================================================

        /**
         * @brief Erases the string content and frees the buffer.
         *
         * Calls `delete[] val.str` and sets `val.str = nullptr`. After this call
         * the object is in the same state as a moved-from `String`:
         * - `empty()` returns `true`
         * - `size()` returns 0
         * - The destructor is safe (deletes `nullptr`)
         * - Calling `clear()` again is safe (no double-free)
         *
         * @note Unlike `std::wstring::clear()`, this releases heap memory rather
         *       than retaining a zero-length allocation.
         */
        constexpr void clear()
        {
            delete[] val.str;
            val.str = nullptr;
        }

    private:
        // =====================================================================
        // Private helpers
        // =====================================================================

        /**
         * @brief Converts a Pascal wide-character buffer to a UTF-8 `std::string`.
         *
         * The buffer layout is `[length (XCHAR)][wide chars...][NUL terminator]`.
         * Returns an empty string immediately if `str` is `nullptr` or the length
         * byte is zero.
         *
         * **Platform behaviour:**
         * - **Windows:** uses `WideCharToMultiByte(CP_UTF8, ...)`.
         * - **Other:** assumes `sizeof(wchar_t) == sizeof(char32_t)` (enforced by
         *   `static_assert`) and uses the `utfcpp` library's `utf32to8()`.
         *
         * @param str Pointer to a Pascal-format wide-character buffer, or `nullptr`.
         * @return UTF-8 encoded string, or `""` if `str` is null/empty.
         *
         * @throws std::runtime_error if the wide → UTF-8 conversion fails
         *         (Windows: `WideCharToMultiByte` returns 0).
         */
        constexpr static std::string to_string(XCHAR const* str)
        {
#ifdef _WIN32

            if (not str or str[0] == L'\0') return "";

            const auto size = static_cast<size_t>(WideCharToMultiByte(CP_UTF8,    // Code page
                                           0,          // Flags
                                           &str[1],    // Source wide string
                                           str[0],     // Source length
                                           nullptr,    // Destination buffer (null for size calculation)
                                           0,          // Destination buffer size
                                           nullptr,    // Default char
                                           nullptr     // Used default char flag
            ));

            if (size == 0) throw std::runtime_error("String conversion failed");

            // Create the output string with the required size
            std::string text(size, '\0');

            // Convert the wide string to UTF-8
            if (WideCharToMultiByte(CP_UTF8, 0, &str[1], str[0], text.data(), static_cast<int>(size), nullptr, nullptr) == 0) {
                throw std::runtime_error("String conversion failed");
            }

            return text;
#else
            static_assert(sizeof(wchar_t) == sizeof(char32_t), "wchar_t and char32_t does not have the same size!");
            auto view = std::u32string(reinterpret_cast<const char32_t*>(&str[1]));
            auto result = utf8::utf32to8(view);
            return result;
#endif
        }

        /**
         * @brief Allocates a Pascal wide-character buffer from a UTF-8 string view.
         *
         * Converts the UTF-8 input to a heap-allocated buffer with the layout:
         * ```
         * [ length (XCHAR) | wide chars (sz × XCHAR) | NUL terminator (XCHAR) ]
         * ```
         * Total allocation: `(sz + 2) × sizeof(XCHAR)` bytes.
         *
         * For empty input a minimal 2-element buffer `[0][NUL]` is returned.
         *
         * **Platform behaviour:**
         * - **Windows:** uses `MultiByteToWideChar(CP_UTF8, ...)`.
         * - **Other:** uses the `utfcpp` library's `utf8to32()`.
         *
         * @param str UTF-8 encoded string view. Need not be null-terminated.
         * @return `unique_ptr<XCHAR[]>` owning the new buffer. Ownership must be
         *         released (via `.release()`) and stored in `val.str`.
         *
         * @throws std::runtime_error if the UTF-8 → wide conversion fails.
         * @throws std::length_error if the wide-character count exceeds 65 535.
         */
        constexpr static std::unique_ptr<XCHAR[]> make_string(std::string_view str)
        {
            if (str.empty()) {
                auto buffer = std::make_unique<XCHAR[]>(2);
                buffer[0] = 0;
                buffer[1] = 0;
                return buffer;
            }

        #ifdef _WIN32
            auto data = str.data();
            auto sz   = static_cast<size_t>(MultiByteToWideChar(CP_UTF8, 0, data, static_cast<int>(str.size()), nullptr, 0));
            if (sz == 0) throw std::runtime_error("String conversion failed");
            if (sz > 65535) throw std::length_error("String exceeds Excel maximum length of 65535 characters");

            auto buffer = std::make_unique<XCHAR[]>(sz + 2);
            buffer[0]      = static_cast<XCHAR>(sz);
            buffer[sz + 1] = 0;

            if (MultiByteToWideChar(CP_UTF8, 0, data, static_cast<int>(str.size()), &buffer[1], static_cast<int>(sz)) == 0)
                throw std::runtime_error("String conversion failed");

            return buffer;
        #else
            auto input  = std::string(str);
            auto output = utf8::utf8to32(input);
            auto sz     = output.size();

            if (sz > 65535) throw std::length_error("String exceeds Excel maximum length of 65535 characters");

            auto buffer = std::make_unique<XCHAR[]>(sz + 2);
            buffer[0]      = static_cast<XCHAR>(sz);
            buffer[sz + 1] = 0;

            for (size_t idx = 1; auto c : output)
                buffer[idx++] = static_cast<XCHAR>(c);

            return buffer;
        #endif
        }
    };

    namespace literals
    {
        /**
         * @brief User-defined literal that constructs an `xll::String`.
         *
         * Allows string literals to be used directly as `xll::String` values:
         * ```cpp
         * using namespace xll::literals;
         * auto s = "hello"_xs;   // xll::String
         * ```
         *
         * @param str Pointer to the literal's character data (UTF-8).
         * @param len Length of the literal in bytes (provided by the compiler).
         * @return `xll::String` constructed from the literal.
         */
        inline String operator""_xs(const char* str, std::size_t len) { return String(std::string_view(str, len)); }
    }    // namespace literals

    // =========================================================================
    // Equality operators (namespace scope)
    // =========================================================================
    //
    // Defined here rather than as hidden friends inside the class so that they
    // are visible to *unqualified lookup* (not just ADL). This is required for
    // Catch2's expression-decomposition macros (REQUIRE, CHECK, …) to resolve
    // the operator without needing xll::String on both sides of the expression.

    /**
     * @brief Equality comparison between two `String` values.
     *
     * Both operands are decoded to UTF-8 via `to_string()` and compared as
     * `std::string`. Self-comparison (`s == s`) is safe and always returns `true`.
     *
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return `true` if the strings have identical UTF-8 content.
     */
    inline bool operator==(const String& lhs, const String& rhs)
    {
        return lhs.to_string() == rhs.to_string();
    }

    /**
     * @brief Equality comparison between a `String` and a value convertible to
     *        `std::string`.
     *
     * `TOther` must be convertible to `std::string` and must not itself be
     * `xll::String` or a type derived from it. The right operand is explicitly
     * converted to `std::string` before comparison to ensure correctness with
     * `std::string_view` on all C++ standard versions.
     *
     * @tparam TOther Right-hand operand type (e.g. `const char*`, `std::string`,
     *                `std::string_view`).
     * @param lhs Left operand (`xll::String`).
     * @param rhs Right operand.
     * @return `true` if the decoded content of `lhs` equals `std::string(rhs)`.
     *
     * @note C++20 symmetry rewrites provide the `rhs == lhs` direction automatically.
     */
    template<typename TOther>
        requires (!std::same_as<std::remove_cvref_t<TOther>, String>) &&
                 (!std::derived_from<std::remove_cvref_t<TOther>, String>) &&
                 std::convertible_to<TOther, std::string>
    inline bool operator==(const String& lhs, TOther&& rhs)
    {
        return lhs.to_string() == std::string(std::forward<TOther>(rhs));
    }

    // =========================================================================
    // Three-way comparison operators (namespace scope)
    // =========================================================================

    /**
     * @brief Three-way comparison between two `String` values.
     *
     * Both operands are decoded to UTF-8 and compared lexicographically as
     * `std::string`. Returns `std::strong_ordering`.
     *
     * C++20 automatically synthesises `<`, `<=`, `>`, `>=` from this operator.
     *
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return `std::strong_ordering` result of the lexicographic comparison.
     */
    inline auto operator<=>(const String& lhs, const String& rhs)
    {
        return lhs.to_string() <=> rhs.to_string();
    }

    /**
     * @brief Three-way comparison between a `String` and a value convertible to
     *        `std::string`.
     *
     * The right operand is explicitly converted to `std::string` before the
     * comparison. The same type constraints as the templated `operator==` apply
     * to `TOther`.
     *
     * @tparam TOther Right-hand operand type.
     * @param lhs Left operand (`xll::String`).
     * @param rhs Right operand (convertible to `std::string`).
     * @return `std::strong_ordering` result of the lexicographic comparison.
     */
    template<typename TOther>
        requires (!std::same_as<std::remove_cvref_t<TOther>, String>) &&
                 (!std::derived_from<std::remove_cvref_t<TOther>, String>) &&
                 std::convertible_to<TOther, std::string>
    inline auto operator<=>(const String& lhs, TOther&& rhs)
    {
        return lhs.to_string() <=> std::string(std::forward<TOther>(rhs));
    }

    // =========================================================================
    // Free utility functions
    // =========================================================================

    /**
     * @brief Returns a copy of `str` with leading and trailing ASCII whitespace
     *        removed.
     *
     * Whitespace is defined by `std::isspace` with an `unsigned char` cast, which
     * covers the ASCII characters HT (`\t`), LF (`\n`), VT (`\v`), FF (`\f`),
     * CR (`\r`), and SP (` `). Unicode-only whitespace (e.g. U+00A0 non-breaking
     * space) is **not** trimmed.
     *
     * Interior whitespace is preserved. An all-whitespace input produces an empty
     * `String`.
     *
     * @param str The `String` to trim.
     * @return A new `String` containing the trimmed content.
     *
     * @note Does not modify `str`.
     */
    constexpr inline xll::String trim(const xll::String& str)
    {
        auto s = str.to_string();

        // Find the first non-whitespace character.
        auto start = std::ranges::find_if_not(s, [](unsigned char ch) { return std::isspace(ch); });

        // If the string is entirely whitespace, return an empty string.
        if (start == s.end()) {
            return "";
        }

        // Find the last non-whitespace character.
        auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();

        // Construct the trimmed string.
        std::string result(start, end);

        return String(result); // NOLINT
    }

    /**
     * @brief Returns a copy of `str` with all ASCII letters converted to uppercase.
     *
     * Uses `std::toupper` with an `unsigned char` cast to avoid undefined behaviour
     * on signed `char`. Non-ASCII multi-byte UTF-8 sequences pass through unchanged
     * (their bytes are all ≥ 0x80 and are not affected by `std::toupper`). Unicode
     * letters outside the ASCII range (e.g. `é`, `ü`) are **not** uppercased.
     *
     * @param str The `String` to convert.
     * @return A new `String` with ASCII letters uppercased.
     *
     * @note Does not modify `str`.
     */
    constexpr inline xll::String to_upper(const xll::String& str)
    {
        std::string result = str.to_string();
        std::ranges::transform(result, result.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

        return String(result); // NOLINT
    }

}    // namespace xll

/**
 * @brief `std::formatter` specialisation for `xll::String`.
 *
 * Enables `xll::String` to be used with `std::format` and `std::print`:
 * ```cpp
 * xll::String s("hello");
 * std::string formatted = std::format("Value: {}", s);  // "Value: hello"
 * ```
 *
 * Delegates to `std::formatter<std::string>` after converting via `to_string()`,
 * so all standard string format specifiers (width, fill, alignment) are supported.
 */
template<>
struct std::formatter<xll::String> : std::formatter<std::string> {
    auto format(const xll::String& str, std::format_context& ctx) const {
        return std::formatter<std::string>::format(str.to_string(), ctx);
    }
};
