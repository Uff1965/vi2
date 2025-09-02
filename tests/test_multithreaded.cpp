// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <cassert>
#include <chrono>
#include <thread>
#include <random>
#include <vector>

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace 
{
	constexpr char THREADFUNC_NAME[] = "threadFunc";
	constexpr char THREADFUNCLOOP_NAME[] = "threadFunc loop";
	constexpr auto CNT = 1'000;
	constexpr auto AMN = 2;
	const auto numThreads = 2U * std::thread::hardware_concurrency();
	std::mt19937 gen{/*std::random_device{}()*/ };
	std::uniform_int_distribution dis{ 1, 3 };
	std::mutex mtx;

	void threadFunc(unsigned)
	{	VI_TM(THREADFUNC_NAME);

		{	std::lock_guard<std::mutex> lock(mtx);
			std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
		}

		for (auto i = 0; i < CNT; ++i)
		{	VI_TM(THREADFUNCLOOP_NAME, AMN);
		}
	}
}

TEST(vi_timing, vi_tmMultithreaded)
{
	std::vector<std::thread> threads;
	threads.reserve(numThreads);
	for (unsigned i = 0; i < numThreads; ++i)
	{	threads.emplace_back(threadFunc, i);
	}

	for (auto& t : threads)
	{	t.join();
	}

	vi_tmMeasurementStats_t stats;

	auto m = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNC_NAME);
	vi_tmMeasurementGet(m, nullptr, &stats);
	ASSERT_EQ(vi_tmMeasurementStatsIsValid(&stats), 0);
	ASSERT_EQ(stats.calls_, numThreads);
	ASSERT_EQ(stats.cnt_, stats.calls_);

	m = vi_tmMeasurement(VI_TM_HGLOBAL, THREADFUNCLOOP_NAME);
	vi_tmMeasurementGet(m, nullptr, &stats);
	ASSERT_EQ(vi_tmMeasurementStatsIsValid(&stats), 0);
	ASSERT_EQ(stats.calls_, numThreads * CNT);
	ASSERT_EQ(stats.cnt_, stats.calls_ * AMN);
}
