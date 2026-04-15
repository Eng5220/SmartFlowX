#include "monitor/Monitor.h"
#include "utils/Logger.h"

#include <sstream>
#include <iomanip>

void Monitor::onStateChange(const std::string& state,
                             double volumeML,
                             int    bottleCount) {
    if (state != lastState_) {
        if (!lastState_.empty()) {
            if (state == "CONFIRMATION") {
                Logger::info("Cup detected! Starting confirmation timer...");
            } else if (state == "WAITING") {
                if (lastState_ == "CONFIRMATION") {
                    Logger::info("Cup removed before confirmation - resetting.");
                } else if (lastState_ == "FILL_COMPLETE") {
                    Logger::info("Fill complete. Waiting for next cup...");
                }
            } else if (state == "FILLING") {
                Logger::info("Cup confirmed! Filling in progress...");
            } else if (state == "FILL_COMPLETE") {
                std::ostringstream oss;
                oss << "Target reached! Dispensed " << std::fixed << std::setprecision(1) << volumeML 
                    << " ml. Bottles filled: " << bottleCount;
                Logger::info(oss.str());
            }
        }
        lastState_ = state;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1)
        << "[State: " << state
        << " | Volume: " << volumeML << " ml"
        << " | Bottles: " << bottleCount << "]";
    Logger::info(oss.str());
}
