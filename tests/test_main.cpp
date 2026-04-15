#include "UltrasonicSensor.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Starting Ultrasonic Sensor Test..." << std::endl;

    try {
        // Change chip number, Trigger pin, and Echo pin here!
        // Example: chip 4 (Pi 5), Trig=23, Echo=24
        UltrasonicSensor ultra(4, 23, 24);
        
        // Every time the sensor gets a new reading, it runs this code:
        ultra.registerDistanceCallback([](float dist) {
            std::cout << "Object detected at: " << dist << " cm" << std::endl;
        });
        
        // If the sensor breaks or times out, it runs this code:
        ultra.registerErrorCallback([](const std::string& err) {
            std::cerr << "Sensor Error: " << err << std::endl;
        });

        std::cout << "Sensor initializing..." << std::endl;
        
        // It might be 'init' or 'start' depending on the exact version you have.
        // If .init() gives an error, use .start() instead!
        ultra.init();  
        
        std::cout << "Measuring... Wave your hand in front of the sensor!" << std::endl;
        
        // Keep the program alive forever
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Setup Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
