// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "test.h"

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <cassert>
#include <cerrno>

TEST_F(ViTimingJournalFixture, measurement)
{   const char name[] = "test_entry";
	vi_tmMeasurementStats_t stats{};
	const char *pname = nullptr;

	const auto meas = vi_tmJournalGetMeas(journal(), name);
    ASSERT_NE(meas, nullptr) << "vi_tmJournalGetMeas should return a valid descriptor";

	vi_tmMeasurementGet(meas, &pname, &stats);
	EXPECT_STREQ(pname, name);
	EXPECT_EQ(stats.calls_, 0) << "New measurement should have zero calls";
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, 0) << "Count of measured events should be 0";
    EXPECT_EQ(stats.sum_, 0) << "Total time should be 0";
#endif

	const auto tmp = vi_tmJournalGetMeas(journal(), name);
	EXPECT_EQ(meas, tmp) << "The probe address must not change while the journal exists.";
}

TEST_F(ViTimingJournalFixture, Journal)
{   const char name[] = "test_entry";
	constexpr VI_TM_SIZE CNT = 10;
    constexpr VI_TM_SIZE AMT = 100;
    constexpr VI_TM_TDIFF DUR = 1000;

    // Add a measurement to check reset
    VI_TM_HMEAS meas = vi_tmJournalGetMeas(journal(), name);
    ASSERT_NE(meas, nullptr) << "vi_tmJournalGetMeas should return a valid descriptor";

    // Add a CNT measurement
    for (unsigned n = 0; n < CNT; ++n)
    {   vi_tmMeasurementAdd(meas, DUR, AMT);
    }

    // Get measurement statistics
    vi_tmMeasurementStats_t stats{};
    vi_tmMeasurementGet(meas, nullptr, &stats);
    EXPECT_EQ(stats.calls_, CNT) << "Number of calls should be " << CNT;
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, CNT * AMT) << "Count of measured events should be " << CNT * AMT;
    EXPECT_EQ(stats.sum_, CNT * DUR) << "Total time should be " << CNT * DUR;
#endif

    // Reset the journal
    vi_tmJournalReset(journal());

    // After reset, the measurement handle should remain valid
    vi_tmMeasurementGet(meas, nullptr, &stats);
    // Check that the number of calls is reset
    EXPECT_EQ(stats.calls_, 0) << "After journal reset, statistics should be reset";
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, 0) << "Count of measured events should be 0";
    EXPECT_EQ(stats.sum_, 0) << "Total time should be 0";
#endif

	const auto tmp = vi_tmJournalGetMeas(journal(), name);
	EXPECT_EQ(meas, tmp);
}

TEST(misc, vi_tmStaticInfo)
{
    const auto flags = *static_cast<const unsigned*>(vi_tmStaticInfo(vi_tmInfoFlags));

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
#if VI_TM_STAT_USE_RAW
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
#if VI_TM_STAT_USE_RMSE
        constexpr auto flag = vi_tmStatUseRMSE;
#else
		constexpr auto flag = 0U;
#endif
        EXPECT_EQ(flag, flags & vi_tmStatUseRMSE) << "The use RMSE flag does not match.";
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
