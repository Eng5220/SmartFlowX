#include <gtest/gtest.h>

// On Windows, if <sys/timerfd.h> is missing, we must mock/skip Timer.
// But we assume building in WSL/Linux based on CMake requirements.
#include "utils/Timer.h"

#include <atomic>
#include <thread>
#include <chrono>

TEST(TimerTest, FiresCallbackRepeatedly) {
    Timer t(50); // 50 ms interval
    
    std::atomic<int> counter{0};
    t.registerCallback([&]() {
        counter++;
    });
    
    t.start();
    // Wait for ~175 ms, we expect roughly 3 fires.
    std::this_thread::sleep_for(std::chrono::milliseconds(175));
    t.stop();
    
    int val = counter.load();
    EXPECT_GE(val, 2);
    EXPECT_LE(val, 5);
}

TEST(TimerTest, StopTerminatesWorker) {
    Timer t(50);
    std::atomic<int> counter{0};
    t.registerCallback([&]() {
        counter++;
    });
    
    t.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    t.stop();
    
    int valBefore = counter.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Counter should not have incremented since stop()
    EXPECT_EQ(counter.load(), valBefore);
}
