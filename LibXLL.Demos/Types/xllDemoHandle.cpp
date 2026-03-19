// xllDemoHandle.cpp
//
// Demonstrates xll::Handle<T>: persisting a user-defined struct as a binary
// name that is saved with the Excel workbook.
//
// The binary name "AppSettings" is attached to the active worksheet.  All
// Handle operations (store / load / exists / remove) are invoked from command
// macros, which is the correct context for binary-name API calls.
//
// Commands (registered on the Macro menu; run via Alt→Tools→Macro or the
// shortcut shown):
//
//   SETTINGS.SAVE    (Ctrl+Shift+S) – writes the current in-memory settings
//                                     to the active worksheet.
//   SETTINGS.LOAD    (Ctrl+Shift+L) – reads the stored settings back and
//                                     shows them in an alert.
//   SETTINGS.EXISTS  (Ctrl+Shift+E) – reports whether the binary name is
//                                     present on the active worksheet.
//   SETTINGS.DELETE  (Ctrl+Shift+D) – removes the binary name.

#include <Auto.hpp>
#include <Commands.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>

#include <format>
#include <iostream>

// ============================================================================
// Persisted type
// ============================================================================

/// A plain value type that holds a small set of application settings.
/// Must be trivially copyable so that the default HandleSerializer<T> applies.
struct AppSettings
{
    double  smoothingFactor = 0.5;
    int     iterations      = 100;
    bool    enabled         = true;
};
static_assert(std::is_trivially_copyable_v<AppSettings>,
              "AppSettings must be trivially copyable for xll::Handle<AppSettings>.");

// The name under which the struct is stored in the active worksheet.
static constexpr std::string_view kBinaryName = "AppSettings";

// In-memory copy that commands read from / write to.
static AppSettings g_settings;

// ============================================================================
// Add-in lifecycle
// ============================================================================

xll::AddInManagerInfo dllName([] { return xll::String("Handle Demo"); });

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { std::cerr << "[Handle Demo] xlAutoOpen\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onOpen);

auto onClose =
    xll::OnClose()
    | xll::Before([] { std::cerr << "[Handle Demo] xlAutoClose\n"; });
XLL_REGISTER(onClose);

// ============================================================================
// Command 1: SETTINGS.SAVE
// ============================================================================

auto settingsSaveCmd =
    xll::Command("SETTINGS.SAVE")
    | xll::Procedure("SettingsSave")
    | xll::ShortcutKey("S")
    | xll::Category("Handle Examples")
    | xll::Description(
        "Stores the current in-memory AppSettings as a binary name "
        "on the active worksheet (saved with the workbook).");
XLL_REGISTER(settingsSaveCmd);

XLL_FUNCTION void XLLAPI SettingsSave()
{
    xll::Handle<AppSettings> h(g_settings);

    if (h.store(kBinaryName)) {
        std::cerr << "[SETTINGS.SAVE] Stored: factor=" << g_settings.smoothingFactor
                  << " iterations=" << g_settings.iterations
                  << " enabled=" << g_settings.enabled << '\n';
        xll::alert(xll::String("Settings saved to workbook."));
    }
    else {
        xll::alert(xll::String("Failed to save settings (call from a command context)."));
    }
}

// ============================================================================
// Command 2: SETTINGS.LOAD
// ============================================================================

auto settingsLoadCmd =
    xll::Command("SETTINGS.LOAD")
    | xll::Procedure("SettingsLoad")
    | xll::ShortcutKey("L")
    | xll::Category("Handle Examples")
    | xll::Description(
        "Loads AppSettings from the active worksheet and shows the values.");
XLL_REGISTER(settingsLoadCmd);

XLL_FUNCTION void XLLAPI SettingsLoad()
{
    const auto h = xll::Handle<AppSettings>::load(kBinaryName);

    if (!h) {
        xll::alert(xll::String(
            "No settings found on this worksheet.\n"
            "Run SETTINGS.SAVE first."));
        return;
    }

    // Update the in-memory copy so other commands see the latest values.
    g_settings = h->value();

    const std::string msg = std::format(
        "Loaded AppSettings:\n"
        "  smoothingFactor = {:.4f}\n"
        "  iterations      = {}\n"
        "  enabled         = {}",
        g_settings.smoothingFactor,
        g_settings.iterations,
        g_settings.enabled ? "true" : "false");

    std::cerr << "[SETTINGS.LOAD] " << msg << '\n';
    xll::alert(xll::String(msg));
}

// ============================================================================
// Command 3: SETTINGS.EXISTS
// ============================================================================

auto settingsExistsCmd =
    xll::Command("SETTINGS.EXISTS")
    | xll::Procedure("SettingsExists")
    | xll::ShortcutKey("E")
    | xll::Category("Handle Examples")
    | xll::Description(
        "Reports whether the AppSettings binary name exists on the active worksheet.");
XLL_REGISTER(settingsExistsCmd);

XLL_FUNCTION void XLLAPI SettingsExists()
{
    const bool found = xll::Handle<AppSettings>::exists(kBinaryName);
    xll::alert(xll::String(found
        ? "AppSettings binary name EXISTS on this worksheet."
        : "AppSettings binary name does NOT exist on this worksheet."));
}

// ============================================================================
// Command 4: SETTINGS.DELETE
// ============================================================================

auto settingsDeleteCmd =
    xll::Command("SETTINGS.DELETE")
    | xll::Procedure("SettingsDelete")
    | xll::ShortcutKey("D")
    | xll::Category("Handle Examples")
    | xll::Description(
        "Removes the AppSettings binary name from the active worksheet.");
XLL_REGISTER(settingsDeleteCmd);

XLL_FUNCTION void XLLAPI SettingsDelete()
{
    if (!xll::Handle<AppSettings>::exists(kBinaryName)) {
        xll::alert(xll::String("AppSettings does not exist on this worksheet."));
        return;
    }

    if (xll::Handle<AppSettings>::remove(kBinaryName)) {
        std::cerr << "[SETTINGS.DELETE] Binary name removed.\n";
        xll::alert(xll::String("AppSettings binary name deleted."));
    }
    else {
        xll::alert(xll::String("Failed to delete AppSettings."));
    }
}

// ============================================================================
// Command 5: SETTINGS.MODIFY
// ============================================================================
// Shows how to load, modify, and re-save in one round-trip.

auto settingsModifyCmd =
    xll::Command("SETTINGS.MODIFY")
    | xll::Procedure("SettingsModify")
    | xll::ShortcutKey("M")
    | xll::Category("Handle Examples")
    | xll::Description(
        "Loads the stored settings, doubles the smoothing factor, and re-saves.");
XLL_REGISTER(settingsModifyCmd);

XLL_FUNCTION void XLLAPI SettingsModify()
{
    auto h = xll::Handle<AppSettings>::load(kBinaryName);
    if (!h) {
        xll::alert(xll::String("No settings to modify. Run SETTINGS.SAVE first."));
        return;
    }

    // Mutate via operator->
    (*h)->smoothingFactor *= 2.0;
    (*h)->iterations      += 10;
    (*h)->enabled          = !(*h)->enabled;

    if (h->store(kBinaryName)) {
        g_settings = h->value();
        xll::alert(xll::String(std::format(
            "Settings modified and re-saved:\n"
            "  smoothingFactor = {:.4f}\n"
            "  iterations      = {}\n"
            "  enabled         = {}",
            g_settings.smoothingFactor,
            g_settings.iterations,
            g_settings.enabled ? "true" : "false")));
    }
}


