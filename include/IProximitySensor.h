#ifndef IPROXIMITYSENSOR_H
#define IPROXIMITYSENSOR_H

#include <array>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

enum class ProximityState {
    NONE,
    PROXIMITY_TRIGGERED,
    PROXIMITY_CLEARED
};

enum class GestureDir {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class InlineChannels {
private:
    std::array<float, 4> data_ = {0.0f, 0.0f, 0.0f, 0.0f};
    std::size_t count_ = 0;
public:
    InlineChannels() = default;
    InlineChannels(std::initializer_list<float> init) {
        for (auto it = init.begin(); it != init.end() && count_ < 4; ++it) {
            data_[count_++] = *it;
        }
    }
    bool empty() const { return count_ == 0; }
    std::size_t size() const { return count_; }
    float front() const { return count_ > 0 ? data_[0] : 0.0f; }
};

struct GestureEvent {
private:
    ProximityState state = ProximityState::NONE;
    GestureDir direction = GestureDir::NONE;

    // Backward-compatible primary reading.
    int proximityValue = 0;

    // Future-proof channel payload, heavily optimised to avert std::vector heap allocations
    // on high-frequency callback ticks (O(1) memory, bounded at 4 channels).
    InlineChannels proximityChannels{};

public:
    GestureEvent() = default;
    
    GestureEvent(ProximityState s, GestureDir d, int pVal = 0, InlineChannels channels = {})
        : state(s), direction(d), proximityValue(pVal), proximityChannels(channels) {}

    ProximityState getState() const { return state; }
    GestureDir getDirection() const { return direction; }
    int getProximityValue() const { return proximityValue; }
    const InlineChannels& getProximityChannels() const { return proximityChannels; }
};

class IProximitySensor {
public:
    // LSP note:
    // The callback signature remains stable while GestureEvent can evolve by
    // adding optional fields (such as proximityChannels), allowing richer
    // derived sensor implementations without changing base consumers.
    using EventCallback = std::function<void(const GestureEvent&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    virtual ~IProximitySensor() = default;
    virtual void registerEventCallback(EventCallback cb) = 0;
    virtual void registerErrorCallback(ErrorCallback cb) = 0;
};

#endif // IPROXIMITYSENSOR_H
