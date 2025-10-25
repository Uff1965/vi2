#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <cassert>
#include <chrono>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace
{
	constexpr char THREADFUNC_NAME[] = "threadFunc2";
	constexpr char THREADFUNCLOOP_NAME[] = "threadFunc loop";
	constexpr auto LOOP_COUNT = 32'768U;
	constexpr auto CNT = 2U;
	constexpr auto DUR = 4U;
	const std::size_t numThreads = []
		{	constexpr auto minThreads = 4U;
			return minThreads + std::thread::hardware_concurrency();
		}();
}

TEST(Multithreaded, vi_tmRegistryGetMeas)
{
	auto action = []
		{	auto threadFunc = [](int delay)
			{	std::this_thread::sleep_for(std::chrono::milliseconds{ delay });
				for (auto i = 0U; i < LOOP_COUNT; ++i)
				{	auto const meas = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, THREADFUNCLOOP_NAME);
					vi_tmMeasurementAdd(meas, DUR, CNT);
				}
			};

			std::mt19937 gen;
			std::uniform_int_distribution dis{ 0, 3 };
			std::vector<std::thread> threads{ numThreads };
			for (auto &t : threads) t = std::thread{ threadFunc, dis(gen) };
			for (auto &t : threads) t.join();
		};

	ASSERT_NO_THROW(action());

	{	vi_tmStats_t stats;
		auto meas = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, THREADFUNCLOOP_NAME);
		vi_tmMeasurementGet(meas, nullptr, &stats);
		ASSERT_EQ(vi_tmStatsIsValid(&stats), 0);
		EXPECT_EQ(stats.calls_, numThreads * LOOP_COUNT);
#if VI_TM_STAT_USE_RAW
		EXPECT_EQ(stats.cnt_, stats.calls_ * CNT);
		EXPECT_EQ(stats.sum_, stats.calls_ * DUR);
#endif
#if VI_TM_STAT_USE_MINMAX
		EXPECT_EQ(stats.min_, DUR / CNT);
		EXPECT_EQ(stats.max_, DUR / CNT);
#endif
#if VI_TM_STAT_USE_RMSE
		EXPECT_EQ(stats.flt_calls_, numThreads * LOOP_COUNT);
		EXPECT_EQ(stats.flt_cnt_, stats.calls_ * CNT);
		EXPECT_EQ(stats.flt_avg_, DUR / CNT);
		EXPECT_EQ(stats.flt_ss_, 0U);
#endif
	}
}

TEST(Multithreaded, vi_tmMeasurementAdd)
{	static auto const meas = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, THREADFUNC_NAME);
	auto action = []
		{	auto threadFunc = [](int delay)
			{	std::this_thread::sleep_for(std::chrono::milliseconds{ delay });
				for (auto i = 0U; i < LOOP_COUNT; ++i)
				{	vi_tmMeasurementAdd(meas, DUR, CNT);
					vi_tmStats_t stats;
					vi_tmMeasurementGet(meas, nullptr, &stats);
					ASSERT_EQ(vi_tmStatsIsValid(&stats), 0);
				}
			};

			std::mt19937 gen;
			std::uniform_int_distribution dis{ 0, 3 };
			std::vector<std::thread> threads{numThreads};
			for (auto &t : threads) t = std::thread{ threadFunc, dis(gen) };
			for (auto &t : threads) t.join();
		};

	ASSERT_NO_THROW(action());

	{	vi_tmStats_t stats;
		vi_tmMeasurementGet(meas, nullptr, &stats);
		ASSERT_EQ(vi_tmStatsIsValid(&stats), 0);
		EXPECT_EQ(stats.calls_, LOOP_COUNT * numThreads);
#if VI_TM_STAT_USE_RAW
		EXPECT_EQ(stats.cnt_, stats.calls_ * CNT);
		EXPECT_EQ(stats.sum_, stats.calls_ * DUR);
#endif
#if VI_TM_STAT_USE_MINMAX
		EXPECT_EQ(stats.min_, DUR / CNT);
		EXPECT_EQ(stats.max_, DUR / CNT);
#endif
#if VI_TM_STAT_USE_RMSE
		EXPECT_EQ(stats.flt_calls_, stats.calls_);
		EXPECT_EQ(stats.flt_cnt_, stats.calls_ * CNT);
		EXPECT_EQ(stats.flt_avg_, DUR / CNT);
		EXPECT_EQ(stats.flt_ss_, 0U);
#endif
	}
}
