#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <string>
#include <iostream>
#include <gpiod.hpp>


class FlowSensor {
public:
    using FlowCallback = std::function<void(float)>;

    FlowSensor(unsigned int chipNo, unsigned int pinNo, float cal = 98.0f)
        : chipNo_(chipNo), pinNo_(pinNo), calibrationFactor_(cal) {}

    ~FlowSensor() { stop(); }

    void registerFlowCallback(FlowCallback cb) { flowCallback_ = std::move(cb); }

    void start() {
        if (running_) return;
        setupGpio();
        running_ = true;
        pulseCount_ = 0;
        sampleStart_ = std::chrono::steady_clock::now();
        edgeThread_ = std::thread(&FlowSensor::edgeWorker, this);
        rateThread_ = std::thread(&FlowSensor::rateWorker, this);
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        if (edgeThread_.joinable()) edgeThread_.join();
        if (rateThread_.joinable()) rateThread_.join();
    }

private:
    void setupGpio() {
        const std::string chipPath = "/dev/gpiochip" + std::to_string(chipNo_);
        chip_ = std::make_shared<gpiod::chip>(chipPath);
        
        gpiod::line_config lineCfg;
        lineCfg.add_line_settings({pinNo_},
            gpiod::line_settings()
                .set_direction(gpiod::line::direction::INPUT)
                .set_edge_detection(gpiod::line::edge::FALLING));

        auto builder = chip_->prepare_request();
        builder.set_consumer("flow_sensor");
        builder.set_line_config(lineCfg);
        request_ = std::make_shared<gpiod::line_request>(builder.do_request());
    }

    void edgeWorker() {
        while (running_) {
            if (request_->wait_edge_events(std::chrono::milliseconds(200))) {
                gpiod::edge_event_buffer buffer(8);
                std::size_t num = request_->read_edge_events(buffer);
                for (size_t i = 0; i < num; ++i) {
                    if (buffer.get_event(i).event_type() == gpiod::edge_event::event_type::FALLING_EDGE) {
                        pulseCount_++;
                    }
                }
            }
        }
    }

    void rateWorker() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> dt = now - sampleStart_;
            sampleStart_ = now;

            unsigned long pulses = pulseCount_.exchange(0);
            float freq = (dt.count() > 0) ? (static_cast<float>(pulses) / dt.count()) : 0;
            if (flowCallback_) flowCallback_(freq / calibrationFactor_);
        }
    }

    unsigned int chipNo_, pinNo_;
    float calibrationFactor_;
    std::atomic<bool> running_{false};
    std::atomic<unsigned long> pulseCount_{0};
    std::chrono::steady_clock::time_point sampleStart_;
    FlowCallback flowCallback_;
    std::thread edgeThread_, rateThread_;
    std::shared_ptr<gpiod::chip> chip_;
    std::shared_ptr<gpiod::line_request> request_;
};