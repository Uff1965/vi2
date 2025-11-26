#include "header.h"

#include <vi_timing/vi_timing.h>

#include <vector>

VI_TM(FILE_PATH);

namespace
{
	std::vector<test_func_t>& instance()
	{	static std::vector<test_func_t> instance;
		return instance;
	}
}

int register_test(test_func_t fn)
{	instance().emplace_back(fn);
	return 0;
}

int main()
{	VI_TM_FUNC;
	VI_TM_GLOBALINIT("Header:\n", vi_tmSortByName, vi_tmSortAscending, "\nHello, World!\n");

	vi_CurrentThreadAffinityFixate();
	vi_WarmUp(0, 500);

	for (int n = 0; n < 100; ++n)
	{	for (const auto &func : instance())
		{	vi_ThreadYield();
			func();
		}
	}

	vi_CurrentThreadAffinityRestore();
	return 0;
}
