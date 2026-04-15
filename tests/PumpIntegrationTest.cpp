#include "PinConfig.h"
#include "hardware/PumpController.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <pthread.h>
#include <signal.h>

// ─── Graceful shutdown on Ctrl+C ─────────────────────────────────────────────
int main() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    auto waitForStop = [&sigset](std::chrono::milliseconds timeout) {
        timespec ts{};
        ts.tv_sec = timeout.count() / 1000;
        ts.tv_nsec = static_cast<long>((timeout.count() % 1000) * 1000000);
        int sig = sigtimedwait(&sigset, nullptr, &ts);
        if (sig == SIGINT || sig == SIGTERM) {
            std::cout << "\n[Signal " << sig << "] Stopping...\n";
            return true;
        }
        return false;
    };

    std::cout << "=== Pump Integration Test ===\n";
    std::cout << "Using libgpiod on Chip " << GPIO_CHIP_NO << ", Pin " << PUMP_PIN << "\n";

    // Initialise RTES-compliant pump controller (uses libgpiod)
    PumpController pump(GPIO_CHIP_NO, PUMP_PIN);

    if (!pump.init()) {
        std::cerr << "Failed to initialise pump hardware.\n";
        return 1;
    }

    std::cout << "Starting 3-cycle test...\n";

    bool stopRequested = false;
    for (int i = 0; i < 3 && !stopRequested; ++i) {
        std::cout << "Cycle " << i + 1 << ": PUMP ON\n";
        pump.turnOn();
        // Since this is just a sequential integration test script, sleep_for is acceptable here
        // (the main application uses event-driven state machines, not sleeps)
        if (waitForStop(std::chrono::seconds(2))) {
            stopRequested = true;
            break;
        }

        std::cout << "Cycle " << i + 1 << ": PUMP OFF\n";
        pump.turnOff();
        if (waitForStop(std::chrono::seconds(2))) {
            stopRequested = true;
            break;
        }
    }

    // Ensure it's off before exit
    pump.shutdown();
    std::cout << "Test complete. Pump OFF.\n";

    return 0;
}
