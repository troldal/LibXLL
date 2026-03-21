//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Optional.hpp"
#include "../Types/String.hpp"
#include "../Utils/Concepts.hpp"
#include <xlcall.hpp>

namespace xll
{
    // =========================================================================
    // xll::formula_convert  —  wraps xlfFormulaConvert (id 241)
    // =========================================================================
    //
    // Converts cell or range references inside a text formula between A1 and
    // R1C1 notation and/or between absolute and relative addressing.  The
    // formula must be syntactically valid and must start with '='.
    //
    // Four overloads are provided, each accepting one more optional argument
    // than the previous.  The overload to use is determined by how many of
    // the optional conversion parameters are needed:
    //
    //   formula_convert(formula, from_a1)
    //       → keeps the same reference style; no abs/rel conversion.
    //
    //   formula_convert(formula, from_a1, to_a1)
    //       → converts between A1 and R1C1; no abs/rel conversion.
    //
    //   formula_convert(formula, from_a1, to_a1, to_ref_type)
    //       → converts style and abs/rel type (to_ref_type: 1–4, see below).
    //
    //   formula_convert(formula, from_a1, to_a1, to_ref_type, rel_ref)
    //       → full conversion; rel_ref provides the origin cell for R1C1
    //         relative references (must be xltypeSRef or xltypeRef).
    //
    // to_ref_type values:
    //   1 = row and column absolute  (e.g. $A$1)
    //   2 = row absolute only        (e.g. $A1)
    //   3 = column absolute only     (e.g. A$1)
    //   4 = row and column relative  (e.g. A1)
    //
    // @note Callable from commands and macro sheet functions only.
    // =========================================================================

    // -------------------------------------------------------------------------
    // 2-arg overload: formula + from_a1
    // Style unchanged; no absolute/relative conversion.
    // -------------------------------------------------------------------------

    /**
     * @brief Converts references in @p formula without changing style or
     *        absolute/relative type.
     *
     * Invokes `xlfFormulaConvert` with two arguments, leaving the reference
     * style (A1 or R1C1) and the absolute/relative type unchanged.
     *
     * @param formula  A valid Excel formula string starting with `'='`,
     *                 e.g. `"=A1"` or `"=R1C1"`.
     * @param from_a1  `true` if @p formula uses A1 notation; `false` for R1C1.
     *
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     */
    inline Optional<String> convert_formula(const String& formula, Bool from_a1)
    {
        XLOPER12 raw {};
        if (Excel12(xlfFormulaConvert, &raw, 2,
                    static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                    static_cast<LPXLOPER12>(&from_a1)) != xlretSuccess ||
            raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

    // -------------------------------------------------------------------------
    // 3-arg overload: + to_a1
    // Converts between A1 and R1C1; no absolute/relative conversion.
    // -------------------------------------------------------------------------

    /**
     * @brief Converts the reference style in @p formula.
     *
     * Invokes `xlfFormulaConvert` with three arguments, converting between A1
     * and R1C1 notation.  The absolute/relative type of the references is not
     * changed.
     *
     * @param formula  A valid Excel formula string starting with `'='`.
     * @param from_a1  `true` if @p formula uses A1 notation; `false` for R1C1.
     * @param to_a1    `true` to produce A1 notation; `false` for R1C1.
     *
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     */
    inline Optional<String> convert_formula(const String& formula,
                                             Bool          from_a1,
                                             Bool          to_a1)
    {
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
    // 4-arg overload: + to_ref_type
    // Converts style and absolute/relative type.
    // -------------------------------------------------------------------------

    /**
     * @brief Converts the reference style and absolute/relative type in @p formula.
     *
     * Invokes `xlfFormulaConvert` with four arguments.
     *
     * @param formula      A valid Excel formula string starting with `'='`.
     * @param from_a1      `true` if @p formula uses A1 notation; `false` for R1C1.
     * @param to_a1        `true` to produce A1 notation; `false` for R1C1.
     * @param to_ref_type  Absolute/relative target type (must be 1–4):
     *                     - 1: row and column absolute  (`$A$1`)
     *                     - 2: row absolute only        (`$A1`)
     *                     - 3: column absolute only     (`A$1`)
     *                     - 4: row and column relative  (`A1`)
     *
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     */
    inline Optional<String> convert_formula(const String& formula,
                                             Bool          from_a1,
                                             Bool          to_a1,
                                             Int           to_ref_type)
    {
        XLOPER12 raw {};
        if (Excel12(xlfFormulaConvert, &raw, 4,
                    static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                    static_cast<LPXLOPER12>(&from_a1),
                    static_cast<LPXLOPER12>(&to_a1),
                    static_cast<LPXLOPER12>(&to_ref_type)) != xlretSuccess ||
            raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

    // -------------------------------------------------------------------------
    // 5-arg overload: + rel_ref  (template over SingleRef / MultiRef)
    // Full conversion; rel_ref provides the R1C1 relative-reference origin.
    // -------------------------------------------------------------------------

    /**
     * @brief Full reference conversion including an R1C1 relative-reference origin.
     *
     * Invokes `xlfFormulaConvert` with all five arguments.  The @p rel_ref
     * argument is required when the output uses R1C1 notation and contains
     * relative references: it specifies the cell that relative row/column
     * offsets are computed from.
     *
     * @tparam TRef       The reference type — must be `xll::SingleRef`
     *                    (`xltypeSRef`) or `xll::MultiRef` (`xltypeRef`).
     *
     * @param formula      A valid Excel formula string starting with `'='`.
     * @param from_a1      `true` if @p formula uses A1 notation; `false` for R1C1.
     * @param to_a1        `true` to produce A1 notation; `false` for R1C1.
     * @param to_ref_type  Absolute/relative target type (1–4, see 4-arg overload).
     * @param rel_ref      Origin cell for R1C1 relative references.  Must be an
     *                     `xltypeSRef` or `xltypeRef` operand.
     *
     * @return The converted formula as an `xll::String`, or `xll::None` on failure.
     */
    template<typename TRef>
        requires is_xll_type<TRef>
    inline Optional<String> convert_formula(const String& formula,
                                             Bool          from_a1,
                                             Bool          to_a1,
                                             Int           to_ref_type,
                                             const TRef&   rel_ref)
    {
        XLOPER12 raw {};
        if (Excel12(xlfFormulaConvert, &raw, 5,
                    static_cast<LPXLOPER12>(const_cast<String*>(&formula)),
                    static_cast<LPXLOPER12>(&from_a1),
                    static_cast<LPXLOPER12>(&to_a1),
                    static_cast<LPXLOPER12>(&to_ref_type),
                    static_cast<LPXLOPER12>(const_cast<TRef*>(&rel_ref))) != xlretSuccess ||
            raw.xltype != xltypeStr)
            return None;

        String result(raw);
        Excel12(xlFree, nullptr, 1, &raw);
        return result;
    }

}    // namespace xll

