// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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
	constexpr char THREADFUNC_NAME_1[] = "threadFunc1";
	constexpr char THREADFUNC_NAME_2[] = "threadFunc2";
	constexpr char THREADFUNCLOOP_NAME[] = "threadFunc loop";
	constexpr auto LOOP_COUNT = 32'768U;
	constexpr auto CNT = 2U;
	constexpr auto DUR = 4U;
	const auto numThreads = 4U * std::thread::hardware_concurrency();
}

TEST(vi_tmMultithreaded, Add)
{
	auto threadFunc = [](unsigned)
		{	std::mt19937 gen{/*std::random_device{}()*/ };
			std::uniform_int_distribution dis{ 0, 3 };
			static auto const mfunc = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNC_NAME_1);
			(void)vi_tmGetTicks();

			std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

			for (auto i = 0U; i < LOOP_COUNT; ++i)
			{	auto const mloop = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNCLOOP_NAME);
				(void)vi_tmGetTicks();
				(void)vi_tmGetTicks();
				vi_tmMeasurementAdd(mloop, DUR, CNT);
			}

			(void)vi_tmGetTicks();
			vi_tmMeasurementAdd(mfunc, DUR, CNT);
		};

	{	std::vector<std::thread> threads;
		threads.reserve(numThreads);
		for (unsigned i = 0; i < numThreads; ++i)
		{	threads.emplace_back(threadFunc, i);
		}

		for (auto &t : threads)
		{	t.join();
		}
	}

	{	vi_tmMeasurementStats_t stats;

		auto m = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNC_NAME_1);
		vi_tmMeasurementGet(m, nullptr, &stats);
		ASSERT_EQ(vi_tmMeasurementStatsIsValid(&stats), 0);
		ASSERT_EQ(stats.calls_, numThreads);
		ASSERT_EQ(stats.cnt_, stats.calls_ * CNT);
		ASSERT_EQ(stats.sum_, stats.calls_ * DUR);

		m = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNCLOOP_NAME);
		vi_tmMeasurementGet(m, nullptr, &stats);
		ASSERT_EQ(vi_tmMeasurementStatsIsValid(&stats), 0);
		ASSERT_EQ(stats.calls_, numThreads * LOOP_COUNT);
		ASSERT_EQ(stats.cnt_, stats.calls_ * CNT);
		ASSERT_EQ(stats.sum_, stats.calls_ * DUR);
	}
}

TEST(vi_tmMultithreaded, AddGetReset)
{
	static auto const mfunc = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNC_NAME_2);
	auto threadFunc = [](unsigned)
		{	std::mt19937 gen{/*std::random_device{}()*/ };
			std::uniform_int_distribution dis{ 0, 3 };

			std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

			for (auto i = 0U; i < LOOP_COUNT; ++i)
			{	vi_tmMeasurementAdd(mfunc, DUR, CNT);
				vi_tmMeasurementStats_t stats;
				vi_tmMeasurementGet(mfunc, nullptr, &stats);
				ASSERT_EQ(vi_tmMeasurementStatsIsValid(&stats), 0);
				vi_tmMeasurementReset(mfunc);
			}
		};

	{	std::vector<std::thread> threads;
		threads.reserve(numThreads);
		for (unsigned i = 0; i < numThreads; ++i)
		{	threads.emplace_back(threadFunc, i);
		}

		for (auto &t : threads)
		{	t.join();
		}
	}

	{	vi_tmMeasurementStats_t stats;

		auto m = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNC_NAME_2);
		vi_tmMeasurementGet(m, nullptr, &stats);
		ASSERT_EQ(vi_tmMeasurementStatsIsValid(&stats), 0);
		ASSERT_EQ(stats.calls_, 0U);
		ASSERT_EQ(stats.cnt_, 0U);
		ASSERT_EQ(stats.sum_, 0);
	}
}
