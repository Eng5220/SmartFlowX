#include "hardware/PumpController.h"

#include <stdexcept>
#include "utils/Logger.h"

// ─────────────────────────────────────────────────────────────────────────────

PumpController::PumpController(unsigned int chipNo, unsigned int pumpPin, DriveMode mode)
    : chipNo_(chipNo), pumpPin_(pumpPin), mode_(mode) {}

PumpController::~PumpController() {
    shutdown();
}

// ── IHardwareDevice ───────────────────────────────────────────────────────────

bool PumpController::init() {
    if (initialised_) return true;

    try {
        const std::string chipPath = "/dev/gpiochip" + std::to_string(chipNo_);
        chip_ = gpiod::chip(chipPath);

        // Configure the pump pin as output, idle = pump OFF
        gpiod::line_config lineCfg;
        lineCfg.add_line_settings(
            pumpPin_,
            gpiod::line_settings()
                .set_direction(gpiod::line::direction::OUTPUT)
                .set_output_value(offValue()));   // start de energised

        auto builder = chip_->prepare_request();
        builder.set_consumer("pump_controller");
        builder.set_line_config(lineCfg);
        request_ = builder.do_request();

        running_     = false;
        initialised_ = true;

        Logger::info("PumpController initialised via libgpiod (chip=" +
                     std::to_string(chipNo_) + ", pin=" + std::to_string(pumpPin_) +
                     ", mode=" + std::string((mode_ == DriveMode::RELAY_ACTIVE_LOW) ? "RELAY(active-LOW)" : "TRANSISTOR(active-HIGH)") +
                     ")");
        return true;

    } catch (const std::exception& e) {
        Logger::error("PumpController::init() error: " + std::string(e.what()));
        return false;
    }
}

void PumpController::shutdown() {
    if (initialised_ && request_) {
        // Ensure pump is off before releasing GPIO
        request_->set_value(pumpPin_, offValue());
    }
    running_     = false;
    initialised_ = false;
    if (request_) request_.reset();
    if (chip_) chip_.reset();
    Logger::info("PumpController shut down (pump OFF).");
}

// ── Pump control ──────────────────────────────────────────────────────────────

void PumpController::turnOn() {
    if (!initialised_ || running_) return;
    if (request_) {
        request_->set_value(pumpPin_, onValue());
    }
    running_ = true;
    Logger::info("Pump started!");
}

void PumpController::turnOff() {
    if (!initialised_ || !running_) return;
    if (request_) {
        request_->set_value(pumpPin_, offValue());
    }
    running_ = false;
    Logger::info("Pump stopped.");
}

bool PumpController::isRunning() const { return running_; }

#ifdef AQUAFLOW_TESTING
void PumpController::enableSimulationForTest() {
    initialised_ = true;
    running_ = false;
    request_.reset();
    chip_.reset();
}
#endif

// ── Helpers ───────────────────────────────────────────────────────────────────

gpiod::line::value PumpController::onValue() const {
    // Relay (active-LOW): energise coil by pulling LOW
    // Transistor (active-HIGH): switch ON by pulling HIGH
    return (mode_ == DriveMode::RELAY_ACTIVE_LOW) ? gpiod::line::value::INACTIVE
                                                  : gpiod::line::value::ACTIVE;
}

gpiod::line::value PumpController::offValue() const {
    return (mode_ == DriveMode::RELAY_ACTIVE_LOW) ? gpiod::line::value::ACTIVE
                                                  : gpiod::line::value::INACTIVE;
}
