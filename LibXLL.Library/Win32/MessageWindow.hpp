#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <deque>
#include <functional>
#include <mutex>
#include <type_traits>
#include <utility>

namespace xll::win32
{
    /// A toolkit-neutral hidden message-only Win32 window that combines:
    ///
    ///  1. **Task-queue dispatch** — `post(std::function<void()>)` enqueues a
    ///     callable; the window procedure drains one task per internal message,
    ///     executing it on the thread that created the window.
    ///
    ///  2. **Raw message callback** — an optional
    ///     `std::function<bool(UINT, WPARAM, LPARAM)>` invoked for every
    ///     `WM_APP+` message that is *not* one of the two reserved internal
    ///     messages.  Return `true` from the callback to mark the message as
    ///     handled; `false` falls through to `DefWindowProc`.
    ///
    ///  3. **Graceful cross-thread shutdown** — `shutdown()` is safe to call
    ///     from any thread; it posts a shutdown message, waits for the window
    ///     to be destroyed on the owner thread, then returns.
    ///
    /// Must be created and destroyed on the same thread (the "owner thread").
    ///
    /// Reserved messages: `WM_APP + 0x7E0` (execute) and `WM_APP + 0x7E1`
    /// (shutdown).  Raw-message callbacks must not use these values.
    class MessageWindow
    {
    public:
        /// Raw-message callback signature.
        /// Return true if the message was handled; false to pass to DefWindowProc.
        using Callback = std::function<bool(UINT, WPARAM, LPARAM)>;

        MessageWindow() = default;

        explicit MessageWindow(Callback callback)
            : m_callback(std::move(callback))
        {
        }

        MessageWindow(const MessageWindow&) = delete;
        MessageWindow& operator=(const MessageWindow&) = delete;
        MessageWindow(MessageWindow&&) = delete;
        MessageWindow& operator=(MessageWindow&&) = delete;

        ~MessageWindow()
        {
            // Best-effort: if still alive, destroy on the current thread.
            // Callers should use shutdown() for cross-thread teardown.
            HWND hwnd = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                hwnd = m_hwnd;
                m_acceptingPosts = false;
                m_tasks.clear();
            }
            if (hwnd != nullptr)
                ::DestroyWindow(hwnd);
        }

        // -----------------------------------------------------------------
        // Lifecycle
        // -----------------------------------------------------------------

        /// Create the hidden message-only window on the current thread.
        /// Returns true on success.  No-op if already created.
        [[nodiscard]] bool create()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_hwnd != nullptr)
                return true;

            if (!ensure_window_class_registered())
                return false;

            m_acceptingPosts = true;
            m_ownerThreadId = ::GetCurrentThreadId();

            HWND hwnd = ::CreateWindowExW(
                0,
                window_class_name(),
                L"",
                0,
                0, 0, 0, 0,
                HWND_MESSAGE,
                nullptr,
                module_handle(),
                this);

            if (hwnd == nullptr) {
                m_acceptingPosts = false;
                m_ownerThreadId = 0;
                return false;
            }

            m_hwnd = hwnd;
            return true;
        }

        /// True when the window exists and is accepting posts.
        [[nodiscard]] bool is_available() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_hwnd != nullptr && m_acceptingPosts;
        }

        /// The HWND that other threads may PostMessage() to.
        /// Returns nullptr if the window has not been created.
        [[nodiscard]] HWND hwnd() const noexcept
        {
            return m_hwnd;
        }

        /// The OS thread ID of the thread that called create(), or 0.
        [[nodiscard]] DWORD owner_thread_id() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_ownerThreadId;
        }

        /// Graceful shutdown.  Safe to call from any thread.
        ///
        /// Stops accepting new posts, destroys the window on the owner
        /// thread, and blocks until destruction is complete.  No-op if the
        /// window was never created or was already shut down.
        void shutdown() noexcept
        {
            HWND hwnd = nullptr;
            DWORD ownerThreadId = 0;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_hwnd == nullptr) {
                    m_acceptingPosts = false;
                    m_ownerThreadId = 0;
                    m_tasks.clear();
                    return;
                }

                m_acceptingPosts = false;
                hwnd = m_hwnd;
                ownerThreadId = m_ownerThreadId;
            }

            if (::GetCurrentThreadId() == ownerThreadId) {
                destroy_on_owner_thread();
                return;
            }

            HANDLE completionEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (completionEvent == nullptr)
                return;

            if (!::PostMessageW(hwnd, message_shutdown(),
                                reinterpret_cast<WPARAM>(completionEvent), 0)) {
                ::CloseHandle(completionEvent);
                return;
            }

            ::WaitForSingleObject(completionEvent, INFINITE);
            ::CloseHandle(completionEvent);
        }

        // -----------------------------------------------------------------
        // Task-queue dispatch
        // -----------------------------------------------------------------

        /// Enqueue a callable to be executed on the owner thread.
        /// Returns false if the window is not available or the post failed.
        [[nodiscard]] bool post(std::function<void()> task)
        {
            if (!task)
                return false;

            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_hwnd == nullptr || !m_acceptingPosts)
                return false;

            m_tasks.emplace_back(std::move(task));
            if (!::PostMessageW(m_hwnd, message_execute(), 0, 0)) {
                m_tasks.pop_back();
                return false;
            }

            return true;
        }

        template <typename F>
            requires std::is_invocable_r_v<void, std::decay_t<F>&>
        [[nodiscard]] bool post(F&& task)
        {
            return post(std::function<void()>(std::forward<F>(task)));
        }

        // -----------------------------------------------------------------
        // Raw-message callback
        // -----------------------------------------------------------------

        /// Replace the raw-message callback.  Thread-safe.
        void set_callback(Callback callback)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callback = std::move(callback);
        }

    private:
        // ----- Reserved internal messages --------------------------------

        static constexpr UINT message_execute() noexcept
        {
            return WM_APP + 0x7E0;
        }

        static constexpr UINT message_shutdown() noexcept
        {
            return WM_APP + 0x7E1;
        }

        // ----- Window class registration ---------------------------------

        static constexpr wchar_t kWindowClassName[] = L"XllMessageWindow";

        [[nodiscard]] static const wchar_t* window_class_name() noexcept
        {
            return kWindowClassName;
        }

        [[nodiscard]] static HINSTANCE module_handle() noexcept
        {
            HMODULE mod = nullptr;
            if (::GetModuleHandleExW(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                    | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(&MessageWindow::WndProc),
                    &mod)) {
                return mod;
            }
            return ::GetModuleHandleW(nullptr);
        }

        [[nodiscard]] static bool ensure_window_class_registered() noexcept
        {
            WNDCLASSW wc = {};
            wc.lpfnWndProc   = &MessageWindow::WndProc;
            wc.hInstance      = module_handle();
            wc.lpszClassName  = window_class_name();

            const ATOM atom = ::RegisterClassW(&wc);
            if (atom != 0)
                return true;

            return ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
        }

        // ----- Window procedure ------------------------------------------

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam)
        {
            if (msg == WM_NCCREATE) {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                auto* self = static_cast<MessageWindow*>(cs->lpCreateParams);
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                                    reinterpret_cast<LONG_PTR>(self));
                return TRUE;
            }

            auto* self = reinterpret_cast<MessageWindow*>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self == nullptr)
                return ::DefWindowProcW(hwnd, msg, wParam, lParam);

            if (msg == message_execute()) {
                self->execute_one();
                return 0;
            }

            if (msg == message_shutdown()) {
                auto completionEvent = reinterpret_cast<HANDLE>(wParam);
                self->destroy_on_owner_thread();
                if (completionEvent != nullptr)
                    ::SetEvent(completionEvent);
                return 0;
            }

            if (msg == WM_NCDESTROY) {
                self->on_window_destroyed(hwnd);
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                return ::DefWindowProcW(hwnd, msg, wParam, lParam);
            }

            // Forward application messages to the raw-message callback.
            if (msg >= WM_APP) {
                std::lock_guard<std::mutex> lock(self->m_mutex);
                if (self->m_callback && self->m_callback(msg, wParam, lParam))
                    return 0;
            }

            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        // ----- Internal helpers ------------------------------------------

        void execute_one() noexcept
        {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_tasks.empty() || !m_acceptingPosts)
                    return;

                task = std::move(m_tasks.front());
                m_tasks.pop_front();
            }

            try {
                task();
            }
            catch (...) {
            }
        }

        void destroy_on_owner_thread() noexcept
        {
            HWND hwnd = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                hwnd = m_hwnd;
                m_acceptingPosts = false;
                m_tasks.clear();
            }

            if (hwnd != nullptr)
                ::DestroyWindow(hwnd);
        }

        void on_window_destroyed(HWND hwnd) noexcept
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_hwnd == hwnd)
                m_hwnd = nullptr;
            m_ownerThreadId = 0;
            m_acceptingPosts = false;
            m_tasks.clear();
        }

        // ----- Data members ----------------------------------------------

        mutable std::mutex                 m_mutex;
        Callback                           m_callback;
        std::deque<std::function<void()>>  m_tasks;
        HWND                               m_hwnd = nullptr;
        DWORD                              m_ownerThreadId = 0;
        bool                               m_acceptingPosts = false;
    };

} // namespace xll::win32

#endif // _WIN32


