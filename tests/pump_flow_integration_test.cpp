/**
 * @file pump_flow_integration_test.cpp
 * @brief Integration test: DC pump + YF-S401 flow sensor running together.
 *
 * Realtime design (matches RTES course requirements):
 *   - Flow pulses are counted via lgGpioSetAlertsFunc (interrupt callback, NOT polling)
 *   - Pulse counter uses std::atomic<int> for thread-safe access
 *   - Main thread drives the pump and prints volume — no blocking delay in callback
 *
 * Hardware connections:
 *   Pump:        GPIO 27 → TIP121 base [1kΩ] → pump (via 5V rail)
 *   Flow sensor: GPIO 17 → YF-S401 signal wire (3.3V-tolerant, pull-up enabled)
 *                          Red  → 5V
 *                          Black → GND
 *
 * Compile on the Pi:
 *   g++ -o /tmp/pump_flow_test tests/pump_flow_integration_test.cpp -llgpio
 *
 * Run (pump must be submerged, tube leading into cup):
 *   sudo /tmp/pump_flow_test
 *
 * Expected output:
 *   Pump turns ON, pulses increment with each flow tick,
 *   estimated volume (mL) printed every second for 10 s, then pump OFF.
 */

#include <lgpio.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

// ─── Configuration (mirrors PinConfig.h values) ───────────────────────────────
static constexpr int  GPIO_CHIP      = 4;     // /dev/gpiochip4 — Raspberry Pi 5
static constexpr int  PUMP_PIN       = 27;    // GPIO 27 — TIP121 base (pump control)
static constexpr int  FLOW_PIN       = 17;    // GPIO 17 — YF-S401 signal (pull-up)
static constexpr double ML_PER_PULSE = 2.25;  // calibration constant (ml per pulse)
static constexpr int  RUN_SECONDS    = 10;    // How long to run the pump

// ─── Shared state (atomic for safe access from alert callback) ────────────────
static std::atomic<int>  pulseCount(0);
static std::atomic<bool> pumpRunning(false);

// ─── Flow pulse callback (called by lgpio from its internal thread) ───────────
/**
 * @brief lgGpioSetAlertsFunc callback — increments pulse counter on each
 *        falling edge from the YF-S401 Hall-effect sensor.
 *
 * This is interrupt-driven (kernel event → lgpio thread → callback).
 * No polling, no sleep — satisfies RTES event-driven requirement.
 */
static void flowAlertCallback(int        numEvents,
                              lgGpioAlert_p events,
                              void*         userData) {
    for (int i = 0; i < numEvents; ++i) {
        // Only count falling edges (signal goes LOW on each blade pass)
        if (events[i].report.level == 0) {
            pulseCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== SmartFlowX: Pump + Flow Sensor Integration Test ===\n";
    std::cout << "Pump pin:  GPIO" << PUMP_PIN
              << "  |  Flow pin: GPIO" << FLOW_PIN << "\n";
    std::cout << "Calibration: " << ML_PER_PULSE << " mL/pulse\n";
    std::cout << "Run duration: " << RUN_SECONDS << " seconds\n\n";

    // --- Open GPIO chip ---
    int handle = lgGpiochipOpen(GPIO_CHIP);
    if (handle < 0) {
        std::cerr << "ERROR: Could not open /dev/gpiochip" << GPIO_CHIP
                  << ". Run with sudo.\n";
        return 1;
    }

    // --- Claim pump pin as output (OFF) ---
    if (lgGpioClaimOutput(handle, 0, PUMP_PIN, 0) != 0) {
        std::cerr << "ERROR: Could not claim pump pin GPIO" << PUMP_PIN << ".\n";
        lgGpiochipClose(handle);
        return 1;
    }

    // --- Claim flow sensor pin as input with pull-up ---
    // LG_SET_PULL_UP enables the Pi's internal pull-up; YF-S401 pulls LOW on pulse.
    if (lgGpioClaimInput(handle, LG_SET_PULL_UP, FLOW_PIN) != 0) {
        std::cerr << "ERROR: Could not claim flow pin GPIO" << FLOW_PIN << ".\n";
        lgGpioFree(handle, PUMP_PIN);
        lgGpiochipClose(handle);
        return 1;
    }

    // --- Register falling-edge alert callback on flow pin ---
    // lgGpioSetAlertsFunc attaches a kernel-level alert: every edge change on
    // FLOW_PIN triggers flowAlertCallback on lgpio's internal thread.
    lgGpioSetAlertsFunc(handle, FLOW_PIN, flowAlertCallback, nullptr);

    std::cout << "Get ready — pump starting in 2 seconds...\n";
    std::cout << "(Make sure pump is submerged in water source!)\n\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // --- Turn pump ON ---
    lgGpioWrite(handle, PUMP_PIN, 1);
    pumpRunning = true;
    std::cout << "✓ Pump ON\n\n";

    // --- Monitor for RUN_SECONDS, printing volume each second ---
    std::cout << std::fixed << std::setprecision(1);
    auto startTime = std::chrono::steady_clock::now();

    for (int elapsed = 1; elapsed <= RUN_SECONDS; ++elapsed) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        int    pulses    = pulseCount.load(std::memory_order_relaxed);
        double volumeML  = static_cast<double>(pulses) * ML_PER_PULSE;

        std::cout << "[" << std::setw(2) << elapsed << "s]  "
                  << "Pulses: " << std::setw(4) << pulses
                  << "  |  Volume: " << std::setw(7) << volumeML << " mL\n";
    }

    // --- Turn pump OFF ---
    lgGpioWrite(handle, PUMP_PIN, 0);
    pumpRunning = false;

    int    totalPulses = pulseCount.load();
    double totalVolumeML = static_cast<double>(totalPulses) * ML_PER_PULSE;

    std::cout << "\n✓ Pump OFF\n";
    std::cout << "─────────────────────────────────────────\n";
    std::cout << "Total pulses:  " << totalPulses   << "\n";
    std::cout << "Total volume:  " << totalVolumeML << " mL\n";
    std::cout << "─────────────────────────────────────────\n\n";

    if (totalPulses == 0) {
        std::cout << "WARNING: Zero pulses detected.\n";
        std::cout << "  → Check flow sensor wiring (5V, GND, signal on GPIO" 
                  << FLOW_PIN << ").\n";
        std::cout << "  → Check that water is actually flowing through the sensor.\n";
    } else {
        std::cout << "Flow sensor working correctly. "
                  << "Ready to run the full state machine.\n";
    }

    // --- Cleanup ---
    lgGpioSetAlertsFunc(handle, FLOW_PIN, nullptr, nullptr);  // detach callback
    lgGpioFree(handle, FLOW_PIN);
    lgGpioFree(handle, PUMP_PIN);
    lgGpiochipClose(handle);

    return 0;
}
