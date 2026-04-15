#include <iostream>
#include <lgpio.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

// --- Configuration ---
const int GPIO_CHIP = 4;   // Pi 5 Header
const int FLOW_PIN  = 27;  // Flow Sensor Signal (Physical Pin 13)

std::atomic<int> pulseCount(0);
std::atomic<bool> running(true);

// Counts ANY edge from the flow sensor (rising or falling)
void flowCallback(int count, lgGpioAlert_p events, void *userdata) {
    for (int i = 0; i < count; i++) {
        pulseCount++;  // Count every edge change — remove filter to catch all signals
        // Debug: uncomment the line below to see raw signal levels
        // std::cout << "\n[DEBUG] GPIO edge detected, level=" << events[i].report.level << "\n";
    }
}

void signalHandler(int signum) {
    running = false;
}

int main() {
    std::signal(SIGINT, signalHandler);

    int handle = lgGpiochipOpen(GPIO_CHIP);
    if (handle < 0) {
        std::cerr << "Failed to open GPIO chip\n";
        return 1;
    }

    // Setup flow sensor pin with internal pull-up
    lgGpioClaimInput(handle, LG_SET_PULL_UP, FLOW_PIN);
    lgGpioSetAlertsFunc(handle, FLOW_PIN, flowCallback, nullptr);

    std::cout << "--- Flow Sensor Test ---\n";
    std::cout << "Run water through the sensor now.\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    while (running) {
        std::cout << "\rPulses: " << pulseCount.load() << "   " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n\nFinal pulse count: " << pulseCount.load() << "\n";
    lgGpiochipClose(handle);
    return 0;
}
