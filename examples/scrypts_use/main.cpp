#include "header.h"

#include <vi_timing/vi_timing.h>

#include <map>

namespace
{
	std::map<std::string, test_func_t>& instance()
	{
		static std::map<std::string, test_func_t> instance;
		return instance;
	}
}

void register_test(std::string test_name, void (*test_func)())
{
	instance()[test_name] = test_func;
}

int main()
{	VI_TM_FUNC;
	VI_TM_GLOBALINIT("Header:\n", vi_tmDoNotReport, vi_tmHideHeader, "Done!\n");

	for (const auto& [name, func]: instance())
	{	func();
	}

	return 0;
}
