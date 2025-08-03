#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>

using namespace std::literals;
namespace ch = std::chrono;

#if _MSC_VER
#	define MS_WARN(s) _Pragma(VI_STRINGIZE(warning(s)))
#else
#	define MS_WARN(s)
#endif

VI_TM_INIT("\nTiming: ", vi_tmSortBySpeed, vi_tmShowResolution, vi_tmShowOverhead, vi_tmShowDuration);
VI_TM("Global");

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions)
{	VI_TM_FUNC;
	// Expect two strings not to be equal.
	EXPECT_STRNE("hello", "world");
	// Expect equality.
	EXPECT_EQ(7 * 6, 42);
}

namespace
{
	void prn_header()
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());

		std::cout << "\nStart: " << std::put_time(std::localtime(&tm), "%F %T.\n") <<
			std::endl;

		std::cout <<
			"Information about the \'vi_timing\' library:\n"
			"\tVersion: " << VI_TM_FULLVERSION << "\n"
			"\tGit describe: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_GIT_DESCRIBE)) << "\n"
			"\tGit commit date and time: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_GIT_DATETIME)) << "\n";
	}
}

int main(int argc, char** argv)
{	VI_TM_FUNC;
	prn_header();
	endl(std::cout);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
