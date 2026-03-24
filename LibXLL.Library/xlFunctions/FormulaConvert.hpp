//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Optional.hpp"
#include "../Types/String.hpp"
#include "../Utils/Concepts.hpp"
#include <xlcall.hpp>

#include <type_traits>

namespace xll
{
    // =========================================================================
    // Notation tag types
    // =========================================================================

    /// Tag type representing A1 cell-reference notation (e.g. `=A1+B2`).
    struct A1 {};

    /// Tag type representing R1C1 cell-reference notation (e.g. `=R1C1+R2C2`).
    struct R1C1 {};

    // =========================================================================
    // Direction wrappers
    // =========================================================================

    /// Wraps a notation tag to express the *source* notation of a formula.
    /// Usage: `From<A1>`, `From<R1C1>`
    template<typename TNotation>
    struct From { using notation = TNotation; };

    /// Wraps a notation tag to express the *target* notation of a formula.
    /// Usage: `To<A1>`, `To<R1C1>`
    template<typename TNotation>
    struct To { using notation = TNotation; };

    // =========================================================================
    // Internal traits
    // =========================================================================

    namespace impl
    {
        template<typename T>
        inline constexpr bool is_from_tag = false;
        template<typename N>
        inline constexpr bool is_from_tag<From<N>> = true;

        template<typename T>
        inline constexpr bool is_to_tag = false;
        template<typename N>
        inline constexpr bool is_to_tag<To<N>> = true;

        template<typename T>
        inline constexpr bool notation_is_a1 = std::is_same_v<T, A1>;
    }

    template<typename T>
    concept IsFromNotation = impl::is_from_tag<T>;

    template<typename T>
    concept IsToNotation = impl::is_to_tag<T>;

    // =========================================================================
    // RefStyle — absolute/relative styling of references
    // =========================================================================

    /**
     * @brief Controls how cell references are styled in the converted formula.
     *
     * Mirrors Excel VBA's `XlReferenceStyle` constants passed to
     * `xlfFormulaConvert` as the `to_ref_type` argument.
     *
     * | Value         | A1 example | Column | Row      |
     * |---------------|------------|--------|----------|
     * | Unchanged     | —          | kept   | kept     |
     * | Absolute      | `$A$1`     | pinned | pinned   |
     * | AbsRow        | `A$1`      | free   | pinned   |
     * | AbsCol        | `$A1`      | pinned | free     |
     * | Relative      | `A1`       | free   | free     |
     */
    enum class RefStyle : int {
        Unchanged = 0,  ///< preserve the existing absolute/relative style (omits the argument)
        Absolute  = 1,  ///< `$A$1` — column and row both absolute
        AbsRow    = 2,  ///< `A$1`  — row absolute, column relative (xlAbsRowRelColumn)
        AbsCol    = 3,  ///< `$A1`  — column absolute, row relative (xlRelRowAbsColumn)
        Relative  = 4,  ///< `A1`   — column and row both relative
    };

    // =========================================================================
    // xll::convert_formula  —  wraps xlfFormulaConvert (id 241)
    // =========================================================================
    //
    // Converts cell or range references inside a text formula between A1 and
    // R1C1 notation and/or between absolute and relative addressing.  The
    // formula must be syntactically valid and must start with '='.
    //
    // The source and target notations are expressed as template parameters:
    //
    //   convert_formula<From<A1>,   To<R1C1>>(formula)
    //   convert_formula<From<R1C1>, To<A1>  >(formula)
    //   convert_formula<From<A1>,   To<A1>  >(formula)  // no-op style round-trip
    //
    // Optional positional arguments give additional control:
    //
    //   convert_formula<From<A1>, To<R1C1>>(formula, RefStyle::Absolute)
    //       → also converts the absolute/relative type of the references.
    //
    //   convert_formula<From<A1>, To<R1C1>>(formula, RefStyle::Absolute, rel_ref)
    //       → full conversion; rel_ref provides the origin cell for R1C1
    //         relative references (must be xltypeSRef or xltypeRef).
    //
    // @note Callable from commands and macro sheet functions only.
    // =========================================================================

    // -------------------------------------------------------------------------
    // Overload 1: formula only — style conversion, no abs/rel change
    // -------------------------------------------------------------------------

    /**
     * @brief Converts the reference notation in @p formula.
     *
     * @tparam TFrom  Source notation — `From<A1>` or `From<R1C1>`.
     * @tparam TTo    Target notation — `To<A1>`   or `To<R1C1>`.
     *
     * @param formula  A valid Excel formula string starting with `'='`.
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     *
     * @code
     * auto r = xll::convert_formula<From<A1>, To<R1C1>>(xll::String("=A1+B2"));
     * @endcode
     */
    template<IsFromNotation TFrom, IsToNotation TTo = To<typename TFrom::notation>>
    inline Optional<String> convert_formula(const String& formula)
    {
        Bool from_a1(impl::notation_is_a1<typename TFrom::notation>);
        Bool to_a1(impl::notation_is_a1<typename TTo::notation>);

        XLOPER12 raw {};
        if (Excel12(xlfFormulaConvert, &raw, 3,
                    static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                    static_cast<LPXLOPER12>(&from_a1),
                    static_cast<LPXLOPER12>(&to_a1)) != xlretSuccess ||
            raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

    // -------------------------------------------------------------------------
    // Overload 2: + to_ref_type — style and abs/rel conversion
    // -------------------------------------------------------------------------

    /**
     * @brief Converts the reference notation and absolute/relative style in @p formula.
     *
     * @tparam TFrom  Source notation — `From<A1>` or `From<R1C1>`.
     * @tparam TTo    Target notation — `To<A1>`   or `To<R1C1>`.
     *
     * @param formula   A valid Excel formula string starting with `'='`.
     * @param ref_style Target absolute/relative style:
     *                  - `RefStyle::Absolute` — `$A$1`
     *                  - `RefStyle::AbsRow`   — `A$1`  (row pinned)
     *                  - `RefStyle::AbsCol`   — `$A1`  (column pinned)
     *                  - `RefStyle::Relative` — `A1`
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     */
    template<IsFromNotation TFrom, IsToNotation TTo = To<typename TFrom::notation>>
    inline Optional<String> convert_formula(const String& formula, RefStyle ref_style)
    {
        Bool from_a1(impl::notation_is_a1<typename TFrom::notation>);
        Bool to_a1(impl::notation_is_a1<typename TTo::notation>);

        XLOPER12 raw {};
        int ret = xlretSuccess;

        if (ref_style == RefStyle::Unchanged) {
            ret = Excel12(xlfFormulaConvert, &raw, 3,
                          static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                          static_cast<LPXLOPER12>(&from_a1),
                          static_cast<LPXLOPER12>(&to_a1));
        }
        else {
            Int ref_type(static_cast<int>(ref_style));
            ret = Excel12(xlfFormulaConvert, &raw, 4,
                          static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                          static_cast<LPXLOPER12>(&from_a1),
                          static_cast<LPXLOPER12>(&to_a1),
                          static_cast<LPXLOPER12>(&ref_type));
        }

        if (ret != xlretSuccess || raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

    // -------------------------------------------------------------------------
    // Overload 3: + rel_ref — full conversion with R1C1 relative-reference origin
    // -------------------------------------------------------------------------

    /**
     * @brief Full reference conversion including an R1C1 relative-reference origin.
     *
     * @tparam TFrom  Source notation — `From<A1>` or `From<R1C1>`.
     * @tparam TTo    Target notation — `To<A1>`   or `To<R1C1>`.
     * @tparam TRef   The reference type — must satisfy `is_xll_type` and carry
     *                `xltypeSRef` or `xltypeRef` semantics.
     *
     * @param formula    A valid Excel formula string starting with `'='`.
     * @param ref_style  Target absolute/relative style (see `RefStyle`).
     * @param rel_ref    Origin cell for R1C1 relative references.
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     */
    template<IsFromNotation TFrom, IsToNotation TTo = To<typename TFrom::notation>, typename TRef>
        requires is_xll_type<TRef>
    inline Optional<String> convert_formula(const String& formula,
                                             RefStyle      ref_style,
                                             const TRef&   rel_ref)
    {
        Bool from_a1(impl::notation_is_a1<typename TFrom::notation>);
        Bool to_a1(impl::notation_is_a1<typename TTo::notation>);

        XLOPER12 raw {};
        int ret = xlretSuccess;

        if (ref_style == RefStyle::Unchanged) {
            Missing missing;
            ret = Excel12(xlfFormulaConvert, &raw, 5,
                          static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                          static_cast<LPXLOPER12>(&from_a1),
                          static_cast<LPXLOPER12>(&to_a1),
                          static_cast<LPXLOPER12>(&missing),
                          static_cast<LPXLOPER12>(const_cast<TRef*>(&rel_ref)));
        }
        else {
            Int ref_type(static_cast<int>(ref_style));
            ret = Excel12(xlfFormulaConvert, &raw, 5,
                          static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                          static_cast<LPXLOPER12>(&from_a1),
                          static_cast<LPXLOPER12>(&to_a1),
                          static_cast<LPXLOPER12>(&ref_type),
                          static_cast<LPXLOPER12>(const_cast<TRef*>(&rel_ref)));
        }

        if (ret != xlretSuccess || raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll

