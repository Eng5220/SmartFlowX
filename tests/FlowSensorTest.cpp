/**
 * @file FlowSensorTest.cpp
 * @brief Clean test program for FlowSensor — starts from zero, no prior data.
 *
 * Uses the mock gpiod.hpp to simulate GPIO pulses on Windows/WSL.
 * Compile:
 *   g++ -std=c++17 -Itests/mock_include -Isrc/hardware \
 *       -o build/flow_sensor_test tests/FlowSensorTest.cpp -lpthread
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <cmath>

#include "FlowMeter.h"  // FlowSensor class

int main() {

    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           SmartFlowX — FlowSensor Test Program             ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Sensor: YF-S401 Hall-effect Flow Meter                    ║\n";
    std::cout << "║  Mode:   Mock GPIO (simulated ~10 pulses/sec)              ║\n";
    std::cout << "║  Calibration: 98 pulses per litre per minute               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int total  = 0;

    // ── Test 1: Fresh construction ───────────────────────────────────────────
    total++;
    std::cout << "── Test 1: Fresh construction ────────────────────────────\n";
    FlowSensor sensor(4, 23, 98.0f);
    assert(sensor.getPulseCount() == 0);
    std::cout << "  ✓ Pulse count starts at 0\n\n";
    passed++;

    // ── Test 2: Register callbacks ───────────────────────────────────────────
    total++;
    std::cout << "── Test 2: Register callbacks ──────────────────────────\n";
    int pulseCallbackCount = 0;
    float lastFlowRate = -1.0f;

    sensor.registerPulseCallback([&pulseCallbackCount]() {
        pulseCallbackCount++;
    });

    sensor.registerFlowCallback([&lastFlowRate](float rate) {
        lastFlowRate = rate;
        std::cout << "  → Flow Rate: " << rate << " L/min\n";
    });
    std::cout << "  ✓ Pulse callback registered\n";
    std::cout << "  ✓ Flow callback registered\n\n";
    passed++;

    // ── Test 3: Start sensor ─────────────────────────────────────────────────
    total++;
    std::cout << "── Test 3: Start sensor ───────────────────────────────\n";
    sensor.start();
    std::cout << "  ✓ Sensor started (edge + rate threads running)\n\n";
    passed++;

    // ── Test 4: Collect data for 3 seconds ───────────────────────────────────
    total++;
    std::cout << "── Test 4: Collect data for 3 seconds ─────────────────\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "  Pulse callbacks received: " << pulseCallbackCount << "\n";
    std::cout << "  Last flow rate: " << lastFlowRate << " L/min\n";

    assert(pulseCallbackCount > 0);
    assert(lastFlowRate > 0.0f);
    std::cout << "  ✓ Pulses detected and flow rate calculated\n\n";
    passed++;

    // ── Test 5: Flow rate sanity check ───────────────────────────────────────
    total++;
    std::cout << "── Test 5: Flow rate sanity check ─────────────────────\n";
    // Mock generates ~10 pulses/sec → frequency ≈ 10
    // Flow rate = frequency / calibration = 10 / 98 ≈ 0.102 L/min
    float expected = 10.0f / 98.0f;  // ~0.102
    float tolerance = 0.03f;
    std::cout << "  Expected ≈ " << expected << " L/min (±" << tolerance << ")\n";
    std::cout << "  Actual   = " << lastFlowRate << " L/min\n";

    assert(std::fabs(lastFlowRate - expected) < tolerance);
    std::cout << "  ✓ Flow rate within expected range\n\n";
    passed++;

    // ── Test 6: Stop sensor ──────────────────────────────────────────────────
    total++;
    std::cout << "── Test 6: Stop sensor ────────────────────────────────\n";
    sensor.stop();
    std::cout << "  ✓ Sensor stopped cleanly\n\n";
    passed++;

    // ── Test 7: No activity after stop ───────────────────────────────────────
    total++;
    std::cout << "── Test 7: No activity after stop ─────────────────────\n";
    int countBefore = pulseCallbackCount;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    assert(pulseCallbackCount == countBefore);
    std::cout << "  ✓ No new pulses after stop (count still " << countBefore << ")\n\n";
    passed++;

    // ── Results ──────────────────────────────────────────────────────────────
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         ALL " << passed << "/" << total
              << " TESTS PASSED ✓                            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    return 0;
}
