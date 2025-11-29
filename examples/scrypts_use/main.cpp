#include "header.h"

#include <cstdio>
#include <map>
#include <string>

VI_TM(FILE_PATH);

namespace
{
	std::map<std::string, test_func_t>& instance()
	{	static std::map<std::string, test_func_t> instance;
		return instance;
	}
}

VI_TM_HREG h_register = VI_TM_HGLOBAL;

int register_test(const char* name, test_func_t fn)
{	instance().try_emplace(name, fn);
	return 0;
}

int main()
{	VI_TM_FUNC;

	auto hreg = vi_tmRegistryCreate();

	VI_TM_GLOBALINIT(
		vi_tmSortByName, vi_tmSortAscending, vi_tmShowResolution, vi_tmShowDuration,
		"Timing report:\n",
		"Goodbye!\n"
	);

	std::fprintf(stdout, "Hellow, World!\n\n");
	fprintf(
		stdout,
		"vi_timing version: \'%s\'\n",
		static_cast<const char *>(vi_tmStaticInfo(vi_tmInfoVersion))
		);

	vi_CurrentThreadAffinityFixate();
//		vi_WarmUp(1, 500);
	auto secperunit = *static_cast<const double*>(vi_tmStaticInfo(vi_tmInfoSecPerUnit));
	auto overhead = secperunit * *static_cast<const double*>(vi_tmStaticInfo(vi_tmInfoOverhead));
	fprintf(stdout, "Overhead: %.0f ns.\n\n", 1e9 * overhead);

	{	std::fprintf(stdout, "First execution:\n");
		h_register = vi_tmRegistryCreate();
		{	auto const h_meas = vi_tmRegistryGetMeas(h_register, "***ALL TESTS***");
			auto tm = vi_tm::scoped_probe_t::make_paused(h_meas);
			for (const auto &[name, func] : instance())
			{	vi_ThreadYield();
				std::fprintf(stdout, "Test: \'%s\' ...", name.c_str());
				{	auto resume = tm.scoped_resume();
					if (!func())
					{	assert(false);
						std::fprintf(stderr, "Test %s failed\n", name.c_str());
						return 1;
					}
				}
				std::fprintf(stdout, "done\n");
			}
		}
		vi_tmRegistryReport(h_register, vi_tmSortByName | vi_tmSortAscending);
		std::fprintf(stdout, "\n");
		vi_tmRegistryClose(h_register);
		h_register = VI_TM_HGLOBAL;
	}

	std::fprintf(stdout, "Other executions:\nTesting: ");
	for (int n = 0; n < 100; ++n)
	{	auto const h_meas = vi_tmRegistryGetMeas(h_register, "***ALL TESTS***");
		auto tm = vi_tm::scoped_probe_t::make_paused(h_meas);
		auto len = std::fprintf(stdout, "%02d/100 ... ", n + 1);
		for (const auto &[name, func] : instance())
		{	vi_ThreadYield();
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

	std::fprintf(stdout, "All tests done.\n\n");
	vi_CurrentThreadAffinityRestore();
	std::fprintf(stdout, "\nMisc:\n");

	vi_tmRegistryReport(hreg, vi_tmSortByTime);
	vi_tmRegistryClose(hreg);
	return 0;
}
