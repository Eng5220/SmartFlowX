#ifndef IPUMP_H
#define IPUMP_H

class IPump {
public:
    virtual ~IPump() = default;
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
    virtual bool isRunning() const = 0;
};

#endif // IPUMP_H
