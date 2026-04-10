// ---------------------------------------------------------------------------
// ImGuiWindowContent.hpp — type erasure interface for ImGui window content.
//
// PUBLIC API
// ----------
// Any type T that satisfies WindowContent can be hosted in an
// ImGuiModalWindow or ImGuiModelessWindow without inheriting from anything:
//
//   FrameAction T::renderContent()
//       Build the Dear ImGui UI for one frame and signal the window:
//         Continue      — keep the window open and rendering.
//         RequestClose  — notify the window to close (modal) or hide
//                         (modeless) after this frame completes.
//
// Three optional hooks are detected at compile time and called when present:
//
//   void T::onDpiChanged(float scale)   — monitor DPI changed.
//   void T::onActivateApp(bool active)  — app activated / deactivated.
//   void T::onClose()                   — window is about to close or hide.
//
// INTERNAL
// --------
// ContentConcept and ContentModel<T> are the virtual bridge used internally
// by ImGuiWindowBase.  They are not part of the public API.
// ---------------------------------------------------------------------------

#pragma once

// ---------------------------------------------------------------------------
// FrameAction — returned from renderContent() to signal the window.
// ---------------------------------------------------------------------------
enum class FrameAction { Continue, RequestClose };

// ---------------------------------------------------------------------------
// WindowContent concept — the only requirement imposed on user types.
// ---------------------------------------------------------------------------
template<typename T>
concept WindowContent = requires(T t)
{
    { t.renderContent() } -> std::same_as<FrameAction>;
};

// ---------------------------------------------------------------------------
// ContentConcept — internal virtual interface bridging the erased type to
// the window machinery.  Not exposed in the public API.
// ---------------------------------------------------------------------------
struct ContentConcept
{
    virtual ~ContentConcept()               = default;
    virtual FrameAction renderContent()     = 0;
    virtual void        onDpiChanged(float) = 0;
    virtual void        onActivateApp(bool) = 0;
    virtual void        onClose()           = 0;
};

// ---------------------------------------------------------------------------
// ContentModel<T> — concrete bridge; one instantiation per content type.
// Optional hooks are detected with requires-expressions and called only
// when present, so content types need not implement every hook.
// ---------------------------------------------------------------------------
template<WindowContent T>
struct ContentModel final : ContentConcept
{
    T m_value;
    explicit ContentModel(T v) : m_value(std::move(v)) {}

    FrameAction renderContent()     override { return m_value.renderContent(); }

    void onDpiChanged(float scale)  override
    {
        if constexpr (requires { m_value.onDpiChanged(scale); })
            m_value.onDpiChanged(scale);
    }
    void onActivateApp(bool active) override
    {
        if constexpr (requires { m_value.onActivateApp(active); })
            m_value.onActivateApp(active);
    }
    void onClose()                  override
    {
        if constexpr (requires { m_value.onClose(); })
            m_value.onClose();
    }
};
