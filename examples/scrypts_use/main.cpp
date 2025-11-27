#include "header.h"

#include <vi_timing/vi_timing.h>

#include <vector>
#include <cstdio>

VI_TM(FILE_PATH);

VI_TM_HREG h_register = VI_TM_HGLOBAL;

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
	VI_TM_GLOBALINIT("Timing report:\n", vi_tmSortByName, vi_tmSortAscending, "Goodbye!\n");

	std::fprintf(stdout, "Hellow, World!\n\n");

	vi_CurrentThreadAffinityFixate();
	vi_WarmUp(1, 500);

	{	h_register = vi_tmRegistryCreate();
		{	TM("***ALL TESTS***");
			for (const auto &func : instance())
			{	vi_ThreadYield();
				func();
			}
		}
		std::fprintf(stdout, "First execution\n");
		vi_tmRegistryReport(h_register, vi_tmSortByName | vi_tmSortAscending);
		std::fprintf(stdout, "\n");
		vi_tmRegistryClose(h_register);
		h_register = VI_TM_HGLOBAL;
	}

	for (int n = 0; n < 100; ++n)
	{	VI_TM("***ALL TESTS***");
		for (const auto &func : instance())
		{	vi_ThreadYield();
			func();
		}
	}

	vi_CurrentThreadAffinityRestore();
	return 0;
}
