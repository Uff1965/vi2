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

#if _MSC_VER
#	define MS_WARN(s) _Pragma(VI_STRINGIZE(warning(s)))
#else
#	define MS_WARN(s)
#endif

VI_TM_INIT(vi_tmReportCb_t{});

namespace
{
	void header(std::ostream& stream)
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());

		stream << "Start: " << std::put_time(std::localtime(&tm), "%F %T.\n") << std::endl;

		stream <<
			"Information about the \'vi_timing\' library:\n"
			"\tVersion: " << VI_TM_FULLVERSION << "\n"
			"\tGit describe: " << static_cast<const char *>(vi_tmStaticInfo(VI_TM_INFO_GIT_DESCRIBE)) << "\n"
			"\tGit commit date and time: " << static_cast<const char *>(vi_tmStaticInfo(VI_TM_INFO_GIT_DATETIME)) << "\n";
		endl(std::cout);

		const auto unit = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_UNIT));
		const auto duration = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_DURATION));
		const auto overhead = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_OVERHEAD));
		const auto resolution = *static_cast<const double *>(vi_tmStaticInfo(VI_TM_INFO_RESOLUTION));
		stream <<
			"\tResolution: " << static_cast<int>(resolution * unit * 1e9) << " ns.\n"
			"\tDuration: " << static_cast<int>(duration * unit * 1e9) << " ns.\n";
		endl(std::cout);
	}
}

int main(int argc, char** argv)
{
	header(std::cout);

	const bool gtest_list_tests = std::any_of(argv, argv + argc, [](auto arg) { return "--gtest_list_tests"sv == arg; });

    ::testing::InitGoogleTest(&argc, argv);
	::benchmark::Initialize(&argc, argv);

	if (::benchmark::ReportUnrecognizedArguments(argc, argv))
	{	return 1;
    }

	if (auto ret = RUN_ALL_TESTS())
	{	return ret;
	}

	if (!gtest_list_tests)
	{	endl(std::cout);
		std::cout << "Benchmark:\n";
		::benchmark::RunSpecifiedBenchmarks();
	}

	return 0;
}
