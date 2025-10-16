// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "test.h"

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <cassert>
#include <limits>

namespace
{	using fp_limits_t = std::numeric_limits<VI_TM_FP>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr auto fp_ONE = static_cast<VI_TM_FP>(1);

	constexpr VI_TM_TDIFF DURATIONS[] = { 1000U, 1500U, 2000U, 2500U }; // Example durations in ticks. No filtering expected.
	constexpr VI_TM_TDIFF DURATION = DURATIONS[0];
	constexpr VI_TM_SIZE ZERO_COUNT = 0U;
	constexpr VI_TM_SIZE COUNT = 1U;
	constexpr VI_TM_SIZE BATCH_SIZE = 10U;
}

TEST(StatsValidation, EmptyStats)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	// Empty statistics should be valid
	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.calls_, 0U);
	EXPECT_EQ(stats.cnt_, 0U);
	EXPECT_EQ(stats.sum_, 0U);
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_EQ(stats.min_, VI_TM_FP_POSITIVE_INF);
	EXPECT_EQ(stats.max_, VI_TM_FP_NEGATIVE_INF);
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, 0U);
	EXPECT_EQ(stats.flt_cnt_, fp_ZERO);
	EXPECT_EQ(stats.flt_avg_, fp_ZERO);
	EXPECT_EQ(stats.flt_ss_, fp_ZERO);
#endif
}

TEST(StatsValidation, SingleMeasurement)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	vi_tmStatsAdd(&stats, DURATION, COUNT);
	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	EXPECT_EQ(stats.calls_, 1U);
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, 1U);
	EXPECT_EQ(stats.sum_, DURATION);
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_EQ(stats.min_, VI_TM_FP(DURATION));
	EXPECT_EQ(stats.max_, VI_TM_FP(DURATION));
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, 1U);
	EXPECT_EQ(stats.flt_cnt_, fp_ONE);
	EXPECT_EQ(stats.flt_avg_, VI_TM_FP(DURATION));
	EXPECT_EQ(stats.flt_ss_, fp_ZERO); // For a single measurement, SS should be 0
#endif
}

TEST(StatsValidation, MultipleMeasurements)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	for (auto duration : DURATIONS)
	{	vi_tmStatsAdd(&stats, duration, COUNT);
		EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	}
	
	EXPECT_EQ(stats.calls_, std::size(DURATIONS));
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, std::size(DURATIONS));
	EXPECT_EQ(stats.sum_, std::accumulate(std::begin(DURATIONS), std::end(DURATIONS), VI_TM_TDIFF(0U)));
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_DOUBLE_EQ(stats.min_, VI_TM_FP(*std::min_element(std::begin(DURATIONS), std::end(DURATIONS))));
	EXPECT_DOUBLE_EQ(stats.max_, VI_TM_FP(*std::max_element(std::begin(DURATIONS), std::end(DURATIONS))));
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, std::size(DURATIONS));
	EXPECT_EQ(stats.flt_cnt_, VI_TM_FP(std::size(DURATIONS)));
#	if VI_TM_STAT_USE_RAW
	EXPECT_DOUBLE_EQ(stats.flt_avg_, VI_TM_FP(stats.sum_) / stats.cnt_);
#	endif
	EXPECT_GT(stats.flt_ss_, fp_ZERO); // For multiple unequal measurements, SS should be > 0
#endif
}

TEST(StatsValidation, BatchMeasurements)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	vi_tmStatsAdd(&stats, DURATION * BATCH_SIZE, BATCH_SIZE);

	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	EXPECT_EQ(stats.calls_, 1U);
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, BATCH_SIZE);
	EXPECT_EQ(stats.sum_, DURATION * BATCH_SIZE);
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_EQ(stats.min_, VI_TM_FP(DURATION));
	EXPECT_EQ(stats.max_, VI_TM_FP(DURATION));
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, 1U);
	EXPECT_EQ(stats.flt_cnt_, VI_TM_FP(BATCH_SIZE));
	EXPECT_EQ(stats.flt_avg_, VI_TM_FP(DURATION));
	EXPECT_EQ(stats.flt_ss_, fp_ZERO);
#endif
}

TEST(StatsValidation, ZeroCountIgnored)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	// Adding a measurement with zero count - should be ignored
	vi_tmStatsAdd(&stats, DURATION, ZERO_COUNT);

	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	EXPECT_EQ(stats.calls_, 0U); // Statistics should remain empty
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, 0U);
	EXPECT_EQ(stats.sum_, 0U);
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_EQ(stats.min_, VI_TM_FP_POSITIVE_INF);
	EXPECT_EQ(stats.max_, VI_TM_FP_NEGATIVE_INF);
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, 0U);
	EXPECT_EQ(stats.flt_cnt_, fp_ZERO);
	EXPECT_EQ(stats.flt_avg_, fp_ZERO);
	EXPECT_EQ(stats.flt_ss_, fp_ZERO);
#endif
}

TEST(StatsValidation, LargeValues)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	// Test with very large values
	constexpr VI_TM_TDIFF LARGE_DURATION = UINT64_MAX / 2;
	
	vi_tmStatsAdd(&stats, LARGE_DURATION, COUNT);
	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	EXPECT_EQ(stats.calls_, 1U);
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, COUNT);
	EXPECT_EQ(stats.sum_, LARGE_DURATION);
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_EQ(stats.min_, VI_TM_FP(LARGE_DURATION));
	EXPECT_EQ(stats.max_, VI_TM_FP(LARGE_DURATION));
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, 1U);
	EXPECT_EQ(stats.flt_cnt_, fp_ONE);
	EXPECT_EQ(stats.flt_avg_, VI_TM_FP(LARGE_DURATION));
	EXPECT_EQ(stats.flt_ss_, fp_ZERO);
#endif
}

TEST(StatsValidation, MergeOperations)
{	vi_tmStats_t stats1, stats2, merged;
	vi_tmStatsReset(&stats1);
	vi_tmStatsReset(&stats2);
	vi_tmStatsReset(&merged);
	
	// Add data to the first statistics
	vi_tmStatsAdd(&stats1, DURATIONS[0], 1U);
	vi_tmStatsAdd(&stats1, DURATIONS[1], 1U);
	
	// Add data to the second statistics
	vi_tmStatsAdd(&stats2, DURATIONS[2], 1U);
	vi_tmStatsAdd(&stats2, DURATIONS[3], 1U);
	
	// Check validity of the original statistics
	EXPECT_EQ(vi_tmStatsIsValid(&stats1), 0);
	EXPECT_EQ(vi_tmStatsIsValid(&stats2), 0);
	
	// Copy the first statistics to merged
	merged = stats1;
	
	// Merge with the second statistics
	vi_tmStatsMerge(&merged, &stats2);
	EXPECT_EQ(vi_tmStatsIsValid(&merged), 0);
	EXPECT_EQ(merged.calls_, 4U);
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(merged.cnt_, 4U);
	EXPECT_EQ(merged.sum_, std::accumulate(std::begin(DURATIONS), std::end(DURATIONS), VI_TM_TDIFF(0U)));
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_DOUBLE_EQ(merged.min_, VI_TM_FP(*std::min_element(std::begin(DURATIONS), std::end(DURATIONS))));
	EXPECT_DOUBLE_EQ(merged.max_, VI_TM_FP(*std::max_element(std::begin(DURATIONS), std::end(DURATIONS))));
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(merged.flt_calls_, 4U);
	EXPECT_EQ(merged.flt_cnt_, 4.0);
#	if VI_TM_STAT_USE_RAW
	EXPECT_DOUBLE_EQ(merged.flt_avg_, VI_TM_FP(merged.sum_) / merged.cnt_);
#	endif
	EXPECT_GT(merged.flt_ss_, fp_ZERO);
#endif
}

TEST(StatsValidation, ResetAfterOperations)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	// Add data
	for (auto duration : DURATIONS)
	{	vi_tmStatsAdd(&stats, duration, 1U);
	}

	// Reset statistics
	vi_tmStatsReset(&stats);
	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0);
	
	// Check that statistics returned to the initial state
	EXPECT_EQ(stats.calls_, 0U);
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats.cnt_, 0U);
	EXPECT_EQ(stats.sum_, 0U);
#endif
#if VI_TM_STAT_USE_MINMAX
	EXPECT_EQ(stats.min_, VI_TM_FP_POSITIVE_INF);
	EXPECT_EQ(stats.max_, VI_TM_FP_NEGATIVE_INF);
#endif
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats.flt_calls_, 0U);
	EXPECT_EQ(stats.flt_cnt_, fp_ZERO);
	EXPECT_EQ(stats.flt_avg_, fp_ZERO);
	EXPECT_EQ(stats.flt_ss_, fp_ZERO);
#endif
}

TEST(StatsValidation, InvalidStatsDetection)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);
	
	// Manually create invalid statistics
	stats.calls_ = 1U;
#if VI_TM_STAT_USE_RAW
	stats.cnt_ = 0U; // Invalid: cnt should be >= calls
	stats.sum_ = 1000U;
#endif
#if VI_TM_STAT_USE_MINMAX
	stats.min_ = 1000.0;
	stats.max_ = 500.0; // Invalid: max should be >= min
#endif
#if VI_TM_STAT_USE_RMSE
	stats.flt_calls_ = 1U;
	stats.flt_cnt_ = fp_ZERO; // Invalid: flt_cnt should be > 0 when flt_calls > 0
	stats.flt_avg_ = 1000.0;
	stats.flt_ss_ = fp_ZERO;
#endif
	
	// Check that invalid statistics are detected
#if VI_TM_STAT_USE_RAW || VI_TM_STAT_USE_MINMAX || VI_TM_STAT_USE_RMSE
	EXPECT_NE(vi_tmStatsIsValid(&stats), 0);
#else
	EXPECT_EQ(vi_tmStatsIsValid(&stats), 0); // If no stats are
#endif
}

TEST(StatsValidation, NullPointerHandling)
{	vi_tmStats_t stats;
	vi_tmStatsReset(&stats);

	// Test with nullptr - should return an error
	EXPECT_NE(vi_tmStatsIsValid(nullptr), 0);

	// vi_tmStatsReset with nullptr should safely terminate
#ifdef NDEBUG
	EXPECT_NO_THROW(vi_tmStatsReset(nullptr));
#else
	EXPECT_DEATH(vi_tmStatsReset(nullptr), "");
#endif

	// vi_tmStatsAdd with nullptr should safely terminate
#ifdef NDEBUG
	EXPECT_NO_THROW(vi_tmStatsAdd(nullptr, 1000U, 1U));
#else
	EXPECT_DEATH(vi_tmStatsAdd(nullptr, 1000U, 1U), "");
#endif
	
	// vi_tmStatsMerge with nullptr should safely terminate
#ifdef NDEBUG
	EXPECT_NO_THROW(vi_tmStatsMerge(nullptr, nullptr));
	EXPECT_NO_THROW(vi_tmStatsMerge(nullptr, &stats));
	EXPECT_NO_THROW(vi_tmStatsMerge(&stats, nullptr));
#else
	EXPECT_DEATH(vi_tmStatsMerge(nullptr, nullptr), "");
	EXPECT_DEATH(vi_tmStatsMerge(nullptr, &stats), "");
	EXPECT_DEATH(vi_tmStatsMerge(&stats, nullptr), "");
#endif
}
