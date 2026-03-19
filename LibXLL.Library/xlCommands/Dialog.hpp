// Dialog.hpp
//
// Fluent builder API over the Excel xlfDialogBox custom dialog facility.
//
// Each dialog is defined as a 7-column table whose first row describes the
// dialog window and whose subsequent rows describe individual controls.
// This header encapsulates that low-level representation behind a set of
// named item factories and a Dialog builder class, making dialog construction
// declarative and type-safe.
//
// Typical usage
// -------------
//   auto result = xll::dialog::Dialog("Logon")
//       .Size(372, 200)
//       .Add(xll::dialog::OkButton("OK").At(50, 170).Size(90, 90))
//       .Add(xll::dialog::CancelButton("Cancel").At(150, 170).Size(90, 90))
//       .Add(xll::dialog::HelpButton("Help").At(250, 170).Size(90, 90)
//                .Url("https://example.com"))
//       .Add(xll::dialog::Text("Please enter your credentials").At(40, 10))
//       .Add(xll::dialog::GroupBox().At(40, 35).Size(290, 100))
//       .Add(xll::dialog::Text("Username").At(50, 53))
//       .Add(xll::dialog::TextBox("MyName").At(150, 50))
//       .Add(xll::dialog::Text("Password").At(50, 73))
//       .Add(xll::dialog::TextBox("**********").At(150, 70))
//       .Add(xll::dialog::CheckBox("Remember credentials").At(50, 110))
//       .show();
//
//   if (result) {
//       auto username = xll::cast<xll::String>(result[2]);  // TextBox index 2
//       auto remember = xll::cast<xll::Bool>(result[5]);    // CheckBox index 5
//   }

#pragma once

#include "../ExcelSDK/xlcall.hpp"
#include "../Types/Any.hpp"
#include "../Types/Array.hpp"
#include "../Types/Bool.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"
#include "../Types/String.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace xll
{
    namespace dialog
    {

        // =====================================================================
        // Item type identifiers (Table 8.32 from the SDK documentation)
        // =====================================================================

        enum class ItemType : int
        {
            OkButton      = 1,   ///< Default OK button (triggered by Enter).
            CancelButton  = 2,   ///< Cancel button (terminates dialog, returns false).
            OkDefault     = 3,   ///< Non-default OK button.
            CancelDefault = 4,   ///< Default Cancel button.
            Text          = 5,   ///< Static text label.
            TextBox       = 6,   ///< Single-line text edit box.
            IntBox        = 7,   ///< Integer-only edit box.
            FloatBox      = 8,   ///< Floating-point edit box.
            FormulaBox    = 9,   ///< Formula edit box.
            ReferenceBox  = 10,  ///< Reference edit box.
            RadioGroup    = 11,  ///< Radio button group container (must precede RadioButtons).
            RadioButton   = 12,  ///< Radio button (must immediately follow a RadioGroup).
            CheckBox      = 13,  ///< Check box.
            GroupBox      = 14,  ///< Visual group box border (no data).
            ListBox       = 15,  ///< Scrollable list box.
            LinkedListBox = 16,  ///< List box linked to the preceding edit box.
            Icons         = 17,  ///< Icon image.
            LinkedFileBox = 18,  ///< Linked file list box.
            LinkedPathBox = 19,  ///< Linked path box (must immediately follow LinkedFileBox).
            DirectoryText = 20,  ///< Directory text box.
            DropDownList  = 21,  ///< Drop-down list box.
            DropDownCombo = 22,  ///< Drop-down combo box (linked to preceding edit box).
            PictureButton = 23,  ///< Picture button.
            HelpButton    = 24,  ///< Help button (column 7 holds the URL with a `!0` suffix).
        };

        // =====================================================================
        // Internal: one row in the 7-column dialog definition table
        // =====================================================================

        struct Item
        {
            int                        type_id {};
            std::optional<double>      x {};       ///< Horizontal position (screen units).
            std::optional<double>      y {};       ///< Vertical position (screen units).
            std::optional<double>      w {};       ///< Width (screen units). Nil → inherit previous.
            std::optional<double>      h {};       ///< Height (screen units). Nil → inherit previous.
            std::optional<std::string> text {};    ///< Column 6: label / list source.
            std::optional<xll::Any>    value {};   ///< Column 7: initial value / URL / result.

            /// Serialises the item to 7 consecutive xll::Any cells.
            void appendTo(std::vector<xll::Any>& cells) const
            {
                cells.emplace_back(xll::Number { static_cast<double>(type_id) });
                cells.emplace_back(x ? xll::Any { xll::Number { *x } } : xll::Any { xll::Nil {} });
                cells.emplace_back(y ? xll::Any { xll::Number { *y } } : xll::Any { xll::Nil {} });
                cells.emplace_back(w ? xll::Any { xll::Number { *w } } : xll::Any { xll::Nil {} });
                cells.emplace_back(h ? xll::Any { xll::Number { *h } } : xll::Any { xll::Nil {} });
                cells.emplace_back(text  ? xll::Any { xll::String(*text) } : xll::Any { xll::Nil {} });
                cells.emplace_back(value ? *value                          : xll::Any { xll::Nil {} });
            }
        };

        // =====================================================================
        // Fluent item builder
        // =====================================================================

        class ItemSpec
        {
        public:
            explicit ItemSpec(ItemType t) : m_item { static_cast<int>(t) } {}

            /// Sets the (x, y) screen-unit position.
            ItemSpec& At(double x, double y)
            {
                m_item.x = x;
                m_item.y = y;
                return *this;
            }

            /// Sets the (width, height) in screen units.
            ItemSpec& Size(double w, double h)
            {
                m_item.w = w;
                m_item.h = h;
                return *this;
            }

            /// Sets the text label / caption.
            ItemSpec& Label(std::string_view t)
            {
                m_item.text = std::string(t);
                return *this;
            }

            /// Sets the initial value for edit boxes, check boxes, list boxes, etc.
            ItemSpec& Initial(xll::Any v)
            {
                m_item.value = std::move(v);
                return *this;
            }

            /// Sets the help URL for a HelpButton item.
            /// The `!0` suffix required by Excel is appended automatically if absent.
            ItemSpec& Url(std::string_view url)
            {
                std::string u(url);
                if (u.size() < 2 || u.compare(u.size() - 2, 2, "!0") != 0)
                    u += "!0";
                m_item.value = xll::Any { xll::String(u) };
                return *this;
            }

            /// Adds 100 to the item type — Excel returns control to the DLL when
            /// this item is clicked, keeping the dialog open.
            /// Does not work for edit boxes (6–10), group boxes (14), the help
            /// button (24), or picture buttons (23).
            ItemSpec& Trigger()
            {
                m_item.type_id += 100;
                return *this;
            }

            /// Adds 200 to the item type — the item is shown grey and disabled.
            ItemSpec& Disabled()
            {
                m_item.type_id += 200;
                return *this;
            }

            [[nodiscard]] const Item& item() const noexcept { return m_item; }

        private:
            Item m_item;
        };

        // =====================================================================
        // Named factory functions
        // =====================================================================

        // --- Buttons ---------------------------------------------------------

        /// Default OK button (item 1). Activated by pressing Enter.
        inline ItemSpec OkButton(std::string_view label = "OK")
        { return ItemSpec(ItemType::OkButton).Label(label); }

        /// Cancel button (item 2). Dismisses the dialog and causes show() to
        /// return a Result with accepted == false.
        inline ItemSpec CancelButton(std::string_view label = "Cancel")
        { return ItemSpec(ItemType::CancelButton).Label(label); }

        /// Non-default OK button (item 3).
        inline ItemSpec OkDefault(std::string_view label = "OK")
        { return ItemSpec(ItemType::OkDefault).Label(label); }

        /// Default Cancel button (item 4). Activated by pressing Enter.
        inline ItemSpec CancelDefault(std::string_view label = "Cancel")
        { return ItemSpec(ItemType::CancelDefault).Label(label); }

        /// Help button (item 24). Call .Url("https://…") to attach a URL.
        inline ItemSpec HelpButton(std::string_view label = "Help")
        { return ItemSpec(ItemType::HelpButton).Label(label); }

        // --- Static text -----------------------------------------------------

        /// Static text label (item 5).
        inline ItemSpec Text(std::string_view content)
        { return ItemSpec(ItemType::Text).Label(content); }

        // --- Edit boxes ------------------------------------------------------

        /// Single-line text edit box (item 6).
        inline ItemSpec TextBox(std::string_view initial = "")
        { return ItemSpec(ItemType::TextBox).Initial(xll::Any { xll::String(std::string(initial)) }); }

        /// Integer edit box (item 7).
        inline ItemSpec IntBox(int initial = 0)
        { return ItemSpec(ItemType::IntBox).Initial(xll::Any { xll::Number { static_cast<double>(initial) } }); }

        /// Floating-point edit box (item 8).
        inline ItemSpec FloatBox(double initial = 0.0)
        { return ItemSpec(ItemType::FloatBox).Initial(xll::Any { xll::Number { initial } }); }

        /// Formula edit box (item 9).
        inline ItemSpec FormulaBox(std::string_view initial = "")
        { return ItemSpec(ItemType::FormulaBox).Initial(xll::Any { xll::String(std::string(initial)) }); }

        /// Reference edit box (item 10).
        inline ItemSpec ReferenceBox(std::string_view initial = "")
        { return ItemSpec(ItemType::ReferenceBox).Initial(xll::Any { xll::String(std::string(initial)) }); }

        // --- Layout / grouping -----------------------------------------------

        /// Visual group box border (item 14). Has no interactive result value.
        inline ItemSpec GroupBox(std::string_view label = "")
        { return ItemSpec(ItemType::GroupBox).Label(label); }

        /// Radio button group container (item 11).
        /// Must be placed immediately before a run of RadioButton items.
        /// Omitting the label suppresses the border.
        inline ItemSpec RadioGroup(std::string_view label = "")
        { return ItemSpec(ItemType::RadioGroup).Label(label); }

        /// Radio button (item 12).
        /// Must appear immediately inside a RadioGroup without interruption.
        inline ItemSpec RadioButton(std::string_view label)
        { return ItemSpec(ItemType::RadioButton).Label(label); }

        // --- Check box -------------------------------------------------------

        /// Check box (item 13).
        /// @param initial  Initial checked state.
        inline ItemSpec CheckBox(std::string_view label, bool initial = false)
        {
            return ItemSpec(ItemType::CheckBox)
                .Label(label)
                .Initial(xll::Any { xll::Bool { initial } });
        }

        // --- List boxes ------------------------------------------------------

        /// Scrollable list box (item 15).
        /// @param items  A range name or a string literal array such as
        ///               `"{\"A\",\"B\",\"C\"}"`.
        inline ItemSpec ListBox(std::string_view items)
        { return ItemSpec(ItemType::ListBox).Label(items); }

        /// List box linked to the immediately preceding edit box (item 16).
        inline ItemSpec LinkedListBox(std::string_view items)
        { return ItemSpec(ItemType::LinkedListBox).Label(items); }

        /// Drop-down list box (item 21). Same semantics as ListBox but only
        /// expands when selected.
        inline ItemSpec DropDownList(std::string_view items)
        { return ItemSpec(ItemType::DropDownList).Label(items); }

        /// Drop-down combo box (item 22). Must be preceded by an edit box.
        /// Returns the selected position in the combo's column 7 and the typed
        /// text in the preceding edit box's column 7.
        inline ItemSpec DropDownCombo(std::string_view items)
        { return ItemSpec(ItemType::DropDownCombo).Label(items); }

        // =====================================================================
        // Dialog result
        // =====================================================================

        struct Result
        {
            /// true  — a non-Cancel button was pressed.
            /// false — Cancel was pressed or the dialog was dismissed.
            bool accepted = false;

            /// 1-based row offset (within the definition table) of the button
            /// that was pressed to close the dialog.  Zero when !accepted.
            int buttonOffset = 0;

            /// Column-7 values for each item row, in the order items were added
            /// via Dialog::Add().  These are deep copies taken before the
            /// Excel-allocated result array is freed, so they remain valid after
            /// show() returns.
            std::vector<xll::Any> values {};

            /// Zero-based indexed access to item result values.
            [[nodiscard]] const xll::Any& operator[](std::size_t i) const { return values[i]; }

            /// Implicit bool: true when the dialog was accepted.
            explicit operator bool() const noexcept { return accepted; }
        };

        // =====================================================================
        // Dialog builder
        // =====================================================================

        class Dialog
        {
        public:
            /// Constructs a dialog with the given title bar text.
            explicit Dialog(std::string_view title = "") : m_title(title) {}

            /// Sets the dialog title bar text.
            Dialog& Title(std::string_view t)
            {
                m_title = std::string(t);
                return *this;
            }

            /// Sets the dialog client area size (screen units).
            /// Screen units are based on the system fixed-width font:
            /// 8 units wide and 12 units high per character.
            Dialog& Size(double w, double h)
            {
                m_w = w;
                m_h = h;
                return *this;
            }

            /// Sets the dialog position on screen (screen units from top-left).
            /// Omitting this lets Excel choose a default position.
            Dialog& Pos(double x, double y)
            {
                m_x = x;
                m_y = y;
                return *this;
            }

            /// Sets the 1-based row offset of the item that receives focus when
            /// the dialog opens.  Omitting this uses Excel's default.
            Dialog& DefaultItem(int pos)
            {
                m_defaultItem = pos;
                return *this;
            }

            /// Appends a control item.
            Dialog& Add(ItemSpec spec)
            {
                m_items.push_back(spec.item());
                return *this;
            }

            /// Displays the dialog and blocks until the user dismisses it.
            /// Returns a Result whose `accepted` member is false if the user
            /// pressed Cancel (or closed the dialog without confirming).
            [[nodiscard]] Result show() const;

        private:
            std::string        m_title {};
            double             m_x = 0, m_y = 0;
            double             m_w = 0, m_h = 0;
            std::optional<int> m_defaultItem {};
            std::vector<Item>  m_items {};

            [[nodiscard]] xll::Array<xll::Any> toArray() const;
        };

        // =====================================================================
        // Dialog::toArray — assembles the 7-column definition array
        // =====================================================================

        inline xll::Array<xll::Any> Dialog::toArray() const
        {
            const std::size_t rowCount = 1 + m_items.size();
            std::vector<xll::Any> cells;
            cells.reserve(7 * rowCount);

            // ------------------------------------------------------------------
            // Row 0: dialog header
            //   Col 1–3 : blank (Nil)
            //   Col 4   : dialog width  (or Nil → Excel default)
            //   Col 5   : dialog height (or Nil → Excel default)
            //   Col 6   : title string
            //   Col 7   : 1-based default-item offset (or Nil)
            // ------------------------------------------------------------------
            cells.emplace_back(xll::Nil {});
            cells.emplace_back(xll::Nil {});
            cells.emplace_back(xll::Nil {});
            cells.emplace_back(m_w > 0 ? xll::Any { xll::Number { m_w } } : xll::Any { xll::Nil {} });
            cells.emplace_back(m_h > 0 ? xll::Any { xll::Number { m_h } } : xll::Any { xll::Nil {} });
            cells.emplace_back(xll::String(m_title));
            cells.emplace_back(m_defaultItem
                ? xll::Any { xll::Number { static_cast<double>(*m_defaultItem) } }
                : xll::Any { xll::Nil {} });

            // ------------------------------------------------------------------
            // Rows 1…n: control items
            // ------------------------------------------------------------------
            for (const auto& item : m_items)
                item.appendTo(cells);

            return xll::Array<xll::Any>(
                cells,
                typename xll::Array<xll::Any>::TwoDimensional { rowCount, 7 });
        }

        // =====================================================================
        // Dialog::show — calls xlfDialogBox and parses the result
        // =====================================================================

        inline Result Dialog::show() const
        {
            auto defn = toArray();

            XLOPER12 res {};
            Excel12(xlfDialogBox, &res, 1, &defn);

            Result r;

            // Excel returns xltypeBool / FALSE when Cancel is pressed.
            if (res.xltype == xltypeBool && res.val.xbool == 0) {
                Excel12(xlFree, nullptr, 1, &res);
                return r;   // r.accepted == false
            }

            r.accepted = true;

            if (res.xltype == xltypeMulti && res.val.array.lparray != nullptr) {
                const XLOPER12* lp = res.val.array.lparray;

                // Row 0, column 6 (0-based index 6) → 1-based row offset of the
                // button that was pressed.
                xll::Any btnCell(lp[6]);
                if (auto v = xll::cast<xll::Number>(btnCell); v)
                    r.buttonOffset = static_cast<int>(static_cast<double>(*v));

                // Rows 1…n, column 6 (0-based index 7*row+6) → per-item results.
                // Values are deep-copied via xll::Any(const XLOPER12&) before
                // xlFree is called so that string buffers remain valid.
                r.values.reserve(m_items.size());
                for (std::size_t i = 0; i < m_items.size(); ++i)
                    r.values.emplace_back(lp[7 * (i + 1) + 6]);
            }

            Excel12(xlFree, nullptr, 1, &res);
            return r;
        }

    }    // namespace dialog
}    // namespace xll

