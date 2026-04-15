#include <iostream>      // Console output
#include "FlowMeter.h"   // FlowSensor class

int main() {

    try {

        // Create flow sensor on chip 4, GPIO23, calibration = 98 pulses/L/min
        // Change chip number if needed:
        //   Pi 5 often uses gpiochip4, older Pi often gpiochip0
        FlowSensor flowSensor(4, 23, 98.0f);

        // Optional: callback on every pulse (useful for debugging)
        flowSensor.registerPulseCallback([]() {
            // Runs each time the sensor detects a pulse
        });

        // Callback for flow rate — prints L/min every second
        flowSensor.registerFlowCallback([](float flowRate) {
            std::cout << "Flow Rate: " << flowRate << " L/min" << std::endl;
        });

        // Start measuring (launches background threads)
        flowSensor.start();

        std::cout << "Flow sensor started. Press Enter to stop..." << std::endl;

        // Keep running until user presses Enter
        std::cin.get();

        // Clean shutdown
        flowSensor.stop();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
