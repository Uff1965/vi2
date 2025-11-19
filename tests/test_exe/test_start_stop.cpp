#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

vi_tm::scoped_probe_t foo(VI_TM_HREG registry)
{	return vi_tm::scoped_probe_t::make_running(vi_tmRegistryGetMeas(registry, "start_stop_ext"));
}

TEST(scoped_probe_t, start_stop)
{
	// "When a reference is bound to a temporary object, the temporary object's lifetime is extended to match the lifetime of the reference
	const auto &&start_stop_ext = foo(VI_TM_HGLOBAL);
	EXPECT_TRUE(start_stop_ext.active());

	{	VI_TM("start_stop_VI_TM");
		auto start_stop = vi_tm::scoped_probe_t::make_paused(vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "start_stop")); // Paused.
		EXPECT_TRUE(start_stop.paused());

		start_stop.resume();
		EXPECT_TRUE(start_stop.active());

		std::this_thread::sleep_for(100ms);
		for (int n = 0; n < 5; n++)
		{
			start_stop.pause();
			EXPECT_TRUE(start_stop.paused());

			std::this_thread::sleep_for(100ms);
			start_stop.resume();
			EXPECT_TRUE(start_stop.active());
		}
		std::this_thread::sleep_for(100ms);
		start_stop.stop();
		EXPECT_TRUE(start_stop.idle());

		std::this_thread::sleep_for(100ms);
	}

	std::this_thread::sleep_for(100ms);
	EXPECT_TRUE(start_stop_ext.active());

	// start_stop -> ~200 ms
	// start_stop_VI_TM -> ~800 ms
	// start_stop_ext -> ~900 ms
}
