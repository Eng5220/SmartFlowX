#include "state/FillingController.h"

#include <chrono>

using Clock = std::chrono::steady_clock;

// ─────────────────────────────────────────────────────────────────────────────

FillingController::FillingController(IProximitySensor& gestureSensor,
                                     IPump&           pump,
                                     IFlowMeter&      flowMeter,
                                     int             holdTimeSeconds,
                                     double          targetVolumeML)
    : gestureSensor_(gestureSensor),
      pump_(pump),
      flowMeter_(flowMeter),
      holdTimeSeconds_(holdTimeSeconds),
      targetVolumeML_(targetVolumeML),
      state_(SystemState::WAITING),
      holdStartTime_(),
      bottleCount_(0),
      monitorCallback_(nullptr)
{
    // Register proximity callback on the sensor abstraction.
    // Called from the sensor worker thread on proximity state change.
    // Callback is intentionally minimal: release-store only — no I/O, no blocking.
    // memory_order_release pairs with the acquire-load in tick() to ensure the
    // stored value is visible to the Timer thread before it reads it.
    gestureSensor_.registerEventCallback([this](const GestureEvent& event) {
        if (event.getState() == ProximityState::PROXIMITY_TRIGGERED) {
            bottlePresent_.store(true,  std::memory_order_release);
        } else if (event.getState() == ProximityState::PROXIMITY_CLEARED) {
            bottlePresent_.store(false, std::memory_order_release);
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────

void FillingController::tick() {

    switch (state_) {

    // ── WAITING: no cup detected yet ─────────────────────────────────────────
    case SystemState::WAITING: {
        // acquire-load pairs with the release-store in the proximity callback
        if (bottlePresent_.load(std::memory_order_acquire)) {
            holdStartTime_ = Clock::now();
            state_ = SystemState::CONFIRMATION;
        }
        break;
    }

    // ── CONFIRMATION: cup must stay present for holdTimeSeconds_ ─────────────
    case SystemState::CONFIRMATION: {
        if (!bottlePresent_.load(std::memory_order_acquire)) {
            // Cup removed before timer expired — reset
            state_ = SystemState::WAITING;
            break;
        }

        if (getHoldElapsed() >= holdTimeSeconds_) {
            // Confirmed — reset meter and start pump
            flowMeter_.resetCount();
            pump_.turnOn();
            state_ = SystemState::FILLING;
        }
        break;
    }

    // ── FILLING: pump running, counting flow pulses ───────────────────────────
    case SystemState::FILLING: {
        double currentML = flowMeter_.getVolumeML();
        if (currentML >= targetVolumeML_) {
            pump_.turnOff();
            bottleCount_++;
            state_ = SystemState::FILL_COMPLETE;
        }
        break;
    }

    // ── FILL_COMPLETE: pump off, reset for next cup ───────────────────────────
    case SystemState::FILL_COMPLETE: {
        flowMeter_.resetCount();
        state_ = SystemState::WAITING;
        break;
    }
    }

    // Notify observer (Monitor + LCD) after every tick — volume updates happen here
    if (monitorCallback_) {
        monitorCallback_(getStateName(), flowMeter_.getVolumeML(), bottleCount_);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

SystemState FillingController::getState() const {
    return state_;
}

std::string FillingController::getStateName() const {
    switch (state_) {
        case SystemState::WAITING:       return "WAITING";
        case SystemState::CONFIRMATION:  return "CONFIRMATION";
        case SystemState::FILLING:       return "FILLING";
        case SystemState::FILL_COMPLETE: return "FILL_COMPLETE";
        default:                         return "UNKNOWN";
    }
}

double FillingController::getHoldElapsed() const {
    if (state_ != SystemState::CONFIRMATION && state_ != SystemState::FILLING) {
        return 0.0;
    }
    auto now = Clock::now();
    return std::chrono::duration<double>(now - holdStartTime_).count();
}

double FillingController::getCurrentVolumeML() const {
    return flowMeter_.getVolumeML();
}

double FillingController::getTargetVolumeML() const {
    return targetVolumeML_;
}

int FillingController::getBottleCount() const {
    return bottleCount_;
}

#ifdef AQUAFLOW_TESTING
void FillingController::forceHoldElapsedForTest(double secondsElapsed) {
    holdStartTime_ = Clock::now() - std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(secondsElapsed));
}
#endif
