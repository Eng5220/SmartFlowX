#ifndef FILLINGCONTROLLER_H
#define FILLINGCONTROLLER_H

#include "hardware/UltrasonicSensor.h"
#include "hardware/PumpController.h"
#include "hardware/FlowMeter.h"
#include <chrono>
#include <string>

/**
 * @brief System states for the filling cycle.
 *
 * WAITING       — No bottle detected, system idle
 * CONFIRMATION  — Bottle seen, waiting for stable hold (60 seconds)
 * FILLING       — Pump ON, counting flow pulses
 * FILL_COMPLETE — Target volume reached, pump OFF, resetting
 */
enum class SystemState {
    WAITING,
    CONFIRMATION,
    FILLING,
    FILL_COMPLETE
};

/**
 * @brief Main state machine that orchestrates the filling cycle.
 *
 * Translates the Python prototype's main loop logic:
 *   - Check ultrasonic distance → detect bottle
 *   - Hold timer (60 seconds) → confirm bottle stable
 *   - Start pump → count flow meter pulses
 *   - Stop pump when target volume reached
 *
 * The FillingController depends on abstractions (hardware objects),
 * not on raw GPIO — supporting Dependency Inversion (SOLID "D").
 */
class FillingController {
public:
    /**
     * @param sensor           Reference to the ultrasonic sensor.
     * @param pump             Reference to the pump controller.
     * @param flowMeter        Reference to the flow meter.
     * @param targetDistanceCM Centre of bottle detection range.
     * @param toleranceCM      ± tolerance for detection.
     * @param holdTimeSeconds  How long bottle must be stable.
     * @param targetVolumeML   Target fill volume in millilitres.
     */
    FillingController(UltrasonicSensor& sensor,
                      PumpController& pump,
                      FlowMeter& flowMeter,
                      double targetDistanceCM,
                      double toleranceCM,
                      int holdTimeSeconds,
                      double targetVolumeML);

    /**
     * @brief Run one cycle of the state machine.
     *
     * Call this repeatedly from main() in a loop.
     * This is the C++ equivalent of the Python while True loop body.
     */
    void tick();

    /** @brief Get the current system state. */
    SystemState getState() const;

    /** @brief Get a human-readable name for the current state. */
    std::string getStateName() const;

    /** @brief Get seconds elapsed in the hold timer (CONFIRMATION state). */
    double getHoldElapsed() const;

    /** @brief Get the current volume dispensed in ml. */
    double getCurrentVolumeML() const;

    /** @brief Get the target volume in ml. */
    double getTargetVolumeML() const;

    /** @brief Get the total number of bottles filled this session. */
    int getBottleCount() const;

private:
    // Hardware references
    UltrasonicSensor& sensor_;
    PumpController& pump_;
    FlowMeter& flowMeter_;

    // Settings
    double targetDistanceCM_;
    double toleranceCM_;
    int holdTimeSeconds_;
    double targetVolumeML_;

    // State
    SystemState state_;
    std::chrono::steady_clock::time_point holdStartTime_;
    double lastDistance_;
    int bottleCount_;
};

#endif // FILLINGCONTROLLER_H
