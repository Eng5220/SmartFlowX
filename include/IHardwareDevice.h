#ifndef IHARDWAREDEVICE_H
#define IHARDWAREDEVICE_H

/**
 * @brief Abstract base interface for all hardware devices.
 *
 * Every hardware class (UltrasonicSensor, PumpController, FlowMeter)
 * implements this interface. This supports:
 *   - Liskov Substitution (SOLID "L") — mock objects can replace real hardware
 *   - Dependency Inversion (SOLID "D") — FillingController depends on
 *     abstractions, not concrete GPIO classes
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
