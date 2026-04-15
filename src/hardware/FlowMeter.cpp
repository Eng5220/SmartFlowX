#include "hardware/FlowMeter.h"

#include <stdexcept>
#include <string>

#include "utils/Logger.h"

// ── IHardwareDevice ───────────────────────────────────────────────────────────

bool FlowMeter::init() {
    if (running_) return true; 
    try {
        setupGpio();
        running_ = true;
        pulseCount_ = 0;
        edgeThread_ = std::thread(&FlowMeter::edgeWorker, this);
        return true;
    } catch (const std::exception& e) {
        shutdown();
        return false;
    }
}

#ifdef AQUAFLOW_TESTING
void FlowMeter::injectPulseCountForTest(int pulseCount) {
    pulseCount_.store(pulseCount);
}
#endif

void FlowMeter::shutdown() {
    running_ = false;
    if (edgeThread_.joinable()) edgeThread_.join();
    // Release libgpiod resources safely regardless of internal failure state
    if (request_) request_.reset();
    if (chip_) chip_.reset();
}

// ── Flow data API ─────────────────────────────────────────────────────────────

void FlowMeter::resetCount() {
    pulseCount_.store(0);
}

int FlowMeter::getPulseCount() const {
    return pulseCount_.load();
}

double FlowMeter::getVolumeML() const {
    return static_cast<double>(pulseCount_.load()) * mlPerPulse_;
}

// ── Private ───────────────────────────────────────────────────────────────────

void FlowMeter::setupGpio() {
    const std::string chipPath = "/dev/gpiochip" + std::to_string(chipNo_);
    chip_ = gpiod::chip(chipPath);

    gpiod::line_config lineCfg;
    lineCfg.add_line_settings(
        pinNo_,
        gpiod::line_settings()
            .set_direction(gpiod::line::direction::INPUT)
            // YF-S401 signal is open-collector: enable the Pi's internal 50kΩ
            // pull-up so the line idles HIGH and pulses LOW on each flow tick.
            // Without this the signal floats and produces spurious counts.
            .set_bias(gpiod::line::bias::PULL_UP)
            .set_edge_detection(gpiod::line::edge::FALLING));

    auto builder = chip_->prepare_request();
    builder.set_consumer("flow_meter");
    builder.set_line_config(lineCfg);
    request_ = builder.do_request();
}

void FlowMeter::edgeWorker() {
    // Initialise to epoch so the very first real pulse is always accepted
    lastPulseTime_ = std::chrono::steady_clock::time_point{};

    try {
        while (running_) {
            // Blocking wait - thread sleeps until a pulse arrives or 200 ms timeout
            if (request_->wait_edge_events(std::chrono::milliseconds(200))) {
                gpiod::edge_event_buffer buffer(8);
                std::size_t num = request_->read_edge_events(buffer);

                for (std::size_t i = 0; i < num; ++i) {
                    if (buffer.get_event(i).type() ==
                        gpiod::edge_event::event_type::FALLING_EDGE) {

                        auto now = std::chrono::steady_clock::now();
                        auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       now - lastPulseTime_).count();

                        // ignore any two pulses that arrive too quickly
                        // Real water flow from the sensor produces pulses about every 10 ms
                        // electrical noise shows up much faster like in microseconds.
                        // So if the gap between pulses is at least DEBOUNCE_MS (5 ms)
                        // we count it but if its less than that we treat it as noise and skip it.
                        if (gap >= DEBOUNCE_MS) {
                            pulseCount_++;
                            lastPulseTime_ = now;
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::error("FlowMeter hardware fault (thread aborted): " + std::string(e.what()));
        running_ = false;
    }
}


