#include "header.h"
#include <vi_timing/vi_timing.h>

VI_TM(FILE_PATH);

namespace native_cpp
{
	// Step 1: Initialization
	// In native C++ there is no interpreter to initialize,
	// but we simulate the same stage for consistency.
	void init_cpp()
	{	VI_TM("C++(1) Initialize");
		// Example: allocate resources, prepare environment
	}

	// Step 2: Load/Compile
	// For native code, this step represents preparing functions or data.
	bool load_cpp()
	{	VI_TM("C++(2) Load and compile");
		// Example: simulate "compilation" or setup
		return true;
	}

	// Step 3: Call
	// Execute a native function directly.
	bool call_cpp()
	{	VI_TM("C++(3) Call");

		auto worker = []() -> int
		{	VI_TM("C++ worker");
			// Simulate some work
			return 42;
		};

		int result = worker();
		(void)result; // suppress unused warning
		return true;
	}

	// Step 4: Cleanup
	// Release resources if any.
	void cleanup_cpp()
	{	VI_TM("C++(4) Cleanup");
		// Example: free memory, close handles
	}

	// Main test function
	// Purpose: measure mandatory time costs of standard steps
	// when working with native C++ code (no interpreter).
	void test_cpp()
	{	VI_TM_FUNC;
		init_cpp();
		if (load_cpp())
			call_cpp();
		cleanup_cpp();
	}

	const auto _ = register_test(test_cpp);

} // namespace native_cpp
