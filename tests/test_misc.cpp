#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(HelloTest2, BasicAssertions)
{	VI_TM_FUNC;
	// Expect two strings not to be equal.
	EXPECT_STRNE("hello", "world");
	// Expect equality.
	EXPECT_EQ(7 * 6, 42);
}

// Demonstrate some basic assertions.
TEST(HelloTest3, BasicAssertions)
{	VI_TM_FUNC;
	// Expect two strings not to be equal.
	EXPECT_STRNE("hello", "world");
	// Expect equality.
	EXPECT_EQ(7 * 6, 42);
}
