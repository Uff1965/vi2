#include "test.h"

#include <vi_timing/vi_timing.hpp>
#include <vi_timing/vi_timing_aux.h>

#include <gtest/gtest.h>

TEST(misc, vi_tmF2A)
{
	struct
	{	int line_; double value_; std::string_view expected_; unsigned char significant_; unsigned char decimal_;
	} static const tests_set[] =
	{
		// The following test cases check the boundary and special floating-point values for the misc::to_string function.
		// They ensure correct handling of INF, -INF, NaN, -NaN, DBL_MAX, DBL_MIN, DBL_TRUE_MIN, and values near zero.
		{__LINE__, 0.0, "0.0  ", 2, 1},
		{__LINE__, NAN, "NaN", 2, 1},									{__LINE__, -NAN, "NaN", 2, 1},
		{__LINE__, std::nextafter( 0.0, 1.0), "0.0  ", 2, 1},			{__LINE__, std::nextafter(-0.0, -1.0), "0.0  ", 2, 1},
		{__LINE__, DBL_TRUE_MIN, "0.0  ", 2, 1},						{__LINE__, -DBL_TRUE_MIN, "0.0  ", 2, 1},
		{__LINE__, std::nextafter( DBL_TRUE_MIN, 1.0), "0.0  ", 2, 1},	{__LINE__, std::nextafter(-DBL_TRUE_MIN, -1.0), "0.0  ", 2, 1},
		{__LINE__, std::nextafter( DBL_MIN, 0.0), "0.0  ", 2, 1},		{__LINE__, std::nextafter(-DBL_MIN, 0.0), "0.0  ", 2, 1},
		{__LINE__, DBL_MIN, "22.0e-309", 2, 1},							{__LINE__, -DBL_MIN, "-22.0e-309", 2, 1},
		{__LINE__, 3.14159, "3.1  ", 2, 1},								{__LINE__, -3.14159, "-3.1  ", 2, 1},
		{__LINE__, DBL_MAX, "180.0e306", 2, 1},							{__LINE__, -DBL_MAX, "-180.0e306", 2, 1},
		{__LINE__, DBL_MAX * (1.0 + DBL_EPSILON),  "INF", 2, 1},		{__LINE__, -DBL_MAX * (1.0 + DBL_EPSILON), "-INF", 2, 1},
		// Test cases for SI prefixes and scientific notation formatting in misc::to_string.
		// Each entry checks that a value like 1e3, 1e-3, etc., is formatted with the correct SI suffix or scientific notation.
		{ __LINE__, 1e-306,"1.0e-306", 2, 1 },	{ __LINE__, -1e-306,"-1.0e-306", 2, 1 },
		{ __LINE__, 1e-30,"1.0 q", 2, 1 },		{ __LINE__, -1e-30,"-1.0 q", 2, 1 },
		{ __LINE__, 1e-27,"1.0 r", 2, 1 },		{ __LINE__, -1e-27,"-1.0 r", 2, 1 },
		{ __LINE__, 1e-24,"1.0 y", 2, 1 },		{ __LINE__, -1e-24,"-1.0 y", 2, 1 },
		{ __LINE__, 1e-21,"1.0 z", 2, 1 },		{ __LINE__, -1e-21,"-1.0 z", 2, 1 },
		{ __LINE__, 1e-18,"1.0 a", 2, 1 },		{ __LINE__, -1e-18,"-1.0 a", 2, 1 },
		{ __LINE__, 1e-15,"1.0 f", 2, 1 },		{ __LINE__, -1e-15,"-1.0 f", 2, 1 },
		{ __LINE__, 1e-12,"1.0 p", 2, 1 },		{ __LINE__, -1e-12,"-1.0 p", 2, 1 },
		{ __LINE__, 1e-9,"1.0 n", 2, 1 },		{ __LINE__, -1e-9,"-1.0 n", 2, 1 },
		{ __LINE__, 1e-6,"1.0 u", 2, 1 },		{ __LINE__, -1e-6,"-1.0 u", 2, 1 },
        { __LINE__, 1e-3,"1.0 m", 2, 1 },		{ __LINE__, -1e-3,"-1.0 m", 2, 1 },
        { __LINE__, 1e0,"1.0  ", 2, 1 },		{ __LINE__, -1e0,"-1.0  ", 2, 1 },
        { __LINE__, 1e3,"1.0 k", 2, 1 },		{ __LINE__, -1e3,"-1.0 k", 2, 1 },
        { __LINE__, 1e6,"1.0 M", 2, 1 },		{ __LINE__, -1e6,"-1.0 M", 2, 1 },
        { __LINE__, 1e9,"1.0 G", 2, 1 },		{ __LINE__, -1e9,"-1.0 G", 2, 1 },
        { __LINE__, 1e12,"1.0 T", 2, 1 },		{ __LINE__, -1e12,"-1.0 T", 2, 1 },
        { __LINE__, 1e15,"1.0 P", 2, 1 },		{ __LINE__, -1e15,"-1.0 P", 2, 1 },
        { __LINE__, 1e18,"1.0 E", 2, 1 },		{ __LINE__, -1e18,"-1.0 E", 2, 1 },
        { __LINE__, 1e21,"1.0 Z", 2, 1 },		{ __LINE__, -1e21,"-1.0 Z", 2, 1 },
        { __LINE__, 1e24,"1.0 Y", 2, 1 },		{ __LINE__, -1e24,"-1.0 Y", 2, 1 },
        { __LINE__, 1e27,"1.0 R", 2, 1 },		{ __LINE__, -1e27,"-1.0 R", 2, 1 },
        { __LINE__, 1e30,"1.0 Q", 2, 1 },		{ __LINE__, -1e30,"-1.0 Q", 2, 1 },
        { __LINE__, 1e306,"1.0e306", 2, 1 },	{ __LINE__, -1e306,"-1.0e306", 2, 1 },
		// rounding tests.
		{__LINE__, 1.19, "1.2  ", 2, 1},		{__LINE__, -1.19, "-1.2  ", 2, 1}, // simple
		{__LINE__, 9.99, "10.0  ", 2, 1},		{__LINE__, -9.99, "-10.0  ", 2, 1},
		{ __LINE__, 1.349, "1.3  ", 2, 1 },		{ __LINE__, -1.349, "-1.3  ", 2, 1 },
		{ __LINE__, 1.35, "1.4  ", 2, 1 },		{ __LINE__, -1.35, "-1.4  ", 2, 1 }, // 0.5 rounding up.
		// test group sellect
		{__LINE__, 0.0001, "100.0 u", 2, 1},	{__LINE__, -0.0001, "-100.0 u", 2, 1},
		{__LINE__, 0.001, "1.0 m", 2, 1},		{__LINE__, -0.001, "-1.0 m", 2, 1},
		{__LINE__, 0.01, "10.0 m", 2, 1},		{__LINE__, -0.01, "-10.0 m", 2, 1},
		{__LINE__, 0.1, "100.0 m", 2, 1},		{__LINE__, -0.1, "-100.0 m", 2, 1},
		{__LINE__, 1.0, "1.0  ", 2, 1},			{__LINE__, -1.0, "-1.0  ", 2, 1},
		{__LINE__, 10.0, "10.0  ", 2, 1},		{__LINE__, -10.0, "-10.0  ", 2, 1},
		{__LINE__, 100.0, "100.0  ", 2, 1},		{__LINE__, -100.0, "-100.0  ", 2, 1},
		{__LINE__, 1000.0, "1.0 k", 2, 1},		{__LINE__, -1000.0, "-1.0 k", 2, 1},
		{__LINE__, 0.1, "100000.0 u", 5, 1},	{__LINE__, -0.1, "-100000.0 u", 5, 1},
		{__LINE__, 1.0, "1000.0 m", 5, 1},		{__LINE__, -1.0, "-1000.0 m", 5, 1},
		{__LINE__, 10.0, "10000.0 m", 5, 1},	{__LINE__, -10.0, "-10000.0 m", 5, 1},
		{__LINE__, 100.0, "100000.0 m", 5, 1},	{__LINE__, -100.0, "-100000.0 m", 5, 1},
		{__LINE__, 1000.0, "1000.0  ", 5, 1},	{__LINE__, -1000.0, "-1000.0  ", 5, 1},
	};

	std::string buff(32, '\0');

	for (auto &test : tests_set)
	{	EXPECT_GE(buff.size(), vi_tmF2A(buff.data(), buff.size(), test.value_, test.significant_, test.decimal_));
		EXPECT_STREQ(buff.data(), test.expected_.data()) << "Line of sample: " << test.line_;
	}
}
