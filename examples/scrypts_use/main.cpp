#include "header.h"

#include <cstdio>
#include <map>
#include <string>

namespace
{
	auto& test_registry()
	{	static std::map<std::string, test_func_t> test_registry;
		return test_registry;
	}

	// Helper function to display timing system information
	void print_timing_info()
	{	const auto sec_per_unit = *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoSecPerUnit));
		const auto overhead = sec_per_unit * *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoOverhead));
		const auto resolution = *static_cast<const double *>(vi_tmStaticInfo(vi_tmInfoResolution));

		std::fprintf(stdout, "Timing Information:\n");
		std::fprintf(stdout, "  Version: \'%s\'\n", VI_TM_FULLVERSION);
		std::fprintf(stdout, "  Clock resolution: %.1f ns\n", 1e9 * resolution * sec_per_unit); // Also calls vi_WarmUp()
		std::fprintf(stdout, "  Measurement overhead: %.1f ns\n", 1e9 * overhead);
		std::fprintf(stdout, "  Timer frequency: %.0f MHz\n\n", 1e-6 / sec_per_unit);
	}

	bool all_test()
	{
		for (const auto &[name, func] : test_registry())
		{	vi_ThreadYield();
			if (!func())
			{	assert(false);
				std::fprintf(stderr, "Test %s failed\n", name.c_str());
				return false;
			}
		}
		return true;
	}

	bool run_phase1()
	{	std::fprintf(stdout, "First execution:\n");
		g_register = vi_tmRegistryCreate();
		{	if(!all_test())
			{	return false;
			}
		} // scope for st.
		vi_tmRegistryReport(g_register, vi_tmSortByName | vi_tmSortAscending);
		vi_tmRegistryClose(g_register);
		g_register = VI_TM_HGLOBAL;
		return true;
	}

	bool run_phase2()
	{	constexpr int num_iterations = 100;
		std::fprintf(stdout, "\nOther executions:\nTesting: ");
		for (int n = 0; n < num_iterations; ++n)
		{	auto len = std::fprintf(stdout, "%02d/%d... ", n + 1, num_iterations);
			if(!all_test())
			{	return false;
			}
			// Erase the progress indication.
			while (len--)
			{	fputs("\b \b", stdout);
			}
		}
		std::fprintf(stdout, "... done.\n\n");
		return true;
	}
}

VI_TM_HREG g_register = VI_TM_HGLOBAL;

int register_test(const char* name, test_func_t fn)
{	test_registry().try_emplace(name, fn);
	return 0;
}

int main()
{	VI_TM_FUNC;

	std::fprintf(stdout, "Hellow, World!\n\n");

	vi_CurrentThreadAffinityFixate(); // Fix thread affinity for more consistent timing measurements
	print_timing_info();

	// Initialize timing system with comprehensive reporting options.
	VI_TM_GLOBALINIT(
		vi_tmSortByName, vi_tmSortAscending, vi_tmShowResolution, vi_tmShowDuration,
		"Timing report:\n",
		"Goodbye!\n"
	);

	const auto sec_per_unit = *static_cast<const double*>(vi_tmStaticInfo(vi_tmInfoSecPerUnit));
	const auto overhead = sec_per_unit * *static_cast<const double*>(vi_tmStaticInfo(vi_tmInfoOverhead));
	fprintf(stdout, "Overhead: %.0f ns.\n\n", 1e9 * overhead);

	// === PHASE 1: Initial execution (warm-up and individual measurements) ===
	if (!run_phase1())
	{	return 1;
	}

	// === PHASE 2: Multiple executions for statistical analysis ===
	if (!run_phase2())
	{	return 2;
	}

	vi_CurrentThreadAffinityRestore(); // Restore original thread affinity
	return 0;
}
