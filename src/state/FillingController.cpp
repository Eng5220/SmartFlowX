#include "state/FillingController.h"

#include <iostream>
#include <iomanip>

using Clock = std::chrono::steady_clock;

// ─────────────────────────────────────────────────────────────────────────────

FillingController::FillingController(UltrasonicSensor& sensor,
                                     PumpController& pump,
                                     FlowMeter& flowMeter,
                                     double targetDistanceCM,
                                     double toleranceCM,
                                     int holdTimeSeconds,
                                     double targetVolumeML)
    : sensor_(sensor),
      pump_(pump),
      flowMeter_(flowMeter),
      targetDistanceCM_(targetDistanceCM),
      toleranceCM_(toleranceCM),
      holdTimeSeconds_(holdTimeSeconds),
      targetVolumeML_(targetVolumeML),
      state_(SystemState::WAITING),
      holdStartTime_(),
      lastDistance_(-1.0),
      bottleCount_(0)
{
}

// ─────────────────────────────────────────────────────────────────────────────

void FillingController::tick() {
    std::cout << std::fixed << std::setprecision(2);

    switch (state_) {

    // ── WAITING: looking for a bottle ────────────────────────────────────────
    case SystemState::WAITING: {
        lastDistance_ = sensor_.getDistanceCM();

        if (lastDistance_ < 0) {
            std::cout << "Sensor timeout\n";
            return;
        }

        std::cout << "Measured Distance = " << lastDistance_ << " cm\n";

        if (sensor_.isBottlePresent(targetDistanceCM_, toleranceCM_)) {
            // Bottle detected — start hold timer
            holdStartTime_ = Clock::now();
            state_ = SystemState::CONFIRMATION;
            std::cout << targetDistanceCM_ << " cm detected, starting timer...\n";
        }
        break;
    }

    // ── CONFIRMATION: bottle must stay stable for holdTimeSeconds_ ───────────
    case SystemState::CONFIRMATION: {
        lastDistance_ = sensor_.getDistanceCM();

        if (lastDistance_ < 0) {
            std::cout << "Sensor timeout, timer reset.\n";
            state_ = SystemState::WAITING;
            return;
        }

        std::cout << "Measured Distance = " << lastDistance_ << " cm\n";

        if (!sensor_.isBottlePresent(targetDistanceCM_, toleranceCM_)) {
            // Bottle moved away — reset timer
            std::cout << "Distance moved away from " << targetDistanceCM_
                      << " cm, timer reset.\n";
            state_ = SystemState::WAITING;
            break;
        }

        // Bottle still present — check elapsed time
        double elapsed = getHoldElapsed();
        std::cout << std::setprecision(1)
                  << "Held for " << elapsed << " / " << holdTimeSeconds_
                  << " seconds\n";

        if (elapsed >= holdTimeSeconds_) {
            // Hold time reached — reset flow counter and start pump
            flowMeter_.resetCount();
            pump_.turnOn();
            state_ = SystemState::FILLING;
            std::cout << "Bottle confirmed! Starting fill to "
                      << std::setprecision(0) << targetVolumeML_ << " ml\n";
        }
        break;
    }

    // ── FILLING: pump running, counting flow pulses ──────────────────────────
    case SystemState::FILLING: {
        double currentML = flowMeter_.getVolumeML();
        int pulses = flowMeter_.getPulseCount();

        std::cout << std::setprecision(1)
                  << "Filling: " << currentML << " ml / "
                  << targetVolumeML_ << " ml"
                  << "  (" << pulses << " pulses)\n";

        if (flowMeter_.hasReachedTarget(targetVolumeML_)) {
            // Target volume reached — stop pump
            pump_.turnOff();
            bottleCount_++;
            state_ = SystemState::FILL_COMPLETE;
            std::cout << std::setprecision(1)
                      << "Target reached! Dispensed " << currentML << " ml. "
                      << "Bottles filled: " << bottleCount_ << "\n";
        }
        break;
    }

    // ── FILL_COMPLETE: pump off, ready for next bottle ───────────────────────
    case SystemState::FILL_COMPLETE: {
        // Reset flow counter for next bottle
        flowMeter_.resetCount();
        state_ = SystemState::WAITING;
        std::cout << "Fill complete. Waiting for next bottle...\n";
        break;
    }
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
