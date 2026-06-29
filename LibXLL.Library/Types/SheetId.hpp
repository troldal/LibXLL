/**
 * @file SheetId.hpp
 * @brief Excel-compatible sheet-identity type for the xll library.
 *
 * Defines `xll::SheetId`, a thin wrapper around the `xltypeRef` variant of
 * `XLOPER12` that carries **only** a sheet identifier (`val.mref.idSheet`).
 * The `val.mref.lpmref` pointer is always `nullptr`; no rectangular reference
 * blocks are stored.
 *
 * **Purpose**
 *
 * - Replaces direct use of the `IDSHEET` / `DWORD_PTR` typedef at call sites,
 *   making sheet-identity values first-class objects with a clear type.
 * - Satisfies `is_xll_type`, so it can be wrapped in `xll::Optional<SheetId>`,
 *   `xll::Expected<SheetId, …>`, `xll::Variant<…, SheetId, …>`, etc.
 * - Mirrors the result that Excel's `xlSheetId` C API function stores in an
 *   `xltypeRef` operand whose `lpmref` field is `nullptr`.
 *
 * **Memory layout**
 *
 * `SheetId` inherits from `XLOPER12` and adds no data members:
 * `sizeof(SheetId) == sizeof(XLOPER12)`.  It can be passed directly to and
 * from Excel C API functions that expect an `LPXLOPER12`.
 *
 * **Conversions**
 *
 * `SheetId` is *implicitly* convertible to `IDSHEET` (the underlying Excel SDK
 * type, which is a 64-bit unsigned integer on both Windows and Linux).  On
 * Windows, where `IDSHEET` (`DWORD_PTR` / `unsigned __int64`) and
 * `std::uint64_t` (`unsigned long long`) are technically distinct types,
 * additional comparison overloads accepting `std::uint64_t` are provided.
 *
 * @see xll::MultiRef   — carries an `IDSHEET` together with XLREF12 blocks.
 * @see xll::Optional   — for nullable sheet IDs returned from lookups.
 */

#pragma once

#include "../Utils/Ensure.hpp"
#include <compare>
#include <xlcall.hpp>

namespace xll
{
    /**
     * @brief Sheet-identity value (`xltypeRef` with `lpmref == nullptr`).
     *
     * Wraps a single `IDSHEET` integer stored in `XLOPER12::val.mref.idSheet`.
     * `val.mref.lpmref` is invariantly `nullptr` — this type represents an
     * identity, not a cell range.
     *
     * @note `sizeof(xll::SheetId) == sizeof(XLOPER12)` is a hard invariant.
     * @note No virtual functions — the vtable-corruption static assertions in
     *       `xll::Optional` etc. are satisfied.
     */
    class SheetId : public XLOPER12
    {
        using xltype_t = decltype(XLOPER12::xltype);

        static constexpr xltype_t TYPE_MASK =
            ~static_cast<xltype_t>(xlbitDLLFree | xlbitXLFree);

    public:
        /// The Excel type tag recognised by `is_xll_type` and `xll::Optional`.
        static constexpr size_t excel_type = xltypeRef;

        // ---- Constructors ----

        /**
         * @brief Default constructor — ID value 0.
         *
         * @post `xltype == xltypeRef`
         * @post `val.mref.lpmref == nullptr`
         * @post `val.mref.idSheet == 0`
         * @post `is_valid() == true`
         */
        SheetId() : XLOPER12()
        {
            xltype           = xltypeRef;
            val.mref.lpmref  = nullptr;
            val.mref.idSheet = 0;
        }

        /**
         * @brief Constructs a `SheetId` from a raw sheet-identifier value.
         *
         * The constructor is `explicit` to prevent accidental implicit construction
         * from arbitrary integers.
         *
         * @param id  The internal Excel sheet identifier.
         */
        explicit SheetId(uint64_t id) : SheetId()
        {
            val.mref.idSheet = id;
        }

        /**
         * @brief Constructs a `SheetId` from an `xltypeRef` `XLOPER12` whose
         *        `lpmref` is `nullptr` (the form returned by `xlSheetId`).
         *
         * @param op  Source operand.  Must have `xltype == xltypeRef` and
         *            `val.mref.lpmref == nullptr`.
         *
         * @throws std::runtime_error (via `ensure`) if preconditions are violated.
         */
        explicit SheetId(const XLOPER12& op) : SheetId()
        {
            XLL_ENSURE((op.xltype & TYPE_MASK) == xltypeRef,
                   "SheetId: source XLOPER12 is not xltypeRef");
            XLL_ENSURE(op.val.mref.lpmref == nullptr,
                   "SheetId: source xltypeRef has a non-null lpmref — use xll::MultiRef instead");
            val.mref.idSheet = op.val.mref.idSheet;
        }

        // Compiler-generated copy / move / destructor are correct (no heap ownership).

        // ---- Validation ----

        /**
         * @brief Returns `true` when the object is in a valid state.
         *
         * Valid means `xltype == xltypeRef` (ignoring memory-management bits) and
         * `val.mref.lpmref == nullptr`.
         */
        [[nodiscard]] bool is_valid() const noexcept
        {
            return (xltype & TYPE_MASK) == xltypeRef && val.mref.lpmref == nullptr;
        }

        // ---- Value access ----

        /**
         * @brief Implicit conversion to `IDSHEET`.
         *
         * `IDSHEET` is `unsigned long long` on Linux and `DWORD_PTR`
         * (`unsigned __int64`) on Windows — both are 64-bit unsigned integers.
         * The conversion is *implicit* so that a `SheetId` can be passed wherever
         * an `IDSHEET` is expected without an explicit cast.
         */
        operator uint64_t() const noexcept { return val.mref.idSheet; }    // NOLINT(*-explicit-constructor)

        // ---- Comparison operators ----

        /// Equality between two `SheetId` objects.
        friend bool operator==(const SheetId& lhs, const SheetId& rhs) noexcept
        {
            return lhs.val.mref.idSheet == rhs.val.mref.idSheet;
        }

        /// Three-way comparison between two `SheetId` objects.
        friend std::strong_ordering operator<=>(const SheetId& lhs,
                                                const SheetId& rhs) noexcept
        {
            return lhs.val.mref.idSheet <=> rhs.val.mref.idSheet;
        }

        /// Equality with a raw `IDSHEET` value.
        friend bool operator==(const SheetId& lhs, uint64_t rhs) noexcept
        {
            return lhs.val.mref.idSheet == rhs;
        }

        /// Three-way comparison with a raw `IDSHEET` value.
        friend std::strong_ordering operator<=>(const SheetId& lhs, uint64_t rhs) noexcept
        {
            return lhs.val.mref.idSheet <=> rhs;
        }

        // ---- Swap ----

        /// ADL-enabled swap — no heap allocation, exchanges ID values directly.
        friend void swap(SheetId& lhs, SheetId& rhs) noexcept
        {
            using std::swap;
            swap(lhs.xltype,          rhs.xltype);
            swap(lhs.val.mref.idSheet, rhs.val.mref.idSheet);
            // lpmref is invariantly nullptr — no need to swap.
        }
    };

}    // namespace xll






