#include "app/AquaFlowApp.h"
#include "PinConfig.h"
#include <iomanip>
#include <sstream>
#include <thread>

AquaFlowApp::AquaFlowApp(IProximitySensor& gestureSensor, IPump& pump, IFlowMeter& flowMeter, LcdDisplay& lcd)
    : gestureSensor_(gestureSensor),
      pump_(pump),
      flowMeter_(flowMeter),
      lcd_(lcd),
      controller_(gestureSensor_, pump_, flowMeter_, HOLD_TIME_SECONDS, TARGET_VOLUME_ML),
      loopTimer_(LOOP_INTERVAL_MS)
{
}

void AquaFlowApp::start() {
    // 1. Wire state transitions to the monitor and the display
    controller_.registerMonitor([this](const std::string& state, double vol, int bottles) {
        monitor_.onStateChange(state, vol, bottles);
        lcd_.print(0, state);
        
        std::ostringstream row1;
        if (state == "FILLING") {
            row1 << "Vol: " << std::fixed << std::setprecision(1) << vol << " ml";
        } else {
            row1 << "Bottles: " << bottles;
        }
        lcd_.print(1, row1.str());
    });

    // 2. Wire the high-frequency state machine tick loop
    loopTimer_.registerCallback([this]() {
        controller_.tick();
        
        // Push high-frequency fluid volume update dynamically
        std::ostringstream row;
        row << "Vol: " << std::fixed << std::setprecision(1) << flowMeter_.getVolumeML() << " ml";
        lcd_.print(1, row.str());
    });

    // 3. Commence processing
    loopTimer_.start();
}

void AquaFlowApp::shutdown() {
    loopTimer_.stop();

    // Visual disconnection sequence entirely encapsulated out of main()
    lcd_.clear();
    lcd_.print(0, "AquaFlow");
    lcd_.print(1, "Shutting down...");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // The hardware shutdown (e.g. lcd.shutdown(), pump.shutdown()) is kept 
    // in main() or whoever owns the hardware objects' lifecycle.
}
