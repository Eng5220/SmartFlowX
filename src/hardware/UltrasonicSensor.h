#pragma once
// Prevents the header from being included multiple times

#include <atomic>           // Thread-safe variables
#include <chrono>           // Timing operations
#include <cstdint>          // Fixed-width integer types
#include <functional>       // Callback function support
#include <memory>           // Smart pointers
#include <stdexcept>        // Runtime error handling
#include <string>           // String handling
#include <thread>           // Multithreading

#include <unistd.h>         // POSIX read/close
#include <sys/timerfd.h>    // Linux periodic timer

#include <gpiod.hpp>        // GPIO access library for Raspberry Pi


/**
 * @brief HC-SR04 Ultrasonic Distance Sensor driver.
 *
 * Sends a 10µs trigger pulse, waits for echo rising/falling edges,
 * and calculates distance from the echo pulse width.
 *
 * Uses a timerfd-based worker thread for periodic measurements
 * and delivers results via registered callbacks.
 */
class UltrasonicSensor {
public:
    using DistanceCallback = std::function<void(float)>;             // distance in cm
    using ErrorCallback = std::function<void(const std::string&)>;   // error reporting

    // Constructor — configures which GPIO chip/pins to use
    //   chipNo:        GPIO chip number (e.g. 0 for /dev/gpiochip0, 4 for Pi 5)
    //   trigPin:        BCM pin for trigger output
    //   echoPin:        BCM pin for echo input
    //   samplePeriodMs: how often to measure (default 200ms = 5 Hz)
    UltrasonicSensor(unsigned int chipNo,
                     unsigned int trigPin,
                     unsigned int echoPin,
                     int samplePeriodMs = 200)
        : chipNo_(chipNo),
          trigPin_(trigPin),
          echoPin_(echoPin),
          samplePeriodMs_(samplePeriodMs) {}

    // Destructor — ensures sensor is stopped cleanly
    ~UltrasonicSensor() {
        stop();
    }

    // Register a callback for distance readings (called every measurement cycle)
    void registerDistanceCallback(DistanceCallback cb) {
        distanceCallback_ = std::move(cb);
    }

    // Register a callback for error messages (timeouts, bad readings, etc.)
    void registerErrorCallback(ErrorCallback cb) {
        errorCallback_ = std::move(cb);
    }

    // Start the sensor — sets up GPIO, timer, and launches worker thread
    void start() {
        if (running_) return;       // Prevent starting twice

        setupGpio();                // Configure TRIG and ECHO pins
        setupTimerFd();             // Create periodic measurement timer

        running_ = true;
        workerThread_ = std::thread(&UltrasonicSensor::worker, this);
    }

    // Stop the sensor — signals thread to exit and waits for it
    void stop() {
        if (!running_) return;

        running_ = false;           // Signal worker thread to exit

        // Close the timer so the blocking read() unblocks
        if (timerFd_ >= 0) {
            close(timerFd_);
            timerFd_ = -1;
        }

        // Wait for worker thread to finish
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }

private:

    // ── GPIO Setup ───────────────────────────────────────────────────────────

    // Configure TRIG as output and ECHO as input with edge detection
    void setupGpio() {
        const std::string chipPath = "/dev/gpiochip" + std::to_string(chipNo_);
        chip_ = std::make_shared<gpiod::chip>(chipPath);

        // TRIG pin — output, starts LOW
        {
            gpiod::line_config trigCfg;
            trigCfg.add_line_settings(
                {trigPin_},
                gpiod::line_settings{}
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE));

            auto builder = chip_->prepare_request();
            builder.set_consumer("ultrasonic_trig");
            builder.set_line_config(trigCfg);
            trigRequest_ = std::make_shared<gpiod::line_request>(builder.do_request());
        }

        // ECHO pin — input, detect both rising and falling edges
        {
            gpiod::line_config echoCfg;
            echoCfg.add_line_settings(
                {echoPin_},
                gpiod::line_settings{}
                    .set_direction(gpiod::line::direction::INPUT)
                    .set_edge_detection(gpiod::line::edge::BOTH));

            auto builder = chip_->prepare_request();
            builder.set_consumer("ultrasonic_echo");
            builder.set_line_config(echoCfg);
            echoRequest_ = std::make_shared<gpiod::line_request>(builder.do_request());
        }
    }

    // ── Timer Setup ──────────────────────────────────────────────────────────

    // Create a periodic timer that fires every samplePeriodMs_ milliseconds
    void setupTimerFd() {
        timerFd_ = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerFd_ < 0) {
            throw std::runtime_error("Failed to create timerfd");
        }

        // Convert milliseconds to seconds + nanoseconds
        itimerspec its{};
        its.it_value.tv_sec = samplePeriodMs_ / 1000;
        its.it_value.tv_nsec = (samplePeriodMs_ % 1000) * 1000000;
        its.it_interval.tv_sec = samplePeriodMs_ / 1000;
        its.it_interval.tv_nsec = (samplePeriodMs_ % 1000) * 1000000;

        if (timerfd_settime(timerFd_, 0, &its, nullptr) < 0) {
            close(timerFd_);
            timerFd_ = -1;
            throw std::runtime_error("Failed to configure timerfd");
        }
    }

    // ── Worker Thread ────────────────────────────────────────────────────────

    // Main measurement loop — blocks on timerfd, measures, reports result
    void worker() {
        while (running_) {
            // Block until timer fires (every samplePeriodMs_)
            uint64_t expirations = 0;
            const ssize_t s = read(timerFd_, &expirations, sizeof(expirations));

            if (!running_) break;

            if (s != static_cast<ssize_t>(sizeof(expirations))) {
                publishError("Timer read failed");
                continue;
            }

            // Take a distance measurement
            const float distanceCm = measureDistanceCm();

            // Report error or success via callbacks
            if (distanceCm < 0.0f) {
                if (distanceCm == -1.0f) {
                    publishError("Echo never went HIGH");
                } else if (distanceCm == -2.0f) {
                    publishError("Echo never went LOW");
                } else if (distanceCm == -3.0f) {
                    publishError("Unexpected GPIO edge sequence");
                } else {
                    publishError("Measurement failed");
                }
            } else {
                if (distanceCallback_) {
                    distanceCallback_(distanceCm);
                }
            }
        }
    }

    // ── Distance Measurement ─────────────────────────────────────────────────

    // Perform one HC-SR04 measurement cycle:
    //   1. Send 10µs trigger pulse
    //   2. Wait for echo rising edge  (sound wave sent)
    //   3. Wait for echo falling edge (sound wave returned)
    //   4. Calculate distance from pulse width
    // Returns distance in cm, or negative value on error
    float measureDistanceCm() {
        clearPendingEchoEvents();   // Discard stale echo events

        // Ensure trigger starts LOW
        setTrig(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        // Send 10µs trigger pulse
        setTrig(true);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        setTrig(false);

        // Wait for echo rising edge (sound wave leaves sensor)
        gpiod::edge_event risingEvent;
        if (!waitForEdge(gpiod::edge_event_types::event_type::RISING_EDGE,
                         std::chrono::milliseconds(50),
                         risingEvent)) {
            return -1.0f;           // Timeout — no echo received
        }

        // Wait for echo falling edge (sound wave returns)
        gpiod::edge_event fallingEvent;
        if (!waitForEdge(gpiod::edge_event_types::event_type::FALLING_EDGE,
                         std::chrono::milliseconds(50),
                         fallingEvent)) {
            return -2.0f;           // Timeout — echo stuck HIGH
        }

        // Calculate pulse width from kernel timestamps (nanoseconds)
        const auto tStart = risingEvent.timestamp_ns();
        const auto tEnd = fallingEvent.timestamp_ns();

        if (tEnd <= tStart) {
            return -3.0f;           // Invalid — falling before rising
        }

        // Convert nanoseconds → microseconds
        const double pulseUs = static_cast<double>(tEnd - tStart) / 1000.0;

        // Distance = (pulse_time × speed_of_sound) / 2
        // Speed of sound = 34300 cm/s = 0.0343 cm/µs
        // Divide by 2 because sound travels to object and back
        return static_cast<float>((pulseUs * 0.0343) / 2.0);
    }

    // ── Edge Detection Helpers ───────────────────────────────────────────────

    // Wait for a specific GPIO edge event (RISING or FALLING) with timeout
    // Returns true if the wanted edge was found, false on timeout
    bool waitForEdge(int wantedType,
                     std::chrono::milliseconds timeout,
                     gpiod::edge_event& matchedEvent) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (running_) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return false;       // Timeout reached
            }

            // Wait for any edge event up to remaining time
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);

            const bool ready = echoRequest_->wait_edge_events(remaining);
            if (!ready) {
                return false;       // No event before deadline
            }

            // Read events from kernel buffer
            gpiod::edge_event_buffer buffer(8);
            const std::size_t numEvents = echoRequest_->read_edge_events(buffer);

            // Check if any event matches the wanted type
            for (std::size_t i = 0; i < numEvents; ++i) {
                const auto ev = buffer.get_event(i);

                if (ev.event_type() == wantedType) {
                    matchedEvent = ev;
                    return true;    // Found it
                }
            }
        }

        return false;
    }

    // Drain any stale echo events left over from previous measurements
    void clearPendingEchoEvents() {
        while (true) {
            // Check if any events are waiting (timeout = 0 = don't block)
            const bool ready = echoRequest_->wait_edge_events(std::chrono::milliseconds(0));
            if (!ready) {
                break;              // No pending events
            }

            // Read and discard them
            gpiod::edge_event_buffer buffer(8);
            const std::size_t numEvents = echoRequest_->read_edge_events(buffer);
            if (numEvents == 0) {
                break;
            }
        }
    }

    // Set the trigger pin HIGH or LOW
    void setTrig(bool on) {
        trigRequest_->set_value(
            trigPin_,
            on ? gpiod::line::value::ACTIVE
               : gpiod::line::value::INACTIVE);
    }

    // Send an error message through the registered error callback
    void publishError(const std::string& msg) {
        if (errorCallback_) {
            errorCallback_(msg);
        }
    }

    // ── Member Variables ─────────────────────────────────────────────────────

private:
    unsigned int chipNo_;       // GPIO chip number (e.g. 0 or 4)
    unsigned int trigPin_;      // BCM pin for TRIG output
    unsigned int echoPin_;      // BCM pin for ECHO input
    int samplePeriodMs_;        // Measurement interval in milliseconds

    std::atomic<bool> running_{false};  // Thread-safe running flag
    int timerFd_{-1};                   // Linux timerfd file descriptor

    DistanceCallback distanceCallback_; // Called with distance in cm
    ErrorCallback errorCallback_;       // Called with error messages

    std::thread workerThread_;          // Background measurement thread

    std::shared_ptr<gpiod::chip> chip_;                  // GPIO chip handle
    std::shared_ptr<gpiod::line_request> trigRequest_;    // TRIG pin handle
    std::shared_ptr<gpiod::line_request> echoRequest_;    // ECHO pin handle
};
