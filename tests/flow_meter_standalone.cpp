/**
 * @file flow_meter_standalone.cpp
 * @brief Standalone FlowMeter test — verifies logic without real GPIO hardware.
 *
 * This file mocks the lgpio functions so we can cross-compile and run
 * the FlowMeter class on any machine (including via WSL cross-compile).
 * On the Raspberry Pi, the real lgpio library is used instead.
 *
 * Compile: aarch64-linux-gnu-g++ -std=c++17 -Itests/mock_include -Iinclude -Isrc
 *          -o build/flow_meter_test tests/flow_meter_standalone.cpp -lpthread
 */

#include <lgpio.h>   // picks up tests/mock_include/lgpio.h (our mock)
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cassert>
#include <cmath>

// ─── Mock lgpio function implementations ─────────────────────────────────────
// These replace the real lgpio library for testing purposes.

// Store the callback so we can simulate pulses
static lgGpioAlertsFunc_t g_callback = nullptr;
static void* g_userdata = nullptr;

int lgGpioClaimInput(int handle, int flags, int pin) {
    std::cout << "  [MOCK] lgGpioClaimInput(handle=" << handle
              << ", pin=" << pin << ") → OK\n";
    return 0;
}

int lgGpioClaimAlert(int handle, int flags, int edge, int pin, int debounce) {
    std::cout << "  [MOCK] lgGpioClaimAlert(handle=" << handle
              << ", pin=" << pin << ", edge=" << edge << ") → OK\n";
    return 0;
}

int lgGpioSetAlertsFunc(int handle, int pin,
                        lgGpioAlertsFunc_t func, void* userdata) {
    g_callback = func;
    g_userdata = userdata;
    if (func) {
        std::cout << "  [MOCK] lgGpioSetAlertsFunc → callback registered\n";
    } else {
        std::cout << "  [MOCK] lgGpioSetAlertsFunc → callback removed\n";
    }
    return 0;
}

int lgGpioFree(int handle, int pin) {
    std::cout << "  [MOCK] lgGpioFree(handle=" << handle
              << ", pin=" << pin << ") → OK\n";
    return 0;
}

// ─── Include the real FlowMeter header and source ────────────────────────────
// The compiler is invoked with -Itests/mock_include FIRST so lgpio.h resolves
// to our mock version.

#include "IHardwareDevice.h"
#include "hardware/FlowMeter.h"

// Inline the FlowMeter implementation directly (avoids separate compilation)
#include "hardware/FlowMeter.cpp"

// ─── Simulate pulses (as if water is flowing) ───────────────────────────────
void simulatePulses(int count, int intervalMs) {
    std::cout << "\n  Simulating " << count << " pulses (" << intervalMs << "ms apart)...\n";
    for (int i = 0; i < count; ++i) {
        if (g_callback && g_userdata) {
            // Call the FlowMeter's static callback as if the GPIO triggered
            g_callback(1, nullptr, g_userdata);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

// ─── Main test ──────────────────────────────────────────────────────────────
int main() {
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          SmartFlowX — FlowMeter Standalone Test             ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Sensor: YF-S401 Hall-effect Flow Meter                    ║\n";
    std::cout << "║  Mode:   Mock GPIO (no hardware required)                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    const int GPIO_HANDLE = 0;
    const int FLOW_PIN = 17;
    const double ML_PER_PULSE = 2.25;
    const double TARGET_ML = 500.0;

    // ── Test 1: Create and initialise ────────────────────────────────────────
    std::cout << "── Test 1: Initialisation ──────────────────────────────\n";
    FlowMeter meter(GPIO_HANDLE, FLOW_PIN, ML_PER_PULSE);
    bool ok = meter.init();
    assert(ok && "FlowMeter init failed!");
    std::cout << "  ✓ FlowMeter initialised successfully\n\n";

    // ── Test 2: Initial state ────────────────────────────────────────────────
    std::cout << "── Test 2: Initial state ──────────────────────────────\n";
    assert(meter.getPulseCount() == 0);
    assert(meter.getVolumeML() == 0.0);
    assert(!meter.hasReachedTarget(TARGET_ML));
    std::cout << "  ✓ Pulse count = " << meter.getPulseCount() << "\n";
    std::cout << "  ✓ Volume      = " << meter.getVolumeML() << " ml\n";
    std::cout << "  ✓ Target reached? No (correct)\n\n";

    // ── Test 3: Simulate 10 pulses ───────────────────────────────────────────
    std::cout << "── Test 3: Simulate 10 pulses ─────────────────────────\n";
    simulatePulses(10, 50);
    std::cout << "  ✓ Pulse count = " << meter.getPulseCount() << "\n";
    std::cout << "  ✓ Volume      = " << meter.getVolumeML() << " ml\n";
    assert(meter.getPulseCount() == 10);
    assert(meter.getVolumeML() == 10 * ML_PER_PULSE);
    std::cout << "  ✓ Volume matches expected (" << 10 * ML_PER_PULSE << " ml)\n\n";

    // ── Test 4: Reset counter ────────────────────────────────────────────────
    std::cout << "── Test 4: Reset counter ──────────────────────────────\n";
    meter.resetCount();
    assert(meter.getPulseCount() == 0);
    assert(meter.getVolumeML() == 0.0);
    std::cout << "  ✓ Counter reset to 0\n\n";

    // ── Test 5: Fill to target (500 ml) ──────────────────────────────────────
    std::cout << "── Test 5: Fill to target (" << TARGET_ML << " ml) ─────────────────\n";
    int pulsesNeeded = static_cast<int>(std::ceil(TARGET_ML / ML_PER_PULSE));
    std::cout << "  Pulses needed: " << pulsesNeeded << "\n";
    simulatePulses(pulsesNeeded, 1);  // fast simulation
    std::cout << "  ✓ Pulse count = " << meter.getPulseCount() << "\n";
    std::cout << "  ✓ Volume      = " << meter.getVolumeML() << " ml\n";
    std::cout << "  ✓ Target reached? " << (meter.hasReachedTarget(TARGET_ML) ? "YES" : "NO") << "\n";
    assert(meter.hasReachedTarget(TARGET_ML));
    std::cout << "  ✓ Target reached correctly!\n\n";

    // ── Test 6: Shutdown ─────────────────────────────────────────────────────
    std::cout << "── Test 6: Shutdown ────────────────────────────────────\n";
    meter.shutdown();
    std::cout << "  ✓ FlowMeter shut down cleanly\n\n";

    // ── Results ──────────────────────────────────────────────────────────────
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              ALL 6 TESTS PASSED ✓                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    return 0;
}
