#include "header.h"

#include <vi_timing/vi_timing.hpp>
//#include <gtest/gtest.h>

//VI_TM_INIT(vi_tmReportCb_t{});

VI_TM("GLOBAL lib/source.cpp");

//TEST(vi_tmF2A, common)
//{
//}

void test_lib_func(void)
{
	VI_TM("lib/source.cpp::test_lib_func");
}
