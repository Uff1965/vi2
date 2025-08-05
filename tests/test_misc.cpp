#include "test.h"

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

TEST_F(ViTimingJournalFixture, Journal)
{
    // Add a measurement to check reset
    VI_TM_HMEAS meas = vi_tmMeasurement(journal(), "test_entry");
    ASSERT_NE(meas, nullptr) << "vi_tmMeasurement should return a valid descriptor";

    // Add a measurement
    vi_tmMeasurementAdd(meas, 100, 10);
    // Get measurement statistics
    vi_tmMeasurementStats_t stats{};
    vi_tmMeasurementGet(meas, nullptr, &stats);
    EXPECT_EQ(stats.calls_, 1) << "Number of calls should be 1";
	EXPECT_EQ(stats.cnt_, 10) << "Count of measured events should be 10";
    EXPECT_EQ(stats.sum_, 100) << "Total time should be 100";

    // Reset the journal
    vi_tmJournalReset(journal());

    // After reset, the measurement handle should remain valid
    vi_tmMeasurementGet(meas, nullptr, &stats);
    // Check that the number of calls is reset
    EXPECT_EQ(stats.calls_, 0) << "After journal reset, statistics should be reset";
	EXPECT_EQ(stats.cnt_, 0) << "Count of measured events should be 0";
    EXPECT_EQ(stats.sum_, 0) << "Total time should be 0";
}
