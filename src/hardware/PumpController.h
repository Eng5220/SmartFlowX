#pragma once

#include "IHardwareDevice.h"
#include "IPump.h"

#include <optional>
#include <string>

#include <gpiod.hpp>

/**
 * @brief DC Water Pump controller via relay module or TIP121 transistor.
 *
 * Uses libgpiod v2 (the officially supported GPIO library for Linux)
 * to drive a single output pin HIGH/LOW.  This replaces the previous
 * lgpio-based implementation which used a hobbyist library not officially
 * supported on Raspberry Pi OS.
 *
 * Relay mode (active-LOW): pass DriveMode::RELAY_ACTIVE_LOW
 * Transistor mode:         pass DriveMode::TRANSISTOR_ACTIVE_HIGH
 */
class PumpController : public IHardwareDevice, public IPump {
public:
    enum class DriveMode {
        RELAY_ACTIVE_LOW,
        TRANSISTOR_ACTIVE_HIGH
    };

    /**
     * @param chipNo   GPIO chip number (e.g. 4 for /dev/gpiochip4 on Pi 5).
     * @param pumpPin  BCM pin number for pump control output.
     * @param mode     Drive polarity mode (default: TRANSISTOR_ACTIVE_HIGH).
     */
    PumpController(unsigned int chipNo, unsigned int pumpPin, DriveMode mode = DriveMode::TRANSISTOR_ACTIVE_HIGH);
    ~PumpController() override;

    PumpController(const PumpController&) = delete;
    PumpController& operator=(const PumpController&) = delete;
    PumpController(PumpController&&) = delete;
    PumpController& operator=(PumpController&&) = delete;

    bool init() override;
    void shutdown() override;

    /** @brief Turn the pump ON. */
    void turnOn() override;

    /** @brief Turn the pump OFF. */
    void turnOff() override;

    /** @brief True while the pump is running. */
    bool isRunning() const override;

#ifdef AQUAFLOW_TESTING
    /** @brief Enable a hardware-free simulation mode for unit tests. */
    void enableSimulationForTest();
#endif

private:
    DriveMode mode_;

    gpiod::line::value onValue()  const;
    gpiod::line::value offValue() const;

    unsigned int chipNo_;
    unsigned int pumpPin_;
    bool running_    = false;
    bool initialised_= false;

    std::optional<gpiod::chip>         chip_;
    std::optional<gpiod::line_request> request_;
};
