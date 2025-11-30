#include "header.h"

#include <cassert>
#include <cstdio>

namespace cpp
{
	// "Script" worker function (native C++)
	int VI_NOINLINE callback(const char* message, int val)
	{	TM("0: CPP callback");
		if (const auto len = message ? strlen(message) : 0; len > 0)
		{	return message[((val - KEY) % len + len) % len];
		}
		assert(false);
		return -1;
	}

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

	VI_OPTIMIZE_OFF
	int call_worker(const char* msg, int n)
	{	return callback(msg, n + KEY);
	}
	VI_OPTIMIZE_ON

	// Step 3: Call worker
	bool call()
	{
		{	TM("3.1: CPP First Call");
			if (MSG[0] != call_worker(MSG, 0))
			{	assert(false);
				return false;
			}
		}

		for (int n = 0; n < 100; ++n)
		{	TM("3.2: CPP Other Call");
			if (MSG[n % (strlen(MSG))] != call_worker(MSG, n))
			{	assert(false);
				return false;
			}
		}

		return true;
	}

	// Step 4: Cleanup (dummy)
	void cleanup()
	{	TM("4: CPP Cleanup");
	}

	// Test entry
	bool test()
	{	TM("*CPP test");

		bool result = false;
		if (init())
		{	if (load_script())
				result = call();

			cleanup();
		}

		return result;
	}

	const auto _ = register_test("CPP", test);

} // namespace cpp
