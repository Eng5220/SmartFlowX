#include <gtest/gtest.h>

#include "hardware/FlowMeter.h"

TEST(FlowMeterTest, StartsWithZeroVolume) {
    FlowMeter fm(4, 18, 1.0f);
    EXPECT_EQ(fm.getPulseCount(), 0);
    EXPECT_DOUBLE_EQ(fm.getVolumeML(), 0.0);
}

TEST(FlowMeterTest, CountsPulsesAndCalculatesVolume) {
    FlowMeter fm(4, 19, 2.5f);  // 2.5 ml per pulse

    fm.injectPulseCountForTest(4);

    EXPECT_EQ(fm.getPulseCount(), 4);
    EXPECT_DOUBLE_EQ(fm.getVolumeML(), 10.0);
}

TEST(FlowMeterTest, ResetClearsCount) {
    FlowMeter fm(4, 20, 1.0f);

    fm.injectPulseCountForTest(10);
    EXPECT_EQ(fm.getPulseCount(), 10);

    fm.resetCount();
    EXPECT_EQ(fm.getPulseCount(), 0);
    EXPECT_DOUBLE_EQ(fm.getVolumeML(), 0.0);
}

TEST(FlowMeterTest, DetectsReachedTarget) {
    FlowMeter fm(4, 21, 5.0f);  // 5 ml per pulse

    fm.injectPulseCountForTest(2);  // 10 ml
    EXPECT_FALSE(fm.hasReachedTarget(15.0));

    fm.injectPulseCountForTest(3);  // 15 ml
    EXPECT_TRUE(fm.hasReachedTarget(15.0));
}
