#ifndef IFLOWMETER_H
#define IFLOWMETER_H

class IFlowMeter {
public:
    virtual ~IFlowMeter() = default;
    virtual void resetCount() = 0;
    virtual int getPulseCount() const = 0;
    virtual double getVolumeML() const = 0;
};

#endif // IFLOWMETER_H
