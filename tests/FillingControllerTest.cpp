#include <gtest/gtest.h>

#include "hardware/FlowMeter.h"
#include "hardware/GestureSensor.h"
#include "hardware/PumpController.h"
#include "state/FillingController.h"

class FillingControllerTest : public ::testing::Test {
protected:
    GestureSensor gs;
    PumpController pump{4, 27};
    FlowMeter flow{4, 17, 1.0f};
    FillingController fc{gs, pump, flow, 2, 50.0};

    FillingControllerTest() {
        pump.enableSimulationForTest();
    }

    void simulateProximity(ProximityState state) {
        gs.emitEventForTest({state, GestureDir::NONE, 200});
    }
};

TEST_F(FillingControllerTest, InitialStateIsWaiting) {
    EXPECT_EQ(fc.getState(), SystemState::WAITING);
    EXPECT_EQ(fc.getBottleCount(), 0);
}

TEST_F(FillingControllerTest, TransitionToConfirmationOnProximity) {
    EXPECT_EQ(fc.getState(), SystemState::WAITING);

    simulateProximity(ProximityState::PROXIMITY_TRIGGERED);
    fc.tick();

    EXPECT_EQ(fc.getState(), SystemState::CONFIRMATION);
}

TEST_F(FillingControllerTest, ReturnsToWaitingIfProximityClearedPrematurely) {
    simulateProximity(ProximityState::PROXIMITY_TRIGGERED);
    fc.tick();
    EXPECT_EQ(fc.getState(), SystemState::CONFIRMATION);

    simulateProximity(ProximityState::PROXIMITY_CLEARED);
    fc.tick();

    EXPECT_EQ(fc.getState(), SystemState::WAITING);
}

TEST_F(FillingControllerTest, CompletesFullCycleSuccessfully) {
    simulateProximity(ProximityState::PROXIMITY_TRIGGERED);
    fc.tick();
    EXPECT_EQ(fc.getState(), SystemState::CONFIRMATION);

    fc.forceHoldElapsedForTest(2.1);

    fc.tick();
    EXPECT_EQ(fc.getState(), SystemState::FILLING);
    EXPECT_TRUE(pump.isRunning());

    flow.injectPulseCountForTest(50);

    fc.tick();
    EXPECT_EQ(fc.getState(), SystemState::FILL_COMPLETE);
    EXPECT_FALSE(pump.isRunning());
    EXPECT_EQ(fc.getBottleCount(), 1);

    fc.tick();
    EXPECT_EQ(fc.getState(), SystemState::WAITING);
}
