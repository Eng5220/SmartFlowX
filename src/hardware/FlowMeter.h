#pragma once

#include "IHardwareDevice.h"
#include "IFlowMeter.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <thread>

#include <gpiod.hpp>

/**
 * @brief YF-S401 hall-effect flow sensor driver.
 *
 * Uses libgpiod v2 to detect falling edges (pulses) on the sensor's
 * signal pin in a dedicated thread (blocking I/O, no polling).
 *
 * Each pulse corresponds to a fixed volume of water (mlPerPulse).
 * The accumulated count is exposed via a poll-safe atomic API so that
 * FillingController can query volume without locking.
 *
 * Software debouncing rejects pulses spaced closer than DEBOUNCE_MS apart.
 * At the YF-S401's maximum flow rate (6 L/min) real pulses are ~10 ms apart,
 * so a 5 ms debounce window filters motor-induced EMI without losing real data.
 *
 * Inherits IHardwareDevice for SOLID Liskov substitutability.
 */
class FlowMeter : public IHardwareDevice, public IFlowMeter {
public:
    /**
     * @param chipNo      GPIO chip number (0 for Pi 1-4, 4 for Pi 5).
     * @param pinNo       BCM pin connected to sensor signal output.
     * @param mlPerPulse  Volume per pulse in millilitres (default 1.0 ml).
     */
    FlowMeter(unsigned int chipNo, unsigned int pinNo, float mlPerPulse = 1.0f)
        : chipNo_(chipNo), pinNo_(pinNo), mlPerPulse_(mlPerPulse) {}

    FlowMeter(const FlowMeter&) = delete;
    FlowMeter& operator=(const FlowMeter&) = delete;
    FlowMeter(FlowMeter&&) = delete;
    FlowMeter& operator=(FlowMeter&&) = delete;

    ~FlowMeter() override { shutdown(); }

    // ── IHardwareDevice interface ────────────────────────────────────────────

    /** @brief Set up GPIO and launch pulse-counting thread. */
    bool init() override;

    /** @brief Stop thread and release GPIO resources. */
    void shutdown() override;

    // ── Flow data API (safe to call from any thread) ─────────────────────────

    /** @brief Reset the pulse counter to zero (call before a new fill). */
    void resetCount() override;

    /** @brief Current pulse count since last reset. */
    int getPulseCount() const override;

    /** @brief Accumulated volume in millilitres since last reset. */
    double getVolumeML() const override;

#ifdef AQUAFLOW_TESTING
    /** @brief Test seam for unit tests to inject a synthetic pulse count. */
    void injectPulseCountForTest(int pulseCount);
#endif

private:
    void setupGpio();

    /** @brief Background thread: blocks on GPIO edge events, increments counter. */
    void edgeWorker();

    unsigned int chipNo_;
    unsigned int pinNo_;
    float mlPerPulse_;

    // Minimum time between valid pulses (ms).
    // YF-S401 max flow = 6 L/min → ~98 pulses/sec → ~10 ms between pulses.
    // 5 ms rejects motor-EMI noise bursts while passing all real flow pulses.
    static constexpr int DEBOUNCE_MS = 5;

    std::atomic<bool> running_{false};
    std::atomic<int>  pulseCount_{0};

    // Only ever written/read by edgeWorker — no atomic needed
    std::chrono::steady_clock::time_point lastPulseTime_{};

    std::thread edgeThread_;
    std::optional<gpiod::chip>         chip_;
    std::optional<gpiod::line_request> request_;
};
