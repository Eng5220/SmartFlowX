#include <gtest/gtest.h>

#include "hardware/PumpController.h"

TEST(PumpControllerTest, InitialStateIsOff) {
    PumpController pump(4, 27);
    EXPECT_FALSE(pump.isRunning());
}

TEST(PumpControllerTest, TurnOnAndOff) {
    PumpController pump(4, 27, PumpController::DriveMode::TRANSISTOR_ACTIVE_HIGH);
    pump.enableSimulationForTest();

    pump.turnOn();
    EXPECT_TRUE(pump.isRunning());

    pump.turnOff();
    EXPECT_FALSE(pump.isRunning());

    pump.shutdown();
}

TEST(PumpControllerTest, RelayModeActiveLow) {
    PumpController pump(4, 27, PumpController::DriveMode::RELAY_ACTIVE_LOW);
    pump.enableSimulationForTest();

    pump.turnOn();
    EXPECT_TRUE(pump.isRunning());

    pump.turnOff();
    EXPECT_FALSE(pump.isRunning());

    pump.shutdown();
}
