// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/vi_timing.hpp>

#ifdef VI_HAS_GTEST
#	include <gtest/gtest.h>
#endif

#ifdef VI_HAS_GBENCHMARK
#	include <benchmark/benchmark.h>
#endif

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string_view>

using namespace std::literals;
namespace ch = std::chrono;

VI_TM_INIT();
//VI_TM_INIT(vi_tmReportCb_t{});
//VI_TM_INIT(vi_tmGetTicksPtr);
//VI_TM_INIT(+[]() noexcept ->VI_TM_TICK { timespec ts; (void)timespec_get(&ts, TIME_UTC); return 1'000'000'000U * ts.tv_sec + ts.tv_nsec; });
//VI_TM_INIT(+[]() noexcept ->VI_TM_TICK { return clock(); });

namespace
{
	void header(std::ostream& stream)
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());

		stream << "Start: " << std::put_time(std::localtime(&tm), "%F %T.\n") << std::endl;

		std::string flags;
		{	const auto flg = *static_cast<const unsigned *>(vi_tmStaticInfo(VI_TM_INFO_FLAGS));
			flags += (flg & vi_tmDebug)? "VI_TM_DEBUG, ": "";
			flags += (flg & vi_tmShared)? "VI_TM_SHARED, ": "";
			flags += (flg & vi_tmThreadsafe)? "VI_TM_THREADSAFE, ": "";
			flags += (flg & vi_tmStatUseBase)? "VI_TM_STAT_USE_RAW, ": "";
			flags += (flg & vi_tmStatUseFilter)? "VI_TM_STAT_USE_FILTER, ": "";
			flags += (flg & vi_tmStatUseMinMax)? "VI_TM_STAT_USE_MINMAX, ": "";
			if(!flags.empty())
			{	flags.resize(flags.size() - 2U);
			}
		}
		stream << "Library version: " << static_cast<const char *>(vi_tmStaticInfo(VI_TM_INFO_VERSION)) << ".\n";
		stream <<
			"Information about the \'vi_timing\' library:\n"
			"\tVersion: " << VI_TM_FULLVERSION << "\n"
			"\tLibrary build flags: " << (flags.empty() ? "<none>" : flags) << "\n"
			"\tGit describe: " << static_cast<const char *>(vi_tmStaticInfo(VI_TM_INFO_GIT_DESCRIBE)) << "\n"
			"\tGit commit date and time: " << static_cast<const char *>(vi_tmStaticInfo(VI_TM_INFO_GIT_DATETIME)) << "\n";
		endl(stream);

		const auto unit = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_UNIT));
		const auto duration = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_DURATION));
		const auto overhead = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_OVERHEAD));
		const auto resolution = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_RESOLUTION));
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
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	if (!::testing::GTEST_FLAG(list_tests))
	{	header(std::cout);
		vi_WarmUp();
		metrics(std::cout);
		endl(std::cout);
	}

	if (auto ret = RUN_ALL_TESTS())
	{	return ret;
	}

	if (!::testing::GTEST_FLAG(list_tests))
	{	::benchmark::Initialize(&argc, argv);
		if (::benchmark::ReportUnrecognizedArguments(argc, argv))
		{	return 1;
		}

		endl(std::cout);
		std::cout << "Benchmark:\n";
		::benchmark::RunSpecifiedBenchmarks();
	}

	return 0;
}
