/**
 * @file Metadata.hpp
 * @brief Internal helpers for reading and writing the 2-byte metadata stored in the
 *        unused tail of the XLOPER12.val union.
 *
 * These utilities are shared by xll::Expected and xll::Optional (and any future type that
 * uses the same metadata scheme).  They are not part of the public API.
 *
 * **Metadata Layout:**
 * ```
 * XLOPER12.val (8 bytes):
 * [0][1][2][3][4][5][6][7]
 *  ^                 ^  ^
 *  |                 |  +-- Byte [-1]: State flag (0x00 = value/engaged, 0x01 = error/disengaged)
 *  |                 +---- Byte [-2]: Magic sentinel (0xE7)
 *  +-- Used by actual data (pointer, int, double, …)
 * ```
 *
 * Using std::byte* is explicitly allowed by the C++ standard (std::byte is exempt from
 * strict-aliasing rules), making these functions safe for type-punning into the union.
 *
 * @author Kenneth Troldal Balslev
 * @date 24/02/2026
 */

#pragma once

#include "../ExcelSDK/xlcall.hpp"
#include <cstddef>

namespace xll::impl
{
    /// Magic sentinel byte that identifies an XLOPER12 tagged with our metadata.
    inline constexpr std::byte kMagic   = std::byte{0xE7};
    inline constexpr std::byte kIsError = std::byte{1};   ///< State flag: error / disengaged
    inline constexpr std::byte kIsValue = std::byte{0};   ///< State flag: value / engaged

    /**
     * @brief Returns a pointer to the last 2 bytes of XLOPER12.val (the metadata area).
     */
    inline std::byte* metadata_bytes(XLOPER12& x) noexcept
    {
        auto* b = reinterpret_cast<std::byte*>(&x.val);
        return b + sizeof(x.val) - 2;
    }

    /**
     * @brief Const-qualified overload of metadata_bytes.
     */
    inline const std::byte* metadata_bytes(const XLOPER12& x) noexcept
    {
        const auto* b = reinterpret_cast<const std::byte*>(&x.val);
        return b + sizeof(x.val) - 2;
    }

    /**
     * @brief Returns true if the magic sentinel is present in @p x.
     *
     * A raw XLOPER12 from Excel will not have the sentinel set, so this distinguishes
     * objects we constructed from objects Excel handed to us.
     */
    inline bool has_metadata(const XLOPER12& x) noexcept
    {
        return metadata_bytes(x)[0] == kMagic;
    }

    /**
     * @brief Writes the magic sentinel and the error/value state flag into @p x.
     *
     * @param x        The XLOPER12 to tag.
     * @param is_error true  → error state (disengaged for Optional, error for Expected)
     *                 false → value state (engaged  for Optional, value  for Expected)
     */
    inline void set_error_state(XLOPER12& x, bool is_error) noexcept
    {
        auto* tag = metadata_bytes(x);
        tag[0] = kMagic;
        tag[1] = is_error ? kIsError : kIsValue;
    }

    /**
     * @brief Returns true if the metadata indicates the error / disengaged state.
     *
     * Returns false (i.e. "value state") for objects that carry no metadata.
     */
    inline bool is_error_state(const XLOPER12& x) noexcept
    {
        const auto* tag = metadata_bytes(x);
        return (tag[0] == kMagic) && (tag[1] == kIsError);
    }

    /**
     * @brief Zeroes the metadata bytes so that Excel sees a plain XLOPER12.
     *
     * Must be called before passing the object to Excel APIs.
     */
    inline void clear_metadata(XLOPER12& x) noexcept
    {
        auto* tag = metadata_bytes(x);
        tag[0] = std::byte{0};
        tag[1] = std::byte{0};
    }

    /**
     * @brief Returns true when the xltype of @p x does NOT match TValue::excel_type.
     *
     * Used as a fallback when no metadata is present (raw XLOPER12 from Excel).
     *
     * @tparam TValue The expected value type (must have a static excel_type member).
     */
    template<typename TValue>
    inline bool is_error_from_xltype(const XLOPER12& x) noexcept
    {
        return (x.xltype & TValue::excel_type) == 0;
    }

} // namespace xll::impl

