#include <vi_timing/vi_timing.hpp>
#include "../test_shared_lib/header.h"

#ifdef VI_HAS_GTEST
#	include <gtest/gtest.h>
#endif

#ifdef VI_TM_ENABLE_BENCHMARK
#	include <benchmark/benchmark.h>
#endif

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string_view>

using namespace std::literals;
namespace ch = std::chrono;

namespace
{
	std::string flags2string(unsigned flg)
	{	std::string result;
		result += (flg & vi_tmStatUseBase)? "VI_TM_STAT_USE_RAW, ": "";
		result += (flg & vi_tmStatUseRMSE)? "VI_TM_STAT_USE_RMSE, ": "";
		result += (flg & vi_tmStatUseFilter)? "VI_TM_STAT_USE_FILTER, ": "";
		result += (flg & vi_tmStatUseMinMax)? "VI_TM_STAT_USE_MINMAX, ": "";
		result += (flg & vi_tmThreadsafe)? "VI_TM_THREADSAFE, ": "";
		result += (flg & vi_tmShared)? "VI_TM_SHARED, ": "";
		result += (flg & vi_tmDebug)? "VI_TM_DEBUG, ": "";
		if(!result.empty())
		{	result.resize(result.size() - 2U);
		}
		return result;
	}

	void header(std::ostream& stream)
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());
		const std::string flags = flags2string(*static_cast<const unsigned *>(vi_tmStaticInfo(vi_tmInfoFlags)));

		std::tm tm_buf;
#ifdef _MSC_VER
		if (localtime_s(&tm_buf, &tm))
		{	assert(false);
		}
#else
		localtime_r(&tm, &tm_buf);
#endif
		stream << "Start: " << std::put_time(&tm_buf, "%F %T.\n") << std::endl;
		stream << "Information about the \'vi_timing\' library:\n"
			"\tVersion: " << VI_TM_FULLVERSION << ";\n"
			"\tBuild flags: " << (flags.empty() ? "<none>" : flags) << ";\n"
			"\tGit describe: " << static_cast<const char *>(vi_tmStaticInfo(vi_tmInfoGitDescribe)) << ";\n"
			"\tGit commit date and time: " << static_cast<const char *>(vi_tmStaticInfo(vi_tmInfoGitDateTime)) << ".\n";
		endl(stream);

		const auto unit = *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoSecPerUnit));
		const auto duration = *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoDuration));
		const auto overhead = *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoOverhead));
		const auto resolution = *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoResolution));
		stream <<
			"Static info:\n"
			"\tDivision price: " << vi_tm::to_string(unit, 3) << "s.\n"
			"\tClock resolution: " << vi_tm::to_string(resolution * unit, 3) << "s.\n"
			"\tClock overhead: " << vi_tm::to_string(overhead * unit, 3) << "s.\n"
			"\tMeasurement duration: " << vi_tm::to_string(duration * unit, 3) << "s.\n";
		endl(stream);
	}

	void metrics(std::ostream &stream)
	{	vi_CurrentThreadAffinityFixate();

		stream << "Calculated:\n";
		{	constexpr auto N = 8U;
			vi_ThreadYield(); // Reduce likelihood of thread interruption during measurement.
			const auto first = vi_tmGetTicks();
			auto last = first;
			for (auto cnt = N; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}
			stream << "\tvi_tmGetTicks() resolution: " << (last - first) / N << " ticks.\n";
		}

		{	constexpr auto CNT = 1'000'000U;
			volatile auto _ = vi_tmGetTicks();
			vi_ThreadYield();
			const auto start = ch::steady_clock::now();
			for (unsigned n = 0U; n < CNT; ++n)
			{
				_ = vi_tmGetTicks();
			}
			const auto finish = ch::steady_clock::now();
			const auto duration = ch::duration_cast<ch::nanoseconds>(finish - start).count();
			stream << "\tvi_tmGetTicks() duration: " << duration / CNT << " ns.\n";
		}

		vi_CurrentThreadAffinityRestore();
	}

	bool any_gtest_arg(int argc, char **argv)
	{	static constexpr auto sample = "--gtest_"sv;
		return std::any_of(
			argv,
			argv + argc,
			[](const char *a)
			{	return std::string_view{ a }.substr(0, sample.size()) == sample;
			}
		);
	}
}

int main(int argc, char** argv)
{
	VI_TM_GLOBALINIT(vi_tmReportDefault, "Timing report:\n", "Success - the test program completed!\n");

	const auto gtest_arg = any_gtest_arg(argc, argv);

	if (!gtest_arg)
	{	header(std::cout);
		vi_WarmUp();
		metrics(std::cout);
		endl(std::cout);
	}

	test_shared_lib_func();

#if VI_HAS_GTEST
	::testing::InitGoogleTest(&argc, argv);
	if (const auto ret = RUN_ALL_TESTS(); gtest_arg || !!ret )
	{	return ret;
	}
#endif

#ifdef VI_TM_ENABLE_BENCHMARK
	::benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv))
	{	return 1;
	}

	endl(std::cout);
	std::cout << "Benchmark:\n";
	::benchmark::RunSpecifiedBenchmarks();
	endl(std::cout);
#endif

	return 0;
}
