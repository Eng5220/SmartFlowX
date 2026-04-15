#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <cstdint>

/**
 * @brief RTES-compliant periodic timer using Linux timerfd.
 *
 * Fires a std::function callback at a fixed interval without
 * using sleep commands. The blocking read() on the timerfd
 * puts the worker thread to sleep between firings, consuming
 * zero CPU while idle.
 *
 * Usage:
 *   Timer t(100);                         // 100 ms interval
 *   t.registerCallback([]{ doWork(); });
 *   t.start();
 *   // ...
 *   t.stop();
 */
class Timer {
public:
    using TimerCallback = std::function<void()>;

    /**
     * @brief Construct an executing context locking the duration intervals.
     * @param intervalMs The repeating cycle time bounded in milliseconds integer.
     */
    explicit Timer(int intervalMs) : intervalMs_(intervalMs) {}
    ~Timer() { stop(); }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    void registerCallback(TimerCallback cb) { callback_ = std::move(cb); }

    /** @brief Create timerfd and launch worker thread. */
    void start();

    /** @brief Signal thread to stop and join it. */
    void stop();

private:
    void worker();

    int intervalMs_;
    int timerFd_{-1};
    std::atomic<bool> running_{false};
    std::thread workerThread_;
    TimerCallback callback_;
};
