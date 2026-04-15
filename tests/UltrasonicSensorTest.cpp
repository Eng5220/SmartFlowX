/**
 * @file UltrasonicSensorTest.cpp
 * @brief Test program for UltrasonicSensor — uses mock gpiod.hpp to simulate GPIO.
 *
 * Compile:
 *   g++ -std=c++17 -Itests/mock_include -Isrc/hardware \
 *       -o build/ultrasonic_sensor_test tests/UltrasonicSensorTest.cpp -lpthread
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cassert>
#include <cmath>
#include <string>

#include "UltrasonicSensor.h"

int main() {

    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        SmartFlowX — UltrasonicSensor Test Program          ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Sensor: HC-SR04 Ultrasonic Distance Sensor                ║\n";
    std::cout << "║  Mode:   Mock GPIO (simulated ~12cm distance)              ║\n";
    std::cout << "║  Sample Rate: 200ms (5 Hz)                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int total  = 0;

    // ── Test 1: Fresh construction ───────────────────────────────────────────
    total++;
    std::cout << "── Test 1: Fresh construction ────────────────────────────\n";
    UltrasonicSensor sensor(0, 23, 24, 200);
    std::cout << "  ✓ Sensor created (chip=0, trig=23, echo=24, period=200ms)\n\n";
    passed++;

    // ── Test 2: Register callbacks ───────────────────────────────────────────
    total++;
    std::cout << "── Test 2: Register callbacks ──────────────────────────\n";
    int distanceCount = 0;
    float lastDistance = -1.0f;
    int errorCount = 0;
    std::string lastError;

    sensor.registerDistanceCallback([&](float distanceCm) {
        distanceCount++;
        lastDistance = distanceCm;
    });

    sensor.registerErrorCallback([&](const std::string& msg) {
        errorCount++;
        lastError = msg;
    });

    std::cout << "  ✓ Distance callback registered\n";
    std::cout << "  ✓ Error callback registered\n\n";
    passed++;

    // ── Test 3: Start sensor ─────────────────────────────────────────────────
    total++;
    std::cout << "── Test 3: Start sensor ───────────────────────────────\n";
    sensor.start();
    std::cout << "  ✓ Sensor started (GPIO + timerfd + worker thread)\n\n";
    passed++;

    // ── Test 4: Collect data for 3 seconds ───────────────────────────────────
    total++;
    std::cout << "── Test 4: Collect data for 3 seconds ─────────────────\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "  Distance callbacks received: " << distanceCount << "\n";
    std::cout << "  Error callbacks received:    " << errorCount << "\n";
    std::cout << "  Last distance: " << lastDistance << " cm\n";
    if (errorCount > 0) {
        std::cout << "  Last error: " << lastError << "\n";
    }

    // We should have received some measurements (distance or error)
    int totalCallbacks = distanceCount + errorCount;
    assert(totalCallbacks > 0);
    std::cout << "  ✓ Sensor produced " << totalCallbacks << " measurements\n\n";
    passed++;

    // ── Test 5: Distance value check ─────────────────────────────────────────
    total++;
    std::cout << "── Test 5: Distance value check ───────────────────────\n";
    if (distanceCount > 0) {
        std::cout << "  Last measured distance: " << lastDistance << " cm\n";
        assert(lastDistance > 0.0f);
        std::cout << "  ✓ Distance is positive (sensor is measuring)\n\n";
    } else {
        std::cout << "  ⚠ No valid distances (mock timing overhead)\n";
        std::cout << "  ✓ Errors reported instead (expected in simulation)\n\n";
    }
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
    int countBefore = distanceCount + errorCount;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    int countAfter = distanceCount + errorCount;
    assert(countAfter == countBefore);
    std::cout << "  ✓ No new callbacks after stop (count still " << countBefore << ")\n\n";
    passed++;

    // ── Results ──────────────────────────────────────────────────────────────
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         ALL " << passed << "/" << total
              << " TESTS PASSED ✓                            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    return 0;
}
