#include "IFlowMeter.h"
#include "IHardwareDevice.h"
#include "IPump.h"
#include "IProximitySensor.h"
#include "state/FillingController.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class ScriptedLifecycleDevice final : public IHardwareDevice {
public:
    explicit ScriptedLifecycleDevice(std::vector<bool> initScript)
        : initScript_(std::move(initScript)) {}

    bool init() override {
        ++initCalls_;
        if (initScript_.empty()) {
            running_ = true;
            return true;
        }

        const bool success = initScript_[scriptIndex_ % initScript_.size()];
        ++scriptIndex_;
        if (success) {
            running_ = true;
        }
        return success;
    }

    void shutdown() override {
        ++shutdownCalls_;
        running_ = false;
    }

    std::size_t initCalls() const { return initCalls_; }
    std::size_t shutdownCalls() const { return shutdownCalls_; }

private:
    std::vector<bool> initScript_;
    std::size_t scriptIndex_ = 0;
    std::size_t initCalls_ = 0;
    std::size_t shutdownCalls_ = 0;
    bool running_ = false;
};

class SyntheticSensorBase : public IHardwareDevice, public IProximitySensor {
public:
    bool init() override {
        ++initCalls_;
        emitCycle();
        return true;
    }

    void shutdown() override { ++shutdownCalls_; }

    void registerEventCallback(IProximitySensor::EventCallback cb) override {
        eventCallback_ = std::move(cb);
    }

    void registerErrorCallback(IProximitySensor::ErrorCallback cb) override {
        errorCallback_ = std::move(cb);
    }

    std::size_t initCalls() const { return initCalls_; }
    std::size_t shutdownCalls() const { return shutdownCalls_; }

protected:
    virtual void emitCycle() = 0;

    void emit(const GestureEvent& event) const {
        if (eventCallback_) {
            eventCallback_(event);
        }
    }

private:
    IProximitySensor::EventCallback eventCallback_;
    IProximitySensor::ErrorCallback errorCallback_;
    std::size_t initCalls_ = 0;
    std::size_t shutdownCalls_ = 0;
};

class SingleChannelSensor final : public SyntheticSensorBase {
protected:
    void emitCycle() override {
        emit({ProximityState::PROXIMITY_TRIGGERED, GestureDir::NONE, 64, {64.0f}});
        emit({ProximityState::PROXIMITY_CLEARED, GestureDir::NONE, 16, {16.0f}});
    }
};

class MultiChannelSensor final : public SyntheticSensorBase {
protected:
    void emitCycle() override {
        emit({ProximityState::PROXIMITY_TRIGGERED, GestureDir::NONE, 80, {80.0f, 77.0f, 83.0f, 79.0f}});
        emit({ProximityState::NONE, GestureDir::LEFT, 80, {80.0f, 77.0f, 83.0f, 79.0f}});
        emit({ProximityState::PROXIMITY_CLEARED, GestureDir::NONE, 20, {20.0f, 18.0f, 22.0f, 19.0f}});
    }
};

float primaryValue(const GestureEvent& event) {
    if (!event.getProximityChannels().empty()) {
        return event.getProximityChannels().front();
    }
    return static_cast<float>(event.getProximityValue());
}

struct CallbackStats {
private:
    std::size_t totalEvents = 0;
    std::size_t multiChannelEvents = 0;
    float lastPrimary = 0.0f;
public:
    void incTotal() { ++totalEvents; }
    void incMultiChannel() { ++multiChannelEvents; }
    void setLastPrimary(float val) { lastPrimary = val; }
    
    std::size_t getTotalEvents() const { return totalEvents; }
    std::size_t getMultiChannelEvents() const { return multiChannelEvents; }
    float getLastPrimary() const { return lastPrimary; }
};

class PumpSpy final : public IPump {
public:
    void turnOn() override {
        running_ = true;
        ++turnOnCalls_;
    }

    void turnOff() override {
        running_ = false;
        ++turnOffCalls_;
    }

    bool isRunning() const override { return running_; }

    int turnOnCalls() const { return turnOnCalls_; }
    int turnOffCalls() const { return turnOffCalls_; }

private:
    bool running_ = false;
    int turnOnCalls_ = 0;
    int turnOffCalls_ = 0;
};

class FlowMeterSpy final : public IFlowMeter {
public:
    void resetCount() override {
        ++resetCalls_;
        pulseCount_ = 0;
        volumeML_ = 0.0;
    }

    int getPulseCount() const override { return pulseCount_; }
    double getVolumeML() const override { return volumeML_; }

    void setVolumeML(double volumeML) { volumeML_ = volumeML; }
    int resetCalls() const { return resetCalls_; }

private:
    int pulseCount_ = 0;
    double volumeML_ = 0.0;
    int resetCalls_ = 0;
};

class ManualSensorSingle final : public IProximitySensor {
public:
    void registerEventCallback(IProximitySensor::EventCallback cb) override {
        eventCallback_ = std::move(cb);
    }

    void registerErrorCallback(IProximitySensor::ErrorCallback cb) override {
        errorCallback_ = std::move(cb);
    }

    void emitTriggered() const {
        if (eventCallback_) {
            eventCallback_({ProximityState::PROXIMITY_TRIGGERED, GestureDir::NONE, 71, {71.0f}});
        }
    }

private:
    IProximitySensor::EventCallback eventCallback_;
    IProximitySensor::ErrorCallback errorCallback_;
};

class ManualSensorMulti final : public IProximitySensor {
public:
    void registerEventCallback(IProximitySensor::EventCallback cb) override {
        eventCallback_ = std::move(cb);
    }

    void registerErrorCallback(IProximitySensor::ErrorCallback cb) override {
        errorCallback_ = std::move(cb);
    }

    void emitTriggered() const {
        if (eventCallback_) {
            eventCallback_({ProximityState::PROXIMITY_TRIGGERED, GestureDir::NONE, 71, {71.0f, 69.0f, 74.0f}});
        }
    }

private:
    IProximitySensor::EventCallback eventCallback_;
    IProximitySensor::ErrorCallback errorCallback_;
};

template <typename SensorType>
void runControllerScenario(SensorType& sensor) {
    PumpSpy pump;
    FlowMeterSpy flow;
    FillingController controller(sensor, pump, flow, 0, 100.0);

    require(controller.getState() == SystemState::WAITING,
            "Controller should start in WAITING");

    sensor.emitTriggered();

    controller.tick();
    require(controller.getState() == SystemState::CONFIRMATION,
            "Controller should move to CONFIRMATION when cup is detected");

    controller.tick();
    require(controller.getState() == SystemState::FILLING,
            "Controller should move to FILLING after hold timer");
    require(pump.turnOnCalls() == 1, "Pump should be turned on exactly once");

    flow.setVolumeML(100.0);
    controller.tick();
    require(controller.getState() == SystemState::FILL_COMPLETE,
            "Controller should move to FILL_COMPLETE when target volume is reached");
    require(pump.turnOffCalls() == 1, "Pump should be turned off exactly once");

    controller.tick();
    require(controller.getState() == SystemState::WAITING,
            "Controller should reset back to WAITING");
    require(controller.getBottleCount() == 1,
            "Bottle counter should increment after a completed fill");
}

void stressLifecycleSubstitution() {
    constexpr std::size_t kRounds = 2000;

    ScriptedLifecycleDevice dev1(std::vector<bool>{true});
    ScriptedLifecycleDevice dev2(std::vector<bool>{true, false, true});
    ScriptedLifecycleDevice dev3(std::vector<bool>{false, true});

    std::vector<ScriptedLifecycleDevice*> ownedDevices = {&dev1, &dev2, &dev3};

    std::vector<IHardwareDevice*> devices;
    devices.reserve(ownedDevices.size());
    for (auto* device : ownedDevices) {
        devices.push_back(device);
    }

    for (std::size_t round = 0; round < kRounds; ++round) {
        for (IHardwareDevice* device : devices) {
            (void)device->init();
        }

        for (auto it = devices.rbegin(); it != devices.rend(); ++it) {
            (*it)->shutdown();
            (*it)->shutdown();
        }
    }

    for (const auto* device : ownedDevices) {
        require(device->initCalls() == kRounds,
                "Each derived device must be initialised through the base exactly once per round");
        require(device->shutdownCalls() == (kRounds * 2),
                "Each derived device must tolerate repeated shutdown via base pointers");
    }
}

void stressCallbackSubstitution() {
    constexpr std::size_t kRounds = 1000;

    SingleChannelSensor singleSensor;
    MultiChannelSensor multiSensor;

    CallbackStats singleStats;
    CallbackStats multiStats;

    IProximitySensor* singleAsBase = &singleSensor;
    IProximitySensor* multiAsBase = &multiSensor;

    singleAsBase->registerEventCallback([&singleStats](const GestureEvent& event) {
        singleStats.incTotal();
        if (event.getProximityChannels().size() > 1) {
            singleStats.incMultiChannel();
        }
        singleStats.setLastPrimary(primaryValue(event));
    });

    multiAsBase->registerEventCallback([&multiStats](const GestureEvent& event) {
        multiStats.incTotal();
        if (event.getProximityChannels().size() > 1) {
            multiStats.incMultiChannel();
        }
        multiStats.setLastPrimary(primaryValue(event));
    });

    std::vector<IHardwareDevice*> lifecycleDevices = {&singleSensor, &multiSensor};

    for (std::size_t round = 0; round < kRounds; ++round) {
        for (IHardwareDevice* device : lifecycleDevices) {
            require(device->init(), "Sensor init should remain substitutable");
        }
        for (IHardwareDevice* device : lifecycleDevices) {
            device->shutdown();
        }
    }

    require(singleStats.getTotalEvents() == (kRounds * 2),
            "Single-channel sensor should emit two events per init cycle");
    require(multiStats.getTotalEvents() == (kRounds * 3),
            "Multi-channel sensor should emit three events per init cycle");
    require(multiStats.getMultiChannelEvents() == multiStats.getTotalEvents(),
            "Multi-channel payload should remain visible through the base callback");
    require(singleStats.getLastPrimary() > 0.0f && multiStats.getLastPrimary() > 0.0f,
            "Primary proximity extraction should work for all substituted sensors");
}

void stressControllerSubstitution() {
    ManualSensorSingle singleSensor;
    ManualSensorMulti multiSensor;

    runControllerScenario(singleSensor);
    runControllerScenario(multiSensor);
}

} // namespace

int main() {
    try {
        stressLifecycleSubstitution();
        stressCallbackSubstitution();
        stressControllerSubstitution();
        std::cout << "[PASS] Liskov substitution stress test completed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "[FAIL] Liskov substitution stress test: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
