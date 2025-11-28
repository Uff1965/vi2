#include "header.h"

#include <cstdio>
#include <map>
#include <string>

VI_TM(FILE_PATH);

VI_TM_HREG h_register = VI_TM_HGLOBAL;

namespace
{
	std::map<std::string, test_func_t>& instance()
	{	static std::map<std::string, test_func_t> instance;
		return instance;
	}
}

int register_test(const char* name, test_func_t fn)
{	instance().try_emplace(name, fn);
	return 0;
}

int main()
{	VI_TM_FUNC;

	auto hreg = vi_tmRegistryCreate();

	{	VI_TM_H(hreg, "1");
		VI_TM_GLOBALINIT("Timing report:\n", vi_tmSortByName, vi_tmSortAscending, "Goodbye!\n");

		std::fprintf(stdout, "Hellow, World!\n\n");

		vi_CurrentThreadAffinityFixate();
		vi_WarmUp(1, 500);
	}

	{	std::fprintf(stdout, "First execution:\n");
		h_register = vi_tmRegistryCreate();
		{	auto const h_meas = vi_tmRegistryGetMeas(h_register, "***ALL TESTS***");
			auto tm = vi_tm::scoped_probe_t::make_paused(h_meas);
			for (const auto &[name, func] : instance())
			{	vi_ThreadYield();
				std::fprintf(stdout, "Test: \'%s\' ...", name.c_str());
				{	auto resume = tm.scoped_resume();
					if (!func())
					{
						assert(false);
						std::fprintf(stderr, "Test %s failed\n", name.c_str());
						return 1;
					}
				}
				std::fprintf(stdout, "done\n");
			}
		}
		{	VI_TM_H(hreg, "2");
			vi_tmRegistryReport(h_register, vi_tmSortByName | vi_tmSortAscending);
		}
		{	VI_TM_H(hreg, "3");
			std::fprintf(stdout, "\n");
		}
		{	VI_TM_H(hreg, "4");
			vi_tmRegistryClose(h_register);
			h_register = VI_TM_HGLOBAL;
		}
	}

	std::fprintf(stdout, "Other executions:\nTesting: ");
	for (int n = 0; n < 100; ++n)
	{	auto const h_meas = vi_tmRegistryGetMeas(h_register, "***ALL TESTS***");
		auto tm = vi_tm::scoped_probe_t::make_paused(h_meas);
		auto len = std::fprintf(stdout, "%02d/100 ... ", n + 1);
		for (const auto &[name, func] : instance())
		{
			vi_ThreadYield();
			std::fflush(stdout);
			{	auto resume = tm.scoped_resume();
				if (!func())
				{	assert(false);
					std::fprintf(stderr, "Test %s failed\n", name.c_str());
					return 1;
				}
			}
		}
		while (len--)
		{	fputs("\b \b", stdout);
		}
	}

	{	VI_TM_H(hreg, "5");
		std::fprintf(stdout, "All tests done.\n\n");
	}
	{	VI_TM_H(hreg, "6");
		vi_CurrentThreadAffinityRestore();
	}
	{	VI_TM_H(hreg, "7");
		std::fprintf(stdout, "\nMisc:");
	}

	vi_tmRegistryReport(hreg, vi_tmSortByTime);
	vi_tmRegistryClose(hreg);
	return 0;
}
