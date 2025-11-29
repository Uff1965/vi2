#include "header.h"

#include <cstdio>
#include <map>
#include <string>

namespace
{
	auto& instance()
	{	static std::map<std::string, test_func_t> instance;
		return instance;
	}
}

VI_TM_HREG g_register = VI_TM_HGLOBAL;

int register_test(const char* name, test_func_t fn)
{	instance().try_emplace(name, fn);
	return 0;
}

int main()
{	VI_TM_FUNC;
	VI_TM_GLOBALINIT(
		vi_tmSortByName, vi_tmSortAscending, vi_tmShowResolution, vi_tmShowDuration,
		"Timing report:\n",
		"Goodbye!\n"
	);

	std::fprintf(stdout, "Hellow, World!\n\n");
	std::fprintf(stdout, "vi_timing version: \'%s\'\n", VI_TM_FULLVERSION);

	vi_CurrentThreadAffinityFixate();
	const auto sec_per_unit = *static_cast<const double*>(vi_tmStaticInfo(vi_tmInfoSecPerUnit));
	const auto overhead = sec_per_unit * *static_cast<const double*>(vi_tmStaticInfo(vi_tmInfoOverhead));
	fprintf(stdout, "Overhead: %.0f ns.\n\n", 1e9 * overhead);

	{	std::fprintf(stdout, "First execution:\n");
		g_register = vi_tmRegistryCreate();
		{	auto const meas = vi_tmRegistryGetMeas(g_register, "***ALL TESTS***");
			auto st = vi_tm::scoped_probe_t::make_paused(meas);
			for (const auto &[name, func] : instance())
			{	vi_ThreadYield(); // We give the remainder of the time quantum to other threads so that we can start measuring the new one from the beginning.
				std::fprintf(stdout, "Test: \'%s\'... ", name.c_str());
				{	auto sr = st.scoped_resume();
					if (!func())
					{	assert(false);
						std::fprintf(stderr, "Test %s failed\n", name.c_str());
						return 1;
					}
				} // scope for sr.
				std::fprintf(stdout, "done\n");
			}
		} // scope for st.
		vi_tmRegistryReport(g_register, vi_tmSortByName | vi_tmSortAscending);
		vi_tmRegistryClose(g_register);
		g_register = VI_TM_HGLOBAL;
	}

	std::fprintf(stdout, "\nOther executions:\nTesting: ");
	for (int n = 0; n < 100; ++n)
	{	auto const meas = vi_tmRegistryGetMeas(g_register, "***ALL TESTS***");
		auto st = vi_tm::scoped_probe_t::make_paused(meas);
		auto len = std::fprintf(stdout, "%02d/100... ", n + 1);
		for (const auto &[name, func] : instance())
		{	vi_ThreadYield();
			std::fflush(stdout);
			{	auto resume = st.scoped_resume();
				if (!func())
				{	assert(false);
					std::fprintf(stderr, "Test %s failed\n", name.c_str());
					return 1;
				}
			}
		}
		// Erase the progress indication.
		while (len--)
		{	fputs("\b \b", stdout);
		}
	}

	std::fprintf(stdout, "... done.\n\n");
	vi_CurrentThreadAffinityRestore();
	return 0;
}
