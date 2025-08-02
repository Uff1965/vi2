#include <gtest/gtest.h>
#include <vi_timing/vi_timing.hpp>

#include <chrono>
#include <iomanip>

using namespace std::literals;
namespace ch = std::chrono;

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

namespace
{
	void prn_header()
	{	const auto tm = ch::system_clock::to_time_t(ch::system_clock::now());

#pragma warning(suppress: 4996) // Suppress MSVC warning: 'localtime': This function or variable may be unsafe.
		std::cout << "\nStart: " << std::put_time(std::localtime(&tm), "%F %T.\n") <<
			std::endl;

		std::cout <<
			"Information about the \'vi_timing\' library:\n"
			"\tVersion: " << VI_TM_FULLVERSION << "\n"
			//"\tBuild type: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_BUILDTYPE)) << "\n"
			//"\tVer: " << *static_cast<const unsigned *>(vi_tmStaticInfo(VI_TM_INFO_VER)) << "\n"
			//"\tBuild number: " << *static_cast<const unsigned *>(vi_tmStaticInfo(VI_TM_INFO_BUILDNUMBER)) << "\n"
			"\tGit describe: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_GIT_DESCRIBE)) << "\n"
			//"\tGit commit hash: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_GIT_COMMIT)) << "\n"
			"\tGit commit date and time: " << static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_GIT_DATETIME)) << "\n";
	}
}

int main(int argc, char** argv)
{	VI_TM_FUNC;
	prn_header();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
