#include "hardware/PumpController.h"

#include <lgpio.h>
#include <iostream>

// ─── Relay mode ──────────────────────────────────────────────────────────────
// Set to true  if using a 1-channel relay module (active-LOW: IN=0 → pump ON).
// Set to false if using a TIP121/MOSFET transistor  (active-HIGH: pin=1 → pump ON).
static constexpr bool RELAY_MODE = true;

// ─────────────────────────────────────────────────────────────────────────────

PumpController::PumpController(int gpioHandle, int pumpPin)
    : handle_(gpioHandle),
      pumpPin_(pumpPin),
      running_(false),
      initialised_(false)
{
}

PumpController::~PumpController() {
    shutdown();
}

bool PumpController::init() {
    // Start idle: relay-mode idles HIGH (de-energised), transistor-mode idles LOW.
    int idleLevel = RELAY_MODE ? 1 : 0;
    if (lgGpioClaimOutput(handle_, 0, pumpPin_, idleLevel) != 0) {
        std::cerr << "ERROR: Failed to claim PUMP pin " << pumpPin_ << "\n";
        return false;
    }

    lgGpioWrite(handle_, pumpPin_, idleLevel);  // ensure pump is OFF
    running_ = false;
    initialised_ = true;

    std::cout << "PumpController initialised (PIN=" << pumpPin_
              << ", mode=" << (RELAY_MODE ? "RELAY(active-LOW)" : "TRANSISTOR(active-HIGH)")
              << ")\n";
    return true;
}

void PumpController::shutdown() {
    if (initialised_) {
        int idleLevel = RELAY_MODE ? 1 : 0;
        lgGpioWrite(handle_, pumpPin_, idleLevel);  // always de-energise on shutdown
        lgGpioFree(handle_, pumpPin_);
        running_ = false;
        initialised_ = false;
        std::cout << "PumpController shut down (pump OFF).\n";
    }
}

void PumpController::turnOn() {
    if (initialised_ && !running_) {
        // Relay module: de-energise = HIGH, energise (pump ON) = LOW
        int onLevel = RELAY_MODE ? 0 : 1;
        lgGpioWrite(handle_, pumpPin_, onLevel);
        running_ = true;
        std::cout << "Pump started!\n";
    }
}

void PumpController::turnOff() {
    if (initialised_ && running_) {
        int offLevel = RELAY_MODE ? 1 : 0;
        lgGpioWrite(handle_, pumpPin_, offLevel);
        running_ = false;
        std::cout << "Pump stopped.\n";
    }
}

bool PumpController::isRunning() const {
    return running_;
}
