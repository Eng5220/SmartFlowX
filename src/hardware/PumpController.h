#ifndef PUMPCONTROLLER_H
#define PUMPCONTROLLER_H

#include "IHardwareDevice.h"

/**
 * @brief DC Water Pump controller via TIP121 transistor.
 *
 * Sets a GPIO pin HIGH to switch the transistor on (pump runs),
 * LOW to switch it off (pump stops).
 */
class PumpController : public IHardwareDevice {
public:
    /**
     * @param gpioHandle  Handle returned by lgGpiochipOpen().
     * @param pumpPin     BCM pin number for pump control output.
     */
    PumpController(int gpioHandle, int pumpPin);
    ~PumpController() override;

    bool init() override;
    void shutdown() override;

    /** @brief Turn the pump ON. */
    void turnOn();

    /** @brief Turn the pump OFF. */
    void turnOff();

    /** @brief Check if the pump is currently running. */
    bool isRunning() const;

private:
    int handle_;
    int pumpPin_;
    bool running_;
    bool initialised_;
};

#endif // PUMPCONTROLLER_H
