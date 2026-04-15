#include <gpiod.hpp>
#include <memory>
#include <string>
#include <functional> // for the std::function


class WaterPump{
    public:

        //constructor
        WaterPump(const std::string& chip_path, unsigned int pin_offset);

        //destructor to handle the close logic
        ~WaterPump();

        // we delete the ability to copy the pump 
        WaterPump(const WaterPump&) = delete;
        WaterPump& operator=(const WaterPump&) = delete;

        void turn_on() const;
        void turn_off() const;
        void set_state_change_callback(std::function<void(bool)> callback);  //so we are subscribing for state changes

    private:
        std::shared_ptr<gpiod::chip> chip;
        std::shared_ptr<gpiod::line_request> line_request;
        unsigned int pin;

        std::function<void(bool)> state_change_callback;


};