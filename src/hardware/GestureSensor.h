#pragma once

#include "IHardwareDevice.h"
#include "IProximitySensor.h"
#include <thread>
#include <atomic>
#include <string>
#include <cstdint>

/**
 * @brief APDS-9960 Gesture & Proximity sensor driver for Linux I2C.
 *
 * Register configuration and gesture detection algorithm ported from the
 * Adafruit APDS9960 Arduino library (v1.3.1, BSD licence).
 * See: Adafruit_APDS9960_Library-1.3.1/ for the reference source.
 *
 * Adaptations for AquaFlow:
 *  - Uses Linux I2C (/dev/i2c-N) via ioctl(I2C_SLAVE) instead of Arduino Wire/TwoWire.
 *  - Uses timerfd for RTES-compliant, zero-CPU polling instead of Arduino delay()/millis().
 *  - Implements IHardwareDevice + IProximitySensor for SOLID interface conformance.
 *  - Event-driven callbacks instead of synchronous return values.
 */
class GestureSensor : public IHardwareDevice, public IProximitySensor {
public:
    GestureSensor(int i2cBus = 1, int i2cAddr = 0x39, int threshold = 40);
    ~GestureSensor() override;

    GestureSensor(const GestureSensor&) = delete;
    GestureSensor& operator=(const GestureSensor&) = delete;
    GestureSensor(GestureSensor&&) = delete;
    GestureSensor& operator=(GestureSensor&&) = delete;

    bool init() override;

    /** @brief Reaps polling thread and safely closes mapped file descriptors. */
    void shutdown() override;

    void registerEventCallback(IProximitySensor::EventCallback cb) override;
    void registerErrorCallback(IProximitySensor::ErrorCallback cb) override;

#ifdef AQUAFLOW_TESTING
    /** @brief Test seam for unit tests to inject a synthetic event. */
    void emitEventForTest(const GestureEvent& event) const;
#endif

private:
    void worker();

    // ── I2C register helpers ─────────────────────────────────────────────────
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);

    /** @brief Burst-read N bytes starting at register `reg` (single I2C transaction). */
    void readBurst(uint8_t reg, uint8_t* buf, uint8_t len);

    // ── Gesture detection (ported from Adafruit readGesture()) ────────────────
    /** @brief Non-blocking single-pass gesture read. Returns gesture code or 0. */
    uint8_t readGesture();
    bool gestureValid();
    void resetCounts();

    int i2cBus_;
    int i2cAddr_;
    int threshold_;
    int fd_{-1};       ///< I2C file descriptor
    int timerFd_{-1};  ///< timerfd used for RTES-compliant blocking wait (replaces sleep)

    std::atomic<bool> running_{false};
    std::thread workerThread_;

    IProximitySensor::EventCallback eventCallback_;
    IProximitySensor::ErrorCallback errorCallback_;

    // ── Adafruit-style gesture direction counters ─────────────────────────────
    // These track the "phase" of a gesture swipe (first half vs second half)
    // to determine direction via a state-machine approach.
    uint8_t UCount_{0};
    uint8_t DCount_{0};
    uint8_t LCount_{0};
    uint8_t RCount_{0};

    // Timestamp (steady_clock ms) of last non-zero gesture FIFO frame,
    // used for the 300ms timeout that finalises a gesture.
    uint64_t gestureLastActivity_{0};
};
