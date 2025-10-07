#include <gtest/gtest.h>

#include <vi_timing/vi_timing.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <numeric>
#include <utility>
#include <random>
#include <vector>
#include <cmath>

namespace
{
	using vector = std::vector<VI_TM_TDIFF>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr VI_TM_FP K = 2.5; // Threshold for outliers.

	vector generate(double mean, double stddev, VI_TM_SIZE n)
	{
		//	std::mt19937 gen(std::random_device{}());
		std::mt19937 gen;
		std::normal_distribution dist(mean, stddev);

		vector result;
		result.reserve(n);
		for (VI_TM_SIZE i = 0; i < n; ++i)
		{
			vector::value_type value;
			do
			{
				value = static_cast<vector::value_type>(dist(gen));
			} while (value <= 0);

			result.push_back(value);
		}
		return result;
	}

	void add_wf(vi_tmStats_t &stats, VI_TM_TDIFF diff, VI_TM_SIZE cnt = 1)
	{
		stats.calls_++;
#if VI_TM_STAT_USE_RAW
		stats.cnt_ += cnt;
		stats.sum_ += diff;
#endif
#if VI_TM_STAT_USE_MINMAX || VI_TM_STAT_USE_RMSE
		const double dur = static_cast<double>(diff) / cnt;
#endif
#if VI_TM_STAT_USE_MINMAX
		stats.min_ = std::min(stats.min_, dur);
		stats.max_ = std::max(stats.max_, dur);
#endif
#if VI_TM_STAT_USE_RMSE
		const double deviation = dur - stats.flt_avg_;
#	if VI_TM_STAT_USE_FILTER
		if (diff <= 1U || // The measurable interval is probably smaller than the resolution of the clock.
			deviation < fp_ZERO || // The minimum value is usually closest to the true value. "deviation < .0" - for some reason slowly!!!
			stats.flt_calls_ <= 2U || // If we have less than 2 measurements, we cannot calculate the standard deviation.
			stats.flt_ss_ <= 1.0 || // A pair of zero initial measurements will block the addition of other.
			std::fma(deviation * deviation, stats.flt_cnt_, -K * K * stats.flt_ss_) < fp_ZERO // Sigma clipping to avoids outliers.
			)
#	endif
		{
			stats.flt_cnt_ += cnt;
			stats.flt_avg_ = std::fma(deviation, cnt / stats.flt_cnt_, stats.flt_avg_);
			stats.flt_ss_ = std::fma(deviation * cnt, static_cast<double>(dur) - stats.flt_avg_, stats.flt_ss_);
			++stats.flt_calls_;
		}
#endif
	}

	class ApiTest_t: public ::testing::TestWithParam<int> {};

	TEST_P(ApiTest_t, vi_tmStatsAdd)
	{	const VI_TM_SIZE M = GetParam();

		static const auto arr = generate(100e6, 20e6, 1'000);
		vi_tmStats_t ws;
		vi_tmStats_t stats;
		{	vi_tmStatsReset(&ws);
			vi_tmStatsReset(&stats);
			for (auto &&v : arr)
			{	add_wf(ws, v, M);
				vi_tmStatsAdd(&stats, v, M);
			}
		}

		ASSERT_EQ(ws.calls_, arr.size());
		EXPECT_EQ(stats.calls_, ws.calls_);

#if VI_TM_STAT_USE_RAW
		ASSERT_EQ(ws.cnt_, M * arr.size());
		EXPECT_EQ(stats.cnt_, ws.cnt_);
		ASSERT_EQ(ws.sum_, std::accumulate(arr.cbegin(), arr.cend(), 0.0));
		EXPECT_EQ(stats.sum_, ws.sum_);
#endif

#if VI_TM_STAT_USE_MINMAX
		ASSERT_DOUBLE_EQ(ws.min_, static_cast<double>(*std::min_element(arr.cbegin(), arr.cend())) / M);
		EXPECT_DOUBLE_EQ(stats.min_, ws.min_);
		ASSERT_DOUBLE_EQ(ws.max_, static_cast<double>(*std::max_element(arr.cbegin(), arr.cend())) / M);
		EXPECT_DOUBLE_EQ(stats.max_, ws.max_);
#endif

#if VI_TM_STAT_USE_RMSE
		ASSERT_NE(ws.flt_cnt_, arr.size());
		EXPECT_DOUBLE_EQ(stats.flt_cnt_, ws.flt_cnt_);
		EXPECT_DOUBLE_EQ(stats.flt_avg_, ws.flt_avg_);
		EXPECT_DOUBLE_EQ(stats.flt_ss_, ws.flt_ss_);

#	if VI_TM_STAT_USE_FILTER
		{	const auto flt_calls_old = ws.flt_calls_;
			const auto flt_cnt_old = ws.flt_cnt_;
			const auto flt_avg_old = ws.flt_avg_;
			const auto flt_ss_old = ws.flt_ss_;
			const auto s = sqrt(ws.flt_ss_ / ws.flt_cnt_); // Sample Standard Deviation.
			const auto low = s * (K - 1e-3);
			const auto big = s * (K + 1e-3);

			// Values exceeding the average by more than K standard deviations are filtered out.
			add_wf(ws, static_cast<VI_TM_TDIFF>(ws.flt_avg_ + big) * M, M); // Must be filtered out!
			ASSERT_EQ(ws.flt_calls_, flt_calls_old);
			ASSERT_EQ(ws.flt_cnt_, flt_cnt_old);
			ASSERT_EQ(ws.flt_avg_, flt_avg_old);
			ASSERT_EQ(ws.flt_ss_, flt_ss_old);
			vi_tmStatsAdd(&stats, static_cast<VI_TM_TDIFF>(stats.flt_avg_ + big) * M, M); // Must be filtered out!
			EXPECT_EQ(stats.flt_calls_, ws.flt_calls_);
			EXPECT_EQ(stats.flt_cnt_, ws.flt_cnt_);
			EXPECT_DOUBLE_EQ(stats.flt_avg_, ws.flt_avg_);
			EXPECT_DOUBLE_EQ(stats.flt_ss_, ws.flt_ss_);

			// We do not discard measurements that are below average.
			// Values that differ from the mean by less than K standard deviations are not filtered out.
			add_wf(ws, static_cast<VI_TM_TDIFF>(ws.flt_avg_ + low) * M, M); // Must not be filtered out!
			ASSERT_EQ(ws.flt_calls_, flt_calls_old + 1.0);
			ASSERT_EQ(ws.flt_cnt_, flt_cnt_old + M);
			vi_tmStatsAdd(&stats, static_cast<VI_TM_TDIFF>(stats.flt_avg_ + low) * M, M); // Must not be filtered out!
			EXPECT_EQ(stats.flt_calls_, ws.flt_calls_);
			EXPECT_EQ(stats.flt_cnt_, ws.flt_cnt_);
			EXPECT_DOUBLE_EQ(stats.flt_avg_, ws.flt_avg_);
			EXPECT_DOUBLE_EQ(stats.flt_ss_, ws.flt_ss_);
		}
#	endif
#endif
	} // TEST_P(ApiTest_t, vi_tmStatsAdd)

	INSTANTIATE_TEST_SUITE_P(api, ApiTest_t, ::testing::Values(1, 100));

} // namespace
