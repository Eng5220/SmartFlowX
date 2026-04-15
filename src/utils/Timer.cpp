#include "utils/Timer.h"

#include <stdexcept>
#include <unistd.h>
#include <sys/timerfd.h>
#include <cstdint>

void Timer::start() {
    if (running_) return;

    // Create a monotonic periodic timer file descriptor
    timerFd_ = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerFd_ < 0) throw std::runtime_error("Timer: timerfd_create failed");

    itimerspec its{};
    its.it_value.tv_sec     = intervalMs_ / 1000;
    its.it_value.tv_nsec    = (intervalMs_ % 1000) * 1000000LL;
    its.it_interval.tv_sec  = intervalMs_ / 1000;
    its.it_interval.tv_nsec = (intervalMs_ % 1000) * 1000000LL;

    if (timerfd_settime(timerFd_, 0, &its, nullptr) < 0) {
        close(timerFd_);
        timerFd_ = -1;
        throw std::runtime_error("Timer: timerfd_settime failed");
    }

    running_ = true;
    workerThread_ = std::thread(&Timer::worker, this);
}

void Timer::stop() {
    running_ = false;
    // Close fd so that the blocking read() in worker() unblocks immediately
    if (timerFd_ >= 0) {
        close(timerFd_);
        timerFd_ = -1;
    }
    if (workerThread_.joinable()) workerThread_.join();
}

void Timer::worker() {
    while (running_) {
        uint64_t expirations = 0;
        ssize_t s = read(timerFd_, &expirations, sizeof(expirations));
        if (!running_) break;
        if (s == sizeof(expirations) && callback_) {
            callback_();
        }
    }
}
