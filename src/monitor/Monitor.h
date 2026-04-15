#pragma once

#include <string>

/**
 * @brief Receives state-change events from FillingController and
 *        logs them to the console (and optionally to a file).
 *
 * Registered as a callback observer — FillingController calls
 * onStateChange() whenever a transition occurs.  This decouples
 * display/logging from the state machine (SOLID SRP).
 */
class Monitor {
public:
    /**
     * @brief Called by FillingController on every tick.
     * @param state       Current state name (e.g. "FILLING").
     * @param volumeML    Volume dispensed so far in this fill cycle.
     * @param bottleCount Total bottles filled this session.
     */
    void onStateChange(const std::string& state, double volumeML, int bottleCount);

private:
    std::string lastState_ = "";
};
