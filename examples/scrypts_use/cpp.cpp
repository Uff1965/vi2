#include "header.h"

#include <cassert>
#include <cstdio>

namespace cpp
{
	constexpr int VAL = 42;

	// "Script" worker function (native C++)
	VI_OPTIMIZE_OFF
	int callback()
	{	TM("0: CPP call");
		const char* message = "Hello, World!";
		if (!message || strcmp(message, "Hello, World!") != 0)
		{	assert(false);
			printf("CPP callback: unexpected string '%s'\n", message ? message : "<null>");
		}
		return VAL;
	}
	VI_OPTIMIZE_ON

	// Step 1: Initialize environment (dummy)
	bool init()
	{	TM("1: CPP Initialize");
		return true;
	}

	// Step 2: Load "script" (dummy)
	bool load_script()
	{	TM("2: CPP Load script");
		return true;
	}

	// Step 3: Call worker
	VI_OPTIMIZE_OFF
	bool call_worker()
	{	const int val = callback();
		assert(val == VAL);
		return val == VAL;
	}
	VI_OPTIMIZE_ON

	bool call()
	{
		{	TM("3.1: CPP First Call");
			if (!call_worker())
				return false;
		}

		for (int n = 0; n < 100; ++n)
		{	TM("3.2: CPP Other Call");
			if (!call_worker())
				return false;
		}

		return true;
	}

	// Step 4: Cleanup (dummy)
	void cleanup()
	{	TM("4: CPP Cleanup");
	}

	// Test entry
	bool test()
	{	TM("CPP test");

		bool result = false;
		if (init())
		{	if (load_script())
				result = call();

			cleanup();
		}

		return result;
	}

	const auto _ = register_test("Native", test);

} // namespace cpp
