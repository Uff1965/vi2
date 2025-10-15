// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "test.h"

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>

namespace
{
	using unique_journal_t = std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)>;
	
	unique_journal_t make_journal()
	{	unique_journal_t result{vi_tmJournalCreate(), vi_tmJournalClose};
		return result;
	}

	using fp_limits_t = std::numeric_limits<VI_TM_FP>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr auto fp_ONE = static_cast<VI_TM_FP>(1);
}

TEST(OutlierFiltering, BasicFiltering)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	auto const meas = vi_tmJournalGetMeas(journal.get(), "basic_filter_test");
	
	// Add normal values (around 1000 ticks)
	static constexpr VI_TM_TDIFF normal_values[] = {
		1000U, 1010U, 990U, 1005U, 995U,
		1002U, 998U, 1008U, 992U, 1001U
	};
	for (auto val : normal_values)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	vi_tmStats_t stats_before;
	vi_tmMeasurementGet(meas, nullptr, &stats_before);
	EXPECT_EQ(stats_before.calls_, std::size(normal_values));
	EXPECT_DOUBLE_EQ(stats_before.flt_avg_,  std::accumulate(std::begin(normal_values), std::end(normal_values), fp_ZERO) / std::size(normal_values));
	
	// Add obvious outliers (10 times larger than normal values)
	static constexpr VI_TM_TDIFF outliers[] = { 5000U, 10000U, 8000U };
	for (auto val : outliers)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	vi_tmStats_t stats_after;
	vi_tmMeasurementGet(meas, nullptr, &stats_after);
	// Check that outliers are filtered
	EXPECT_EQ(stats_after.calls_, stats_before.calls_ + std::size(outliers));
	EXPECT_EQ(stats_after.flt_calls_, stats_before.flt_calls_);
	
	// Check that the filtered average is close to the original
	EXPECT_DOUBLE_EQ(stats_after.flt_avg_,  stats_before.flt_avg_);
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}

TEST(OutlierFiltering, SigmaClippingThreshold)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	auto meas = vi_tmJournalGetMeas(journal.get(), "sigma_clipping_test");
	
	// Create a dataset with known standard deviation
	static constexpr VI_TM_TDIFF base_values[] = {
		1000U, 1000U, 1000U, 1000U, 1000U,
		1000U, 1000U, 1000U, 1000U, 1000U
	};
	
	for (auto val : base_values)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	vi_tmStats_t stats_before;
	vi_tmMeasurementGet(meas, nullptr, &stats_before);
	
	// Add values that should be filtered (more than 2.5 sigma)
	// With zero standard deviation, any deviation from the mean is an outlier
	static constexpr VI_TM_TDIFF strong_outliers[] = {2000U, 3000U, 4000U};
	
	for (auto val : strong_outliers)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	vi_tmStats_t stats_after;
	vi_tmMeasurementGet(meas, nullptr, &stats_after);
	// Check that strong outliers are filtered
	EXPECT_LT(stats_after.flt_calls_, stats_after.calls_);
	// Check that the filtered average remains close to the original
	EXPECT_NEAR(stats_after.flt_avg_, stats_before.flt_avg_, 100.0); // 100 ticks tolerance
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}

TEST(OutlierFiltering, MinimumValuePreference)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	const auto meas = vi_tmJournalGetMeas(journal.get(), "min_value_preference_test");
	
	// Add several normal values
	static constexpr VI_TM_TDIFF normal_values[] = {1000U, 1005U, 995U, 1002U, 998U};
	for (auto val : normal_values)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	// Add a value that is higher than the average but not a strong outlier
	vi_tmMeasurementAdd(meas, 1200U, 1U);
	
	vi_tmStats_t stats_after;
	vi_tmMeasurementGet(meas, nullptr, &stats_after);
	
	// Check that the value 1200 can be filtered
	// due to minimum value preference
	EXPECT_LE(stats_after.flt_calls_, stats_after.calls_);
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}

TEST(OutlierFiltering, InsufficientDataProtection)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	const auto meas = vi_tmJournalGetMeas(journal.get(), "insufficient_data_test");
	
	// Add only one value
	vi_tmMeasurementAdd(meas, 1'000U, 1U);
	
	// Add a second value - it should not be filtered
	vi_tmMeasurementAdd(meas, 20'000U, 1U);
	
	vi_tmStats_t stats;
	vi_tmMeasurementGet(meas, nullptr, &stats);
	
	// With insufficient data, filtering should not work
	EXPECT_EQ(stats.flt_calls_, stats.calls_);
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}

TEST(OutlierFiltering, ZeroInitialMeasurements)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	auto meas = vi_tmJournalGetMeas(journal.get(), "zero_initial_test");
	
	// Add two zero measurements
	vi_tmMeasurementAdd(meas, 0U, 1U);
	vi_tmMeasurementAdd(meas, 0U, 1U);
	
	vi_tmStats_t stats_zero;
	vi_tmMeasurementGet(meas, nullptr, &stats_zero);
	
	// Add a normal value - it should be accepted
	vi_tmMeasurementAdd(meas, 1000U, 1U);
	
	vi_tmStats_t stats_normal;
	vi_tmMeasurementGet(meas, nullptr, &stats_normal);
	
	// Check that the normal value was accepted
	EXPECT_EQ(stats_normal.flt_calls_, 3U);
	EXPECT_GT(stats_normal.flt_avg_, 0.0);
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}

//TEST(OutlierFiltering, ResolutionThreshold)
//{
//#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
//	auto journal = make_journal();
//	const auto meas = vi_tmJournalGetMeas(journal.get(), "resolution_threshold_test");
//	
//	// Add several normal values
//	static constexpr VI_TM_TDIFF normal_values[] = {0U, 1U, 0U, 0U, 1U};
//	
//	for (auto val : normal_values)
//	{	vi_tmMeasurementAdd(meas, val, 1U);
//	}
//	
//	// Add a value that is less than the clock resolution (1 tick)
//	vi_tmMeasurementAdd(meas, 1U, 1U);
//	
//	vi_tmStats_t stats_after;
//	vi_tmMeasurementGet(meas, nullptr, &stats_after);
//	
//	// The value 1 tick should be filtered as less than the resolution
//	EXPECT_EQ(stats_after.flt_calls_, stats_after.calls_);
//#else
//	GTEST_SKIP() << "Filtering is not enabled in this build";
//#endif
//}

TEST(OutlierFiltering, GradualOutlierIntroduction)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	auto meas = vi_tmJournalGetMeas(journal.get(), "gradual_outlier_test");
	
	// Create a baseline with normal values
	static constexpr VI_TM_TDIFF base_values[] = {
		1000U, 1000U, 1000U, 1000U, 1000U,
		1000U, 1000U, 1000U, 1000U, 1000U
	};
	for (auto val : base_values)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	vi_tmStats_t stats_before;
	vi_tmMeasurementGet(meas, nullptr, &stats_before);
	
	// Gradually introduce increasingly strong outliers
	static constexpr VI_TM_TDIFF gradual_outliers[] = {1100U, 1200U, 1500U, 2000U, 5000U};
	for (auto val : gradual_outliers)
	{	vi_tmMeasurementAdd(meas, val, 1U);
		
		vi_tmStats_t stats_current;
		vi_tmMeasurementGet(meas, nullptr, &stats_current);
		// Check that filtering works gradually
		EXPECT_LE(stats_current.flt_calls_, stats_current.calls_);
	}
	
	vi_tmStats_t stats_final;
	vi_tmMeasurementGet(meas, nullptr, &stats_final);
	
	// Final check: strong outliers should be filtered
	EXPECT_LT(stats_final.flt_calls_, stats_final.calls_);
	EXPECT_NEAR(stats_final.flt_avg_, 1000.0, 100.0); // 100 ticks tolerance
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}

TEST(OutlierFiltering, FilteringDisabled)
{
#if !VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	auto meas = vi_tmJournalGetMeas(journal.get(), "no_filter_test");
	
	// Add normal values
	static constexpr VI_TM_TDIFF normal_values[] = {1000U, 1005U, 995U, 1002U, 998U};
	for (auto val : normal_values)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	// Add obvious outliers
	static constexpr VI_TM_TDIFF outliers[] = {5000U, 10000U};
	for (auto val : outliers)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	vi_tmStats_t stats_after;
	vi_tmMeasurementGet(meas, nullptr, &stats_after);
	
	// When filtering is disabled, all values should be accepted
#if VI_TM_STAT_USE_RMSE
	EXPECT_EQ(stats_after.flt_calls_, stats_after.calls_);
#endif
#else
	GTEST_SKIP() << "Filtering is enabled in this build";
#endif
}

TEST(OutlierFiltering, BatchMeasurementsWithOutliers)
{
#if VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
	auto journal = make_journal();
	auto meas = vi_tmJournalGetMeas(journal.get(), "batch_outlier_test");
	
	// Add normal values one by one
	static constexpr VI_TM_TDIFF normal_values[] = {1000U, 1005U, 995U, 1002U, 998U};
	for (auto val : normal_values)
	{	vi_tmMeasurementAdd(meas, val, 1U);
	}
	
	// Add a batch measurement with an outlier
	vi_tmMeasurementAdd(meas, 5000U, 5U); // 5 events of 1000 ticks each
	
	vi_tmStats_t stats_after;
	vi_tmMeasurementGet(meas, nullptr, &stats_after);
	
	// Check that the batch measurement with an outlier is handled correctly
	EXPECT_EQ(stats_after.calls_, std::size(normal_values) + 1U);
#	if VI_TM_STAT_USE_RAW
	EXPECT_EQ(stats_after.cnt_, std::size(normal_values) + 5U);
#	endif
	// Check filtering
	EXPECT_LE(stats_after.flt_calls_, stats_after.calls_);
#else
	GTEST_SKIP() << "Filtering is not enabled in this build";
#endif
}
