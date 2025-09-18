#include "header.h"

#include <vi_timing/vi_timing.hpp>
//#include <gtest/gtest.h>

//VI_TM_INIT();

VI_TM("GLOBAL lib/source.cpp");

//TEST(vi_tmF2A, common)
//{
//}

void test_shared_lib_func(void)
{
	VI_TM("lib/source.cpp::test_shared_lib_func");
}
