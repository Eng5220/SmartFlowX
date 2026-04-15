/**
 * @file pump_test.cpp
 * @brief Standalone smoke test for the SGerste JT80SL DC 3-6V submersible pump.
 *
 * Hardware setup (TIP121 transistor circuit):
 *   Pi GPIO 27 ──[1kΩ]──► TIP121 Base
 *   TIP121 Collector ───────► Pump (-) terminal
 *   Pump (+) terminal ───────► 5V supply
 *   TIP121 Emitter ──────────► GND (shared with Pi GND)
 *   [1N4007 flyback diode across pump, cathode to +]
 *
 * Compile on the Pi:
 *   g++ -o /tmp/pump_test tests/pump_test.cpp -llgpio
 *
 * Run:
 *   sudo /tmp/pump_test
 *
 * Expected:
 *   Pump spins for 5 seconds, then stops.
 *
 * NOTE: Ensure the pump is submerged in water before running.
 *       Running dry will overheat and damage the motor.
 */

#include <lgpio.h>
#include <iostream>
#include <thread>
#include <chrono>

// ─── Configuration ────────────────────────────────────────────────────────────
static constexpr int  GPIO_CHIP    = 4;     // /dev/gpiochip4 — Raspberry Pi 5
static constexpr int  PUMP_PIN     = 27;    // GPIO 27 (BCM) — relay IN pin
static constexpr int  RUN_SECONDS  = 5;     // How long to run the pump

// Set to true  for 1-channel relay module (active-LOW: IN=0 → pump ON)
// Set to false for TIP121/MOSFET         (active-HIGH: pin=1 → pump ON)
static constexpr bool RELAY_MODE   = true;

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== SmartFlowX Pump Smoke Test ===\n";
    std::cout << "Chip: gpiochip" << GPIO_CHIP
              << "  |  Pump pin: GPIO" << PUMP_PIN << "\n\n";

    // Open GPIO chip
    int handle = lgGpiochipOpen(GPIO_CHIP);
    if (handle < 0) {
        std::cerr << "ERROR: Could not open /dev/gpiochip" << GPIO_CHIP
                  << ". Are you running as root (sudo)?\n";
        return 1;
    }

    // Idle level: relay idles HIGH (de-energised), transistor idles LOW
    int idleLevel = RELAY_MODE ? 1 : 0;
    if (lgGpioClaimOutput(handle, 0, PUMP_PIN, idleLevel) != 0) {
        std::cerr << "ERROR: Could not claim GPIO" << PUMP_PIN << " as output.\n";
        lgGpiochipClose(handle);
        return 1;
    }

    std::cout << "Mode: " << (RELAY_MODE ? "RELAY (active-LOW)" : "TRANSISTOR (active-HIGH)") << "\n";
    std::cout << "Pump is OFF — make sure it is submerged before continuing!\n";
    std::cout << "Starting pump in 2 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Turn pump ON
    lgGpioWrite(handle, PUMP_PIN, RELAY_MODE ? 0 : 1);
    std::cout << "✓ Pump ON  (GPIO" << PUMP_PIN << " = " << (RELAY_MODE ? "LOW" : "HIGH") << ")\n";
    std::cout << "Running for " << RUN_SECONDS << " seconds...\n";

    std::this_thread::sleep_for(std::chrono::seconds(RUN_SECONDS));

    // Turn pump OFF
    lgGpioWrite(handle, PUMP_PIN, RELAY_MODE ? 1 : 0);
    std::cout << "✓ Pump OFF (GPIO" << PUMP_PIN << " = " << (RELAY_MODE ? "HIGH" : "LOW") << ")\n\n";

    // Cleanup
    lgGpioFree(handle, PUMP_PIN);
    lgGpiochipClose(handle);

    std::cout << "Test complete.\n";
    std::cout << "  → Did the pump spin? If YES: wiring is correct.\n";
    std::cout << "  → If NO: check TIP121 connections and power supply.\n";
    return 0;
}
