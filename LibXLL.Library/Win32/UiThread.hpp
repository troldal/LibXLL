#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace xll::win32
{
    /// Manages a dedicated UI thread with a synchronized ready-wait protocol.
    ///
    /// Any GUI toolkit (wx, Qt, Dear ImGui, …) can be bootstrapped inside the
    /// thread body.  The body signals success or failure; start() blocks until
    /// that signal arrives and returns the result.
    ///
    /// Typical usage from an XLL command:
    ///
    ///     static xll::win32::UiThread s_uiThread;
    ///
    ///     s_uiThread.start([excelHwnd](xll::win32::UiThread& ut) {
    ///         // … init toolkit, create window …
    ///         ut.signal_ready();          // or signal_failed()
    ///         // … run event loop …
    ///         // … cleanup …
    ///     });
    ///
    /// The thread body MUST call exactly one of signal_ready() or
    /// signal_failed() before it blocks in an event loop or returns.
    class UiThread
    {
    public:
        /// Thread body signature.  Receives a reference to this UiThread
        /// so it can call signal_ready() / signal_failed().
        using ThreadBody = std::function<void(UiThread&)>;

        UiThread() = default;

        UiThread(const UiThread&) = delete;
        UiThread& operator=(const UiThread&) = delete;
        UiThread(UiThread&&) = delete;
        UiThread& operator=(UiThread&&) = delete;

        ~UiThread()
        {
            join();
        }

        /// Spawn the UI thread and block until the body signals.
        ///
        /// Returns true if the thread was already running or if the body
        /// called signal_ready().  Returns false if the body called
        /// signal_failed() (or forgot to signal at all).
        ///
        /// If a previous thread has exited, it is joined before spawning
        /// a new one.
        [[nodiscard]] bool start(ThreadBody body)
        {
            if (m_running.load())
                return true;

            // Clean up a previous thread that already exited.
            if (m_thread.joinable())
                m_thread.join();

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_signaled = false;
                m_succeeded = false;
            }

            m_thread = std::thread([this, body = std::move(body)]() {
                m_threadId.store(::GetCurrentThreadId());
                m_running.store(true);

                body(*this);

                // Safety net: if the body returned without signaling,
                // unblock start() with a failure result.
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (!m_signaled) {
                        m_signaled = true;
                        m_succeeded = false;
                    }
                }
                m_cv.notify_all();

                m_threadId.store(0);
                m_running.store(false);
            });

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_signaled; });
            }

            return m_succeeded;
        }

        /// Signal successful initialization.  Called from the thread body
        /// after the toolkit and window have been created.  Unblocks start()
        /// which will return true.
        void signal_ready()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_signaled = true;
                m_succeeded = true;
            }
            m_cv.notify_all();
        }

        /// Signal failed initialization.  Called from the thread body on
        /// error paths.  Unblocks start() which will return false.
        void signal_failed()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_signaled = true;
                m_succeeded = false;
            }
            m_cv.notify_all();
        }

        /// Block until the thread exits.  Safe to call multiple times.
        void join()
        {
            if (m_thread.joinable())
                m_thread.join();
        }

        /// True from the moment the thread body starts executing until it
        /// returns (i.e. the event loop has exited and cleanup is done).
        [[nodiscard]] bool is_running() const noexcept
        {
            return m_running.load();
        }

        /// The OS thread ID while the thread is running, or 0.
        [[nodiscard]] DWORD thread_id() const noexcept
        {
            return m_threadId.load();
        }

    private:
        std::thread             m_thread;
        mutable std::mutex      m_mutex;
        std::condition_variable m_cv;
        bool                    m_signaled = false;
        bool                    m_succeeded = false;
        std::atomic<bool>       m_running{ false };
        std::atomic<DWORD>      m_threadId{ 0 };
    };

} // namespace xll::win32

#endif // _WIN32

