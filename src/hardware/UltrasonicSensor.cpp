#include <atomic>       // Thread-safe flag for Ctrl+C handling
#include <chrono>       // Sleep durations
#include <csignal>      // Signal handling (SIGINT)
#include <iostream>     // Console output
#include <thread>       // sleep_for

#include "UltrasonicSensor.h"

// Global flag — set to false when user presses Ctrl+C
static std::atomic<bool> keepRunning{true};

// Signal handler for graceful shutdown
static void onSigInt(int) {
    keepRunning = false;
}

int main() {
    // Register Ctrl+C handler
    std::signal(SIGINT, onSigInt);

    try {
        // Create sensor on chip 0, TRIG=GPIO23, ECHO=GPIO24, measuring every 200ms
        // Change chip number if needed:
        //   Pi 5 often uses gpiochip4, older Pi often gpiochip0
        UltrasonicSensor sensor(0, 23, 24, 200);

        // Print distance whenever a valid measurement is taken
        sensor.registerDistanceCallback([](float distanceCm) {
            std::cout << "Measured Distance = " << distanceCm << " cm\n";
        });

        // Print error messages (timeouts, bad readings)
        sensor.registerErrorCallback([](const std::string& msg) {
            std::cout << "ERROR: " << msg << "\n";
        });

        // Start measuring (launches background thread)
        sensor.start();

        // Keep main thread alive until Ctrl+C
        while (keepRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Clean shutdown
        sensor.stop();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
