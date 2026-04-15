#ifndef IHARDWAREDEVICE_H
#define IHARDWAREDEVICE_H

/**
 * @brief Abstract base interface for all hardware devices.
 *
 * Every hardware device class (PumpController, FlowMeter, GestureSensor, LcdDisplay)
 * implements this interface. This supports:
 *   - Liskov Substitution (SOLID "L") — mock objects can replace real hardware
 *   - A stable lifecycle contract (`init` / `shutdown`) shared across devices
 *
 * LSP note:
 * This base interface intentionally contains only lifecycle operations.
 * It does not define a fixed getData() payload because AquaFlow devices
 * expose different data shapes (pump state, pulse counts, gesture events, text output).
 * Forcing one common return type here would push narrowing conversions into derived
 * classes and violate substitutability. Device-specific data contracts live in
 * focused interfaces such as IFlowMeter, IPump, and IProximitySensor.
 */
class IHardwareDevice {
public:
    virtual ~IHardwareDevice() = default;

    /** @brief Initialise the hardware (claim GPIO pins, set modes). */
    virtual bool init() = 0;

    /** @brief Shut down the hardware safely (release GPIO, turn off outputs). */
    virtual void shutdown() = 0;
};

#endif // IHARDWAREDEVICE_H
