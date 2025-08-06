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

TEST(misc, vi_tmStaticInfo)
{
    const auto flags = *static_cast<const unsigned*>(vi_tmStaticInfo(VI_TM_INFO_FLAGS));

    {
#if VI_TM_DEBUG
        constexpr auto flag = vi_tmDebug;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmDebug) << "The build type flag does not match.";
    }

    {
#if VI_TM_SHARED
        constexpr auto flag = vi_tmShared;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmShared) << "The library type flag does not match.";
    }

    {
#if VI_TM_THREADSAFE
        constexpr auto flag = vi_tmThreadsafe;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmThreadsafe) << "The thread safe flag does not match.";
    }

    {
#if VI_TM_STAT_USE_BASE
        constexpr auto flag = vi_tmStatUseBase;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmStatUseBase) << "The use base flag does not match.";
    }

    {
#if VI_TM_STAT_USE_FILTER
        constexpr auto flag = vi_tmStatUseFilter;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmStatUseFilter) << "The use filter flag does not match.";
    }

    {
#if VI_TM_STAT_USE_MINMAX
        constexpr auto flag = vi_tmStatUseMinMax;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmStatUseMinMax) << "The use minmax flag does not match.";
    }
}
