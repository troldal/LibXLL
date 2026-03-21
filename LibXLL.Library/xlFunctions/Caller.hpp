//
// Created by kenne on 21/03/2026.
//

#pragma once

#include "../Types/Error.hpp"
#include "../Types/MultiRef.hpp"
#include "../Types/SheetId.hpp"
#include "../Types/SingleRef.hpp"
#include "../Types/String.hpp"

#include <span>
#include <variant>
#include <xlcall.hpp>

namespace xll
{
    // =========================================================================
    // Caller context types
    // =========================================================================

    /**
     * @brief Caller context: a command attached to a menu bar.
     *
     * Produced when xlfCaller returns a horizontal 4-element array.
     * All positions are 1-based, matching the Excel C API convention.
     */
    struct MenuBarCaller
    {
        int position;          ///< 1-based position of the command on its menu.
        int menu_number;       ///< 1-based menu number on the menu bar.
        int menu_bar_number;   ///< 1-based menu bar number.
        int submenu_position;  ///< 1-based position on the submenu, or 0 if
                               ///< the command is not on a submenu.
    };

    /**
     * @brief Caller context: a command attached to a toolbar.
     *
     * Produced when xlfCaller returns a horizontal 2-element array.
     * The position is 1-based.
     */
    struct ToolbarCaller
    {
        int         position;      ///< 1-based position of the button on the toolbar.
        xll::String toolbar_name;  ///< Name of the toolbar.
    };

    // =========================================================================
    // CallerInfo
    // =========================================================================

    /**
     * @brief Discriminated union over all possible xlfCaller return contexts.
     *
     * | Active alternative | Trigger context                                            |
     * |--------------------|------------------------------------------------------------|
     * | xll::SingleRef     | Worksheet cell, multi-cell array formula, or trapped        |
     * |                    | data-entry / double-click event on the current sheet.       |
     * | xll::MultiRef      | Cross-sheet or multi-area range reference.                  |
     * | MenuBarCaller      | Command on a menu bar item.                                 |
     * | ToolbarCaller      | Button on a toolbar.                                        |
     * | xll::String        | Command attached to a control object (carries the object    |
     * |                    | ID string).                                                 |
     * | xll::Error         | Any other context (normally #REF!), or an unexpected type.  |
     *
     * Use std::visit with xll::overload{...} to dispatch on the active alternative.
     */
    using CallerInfo = std::variant<
        xll::SingleRef,
        xll::MultiRef,
        MenuBarCaller,
        ToolbarCaller,
        xll::String,
        xll::Error
    >;

    // =========================================================================
    // caller()
    // =========================================================================

    /**
     * @brief Returns information about the entity that triggered the current DLL call.
     *
     * Invokes xlfCaller (id 89) via Excel12(), parses the raw XLOPER12 result into
     * a CallerInfo value, and releases any Excel-owned memory with xlFree before
     * returning.  The function can be called multiple times within the same DLL
     * invocation; it will return the same information every time.
     *
     * **Memory management**
     *
     * Any heap memory that Excel allocated in the returned XLOPER12 (indicated by
     * `xlbitXLFree` being set on `xltype`) is deep-copied into the CallerInfo before
     * xlFree is called.  Callers do not need to manage the raw XLOPER12.
     *
     * **xltypeMulti disambiguation**
     *
     * Both the menu bar and toolbar contexts return `xltypeMulti` arrays.  They are
     * told apart by the array shape:
     *   - 1 row × 4 columns → MenuBarCaller [position, menu_num, bar_num, submenu_pos]
     *   - 1 row × 2 columns → ToolbarCaller [position, toolbar_name]
     *
     * @warning Do **not** call this function from `DllMain()` or any OS callback.
     *          Calling xlfCaller outside an active Excel callback context produces
     *          undefined and potentially harmful behaviour.
     *
     * @return A CallerInfo whose active alternative identifies the caller.
     *         Position numbers in MenuBarCaller and ToolbarCaller count from 1.
     */
    inline CallerInfo caller()
    {
        XLOPER12 result {};
        if (Excel12(xlfCaller, &result, 0) != xlretSuccess)
            return xll::ErrValue;   // Should only happen outside an Excel callback

        constexpr auto MASK = static_cast<int>(~(xlbitDLLFree | xlbitXLFree));
        const int eff = static_cast<int>(result.xltype) & MASK;

        // Helper: read element i of the xltypeMulti array as a plain int.
        // Excel returns menu/toolbar positions as xltypeInt or xltypeNum.
        auto elem_int = [&](int i) -> int
        {
            const XLOPER12& e  = result.val.array.lparray[i];
            const int       et = static_cast<int>(e.xltype) & MASK;
            if (et == xltypeInt) return e.val.w;
            if (et == xltypeNum) return static_cast<int>(e.val.num);
            return 0;
        };

        // Default: "other" caller context returns #REF! per the Excel documentation.
        CallerInfo info = xll::ErrRef;

        switch (eff)
        {
        case xltypeSRef:
        {
            // Single-block range on the current sheet.
            // xltypeSRef is stored entirely inline — Excel never sets xlbitXLFree on it.
            const XLREF12& r = result.val.sref.ref;
            info = xll::SingleRef(r.rwFirst, r.rwLast, r.colFirst, r.colLast);
            break;
        }

        case xltypeRef:
        {
            // Multi-area or cross-sheet range. Deep-copy before xlFree.
            if (result.val.mref.lpmref)
            {
                const WORD n = result.val.mref.lpmref->count;
                info = xll::MultiRef(
                    xll::SheetId(static_cast<uint64_t>(result.val.mref.idSheet)),
                    std::span<const XLREF12>(result.val.mref.lpmref->reftbl, n));
            }
            else
            {
                // Sheet-identity-only ref (degenerate but handle gracefully).
                info = xll::MultiRef(
                    xll::SheetId(static_cast<uint64_t>(result.val.mref.idSheet)));
            }
            break;
        }

        case xltypeMulti:
        {
            // Disambiguate menu bar (1×4) from toolbar (1×2) by array shape.
            const int rows  = result.val.array.rows;
            const int total = rows * result.val.array.columns;

            if (rows == 1 && total == 4)
            {
                // Menu bar: [position, menu_number, menu_bar_number, submenu_position]
                // All values are 1-based; submenu_position is 0 when not in a submenu.
                info = MenuBarCaller{
                    elem_int(0),   // position
                    elem_int(1),   // menu_number
                    elem_int(2),   // menu_bar_number
                    elem_int(3),   // submenu_position (0 = not in a submenu)
                };
            }
            else if (rows == 1 && total == 2)
            {
                // Toolbar: [position, toolbar_name]
                // xll::String(const XLOPER12&) deep-copies the XCHAR* buffer.
                const XLOPER12& name_oper = result.val.array.lparray[1];
                const int       name_et   = static_cast<int>(name_oper.xltype) & MASK;

                xll::String name = (name_et == xltypeStr && name_oper.val.str)
                                       ? xll::String(name_oper)
                                       : xll::String();

                info = ToolbarCaller{ elem_int(0), std::move(name) };
            }
            // Unexpected shape → leave info as xll::ErrRef.
            break;
        }

        case xltypeStr:
        {
            // Command attached to a control object: the string is the object ID.
            // xll::String(const XLOPER12&) performs a deep copy of the XCHAR* buffer.
            info = xll::String(result);
            break;
        }

        case xltypeErr:
        {
            // Excel already signalled an error (typically xlerrRef).
            // Preserve the exact error code rather than substituting ErrRef.
            XLOPER12 e{};
            e.xltype  = xltypeErr;
            e.val.err = result.val.err;
            info = xll::Error(e);
            break;
        }

        default:
            // Unexpected xltype — the ErrRef default is already in info.
            break;
        }

        // Release any Excel-allocated memory.  xlFree is always safe to call:
        // Excel no-ops when xlbitXLFree is not set, and nulls any freed pointers
        // to prevent double-frees.
        // Do NOT call xlFree on individual elements of an xltypeMulti array —
        // only the array XLOPER12 itself should be freed.
        Excel12(xlFree, nullptr, 1, &result);

        return info;
    }

}    // namespace xll

