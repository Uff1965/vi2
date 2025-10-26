/*****************************************************************************\
* This file is part of the vi_timing library.
* 
* vi_timing - a compact, lightweight C/C++ library for measuring code 
* execution time. It was developed for experimental and educational purposes, 
* so please keep expectations reasonable.
*
* Report bugs or suggest improvements to author: <programmer.amateur@proton.me>
*
* LICENSE & DISCLAIMER:
* - No warranties. Use at your own risk.
* - Licensed under Business Source License 1.1 (BSL-1.1):
*   - Free for non-commercial use.
*   - For commercial licensing, contact the author.
*   - Change Date: 2029-09-01 - after which the library will be licensed 
*     under GNU GPLv3.
*   - Attribution required: "vi_timing Library (c) A.Prograamar".
*   - See LICENSE in the project root for full terms.
\*****************************************************************************/

// probe_test.cpp
// GoogleTest unit tests for vi_tm::probe_t using the real vi_timing C header types.
// The production timing API (vi_tmGetTicks, vi_tmMeasurementAdd) is replaced
// by test implementations defined here so tests can control time and observe
// recorded measurements.
//
// Build instructions:
// - Link with your probe implementation object that includes vi_tm::probe_t.
// - Compile this file and link with GoogleTest.
// - Ensure this translation unit is linked into the same executable so the
//   test definitions override the production symbols.

#include <gtest/gtest.h>
#include <atomic>
#include <limits>
#include <utility>

#define probe_t probe_fake_t
#define vi_tmGetTicks vi_tmGetTicks_fake
#define vi_tmMeasurementAdd vi_tmMeasurementAdd_fake
#include <vi_timing/vi_timing.hpp>

// ---------------------------------------------------------------------------
// Test fakes for the timing API
// ---------------------------------------------------------------------------

namespace
{
	// A simple test handle value used as the measurement handle in tests.
	VI_TM_HMEAS const TEST_MEAS = reinterpret_cast<VI_TM_HMEAS>(static_cast<std::uintptr_t>(0x1234));
	VI_TM_HMEAS const UNDEF_MEAS = reinterpret_cast<VI_TM_HMEAS>(static_cast<std::uintptr_t>(0xBAADF00D));
	VI_TM_TDIFF constexpr UNDEF_DIFF = VI_TM_TDIFF{ 0xBAADF00D };
	VI_TM_SIZE constexpr UNDEF_SIZE = VI_TM_SIZE{ 0xBAADF00D };

	// Controlled tick counter used by tests. Use plain atomic to be safe across threads.
	VI_TM_TICK g_ticks{ VI_TM_TICK{ 0 } };
	// Helpers to manipulate the fake tick source from tests.
	void set_ticks(VI_TM_TICK t) noexcept { g_ticks = t; }
	void advance_ticks(VI_TM_TDIFF delta) noexcept { g_ticks += delta; }

	// Last measurement observed by the fake vi_tmMeasurementAdd_fake.
	// Stored as atomic-friendly integer / pointer types.
	VI_TM_HMEAS g_last_meas = UNDEF_MEAS;
	VI_TM_TDIFF g_last_dur{ UNDEF_DIFF };
	VI_TM_SIZE g_last_cnt{ UNDEF_SIZE };

	// Clear recorded measurement state between tests.
	void clear_last_measurement() noexcept
	{	g_last_meas = UNDEF_MEAS;
		g_last_dur = UNDEF_DIFF;
		g_last_cnt = UNDEF_SIZE;
	}

	// ---------------------------------------------------------------------------
	// Test fixture
	// ---------------------------------------------------------------------------

	struct ProbeTest : ::testing::Test {
		void SetUp() override {
			clear_last_measurement();
			set_ticks(VI_TM_TICK{0});
		}
		void TearDown() override {
			clear_last_measurement();
		}
	};
}

// Test implementations of the C API functions.
// The signatures must match exactly the declarations in vi_timing_c.h.
// The linker will prefer these definitions in the test binary.
#pragma warning(suppress: 4273)
VI_TM_TICK VI_TM_CALL vi_tmGetTicks_fake(void) VI_NOEXCEPT
{	return g_ticks;
}

#pragma warning(suppress: 4273)
void VI_TM_CALL vi_tmMeasurementAdd_fake(VI_TM_HMEAS m, VI_TM_TDIFF dur, VI_TM_SIZE cnt) VI_NOEXCEPT
{	// Record the values for assertions in tests.
	g_last_meas = m;
	g_last_dur = dur;
	g_last_cnt = cnt;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(ProbeTest, StartStopRecordsDurationAndCount) {
	// Start at tick 100, then advance by 50 and stop.
	set_ticks(VI_TM_TICK{100});
	auto p = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{3});
	EXPECT_TRUE(p.active());
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{0});
	advance_ticks(VI_TM_TDIFF{50}); // ticks == 150
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{50});
	p.stop();
	EXPECT_TRUE(p.idle());
	EXPECT_EQ(g_last_meas, TEST_MEAS);
	EXPECT_EQ(g_last_dur, VI_TM_TDIFF{50});
	EXPECT_EQ(g_last_cnt, VI_TM_SIZE{3});

	// Stopping again must be a no-op.
	clear_last_measurement();
	p.stop();
	EXPECT_TRUE(p.idle());
	EXPECT_EQ(g_last_meas, UNDEF_MEAS);
	EXPECT_EQ(g_last_dur, UNDEF_DIFF);
	EXPECT_EQ(g_last_cnt, UNDEF_SIZE);
}
TEST_F(ProbeTest, PauseAccumulatesAndResumeContinues) {
	// Run, pause, resume, and stop; total duration should be sum of parts.
	set_ticks(VI_TM_TICK{10});
	auto p = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{2});
	advance_ticks(VI_TM_TDIFF{7}); // running for 7
	p.pause();
	EXPECT_TRUE(p.paused());
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{7});

	// Advance time while paused (should not count), then resume and add 5 more ticks.
	advance_ticks(VI_TM_TDIFF{3}); // tick source jumps while paused
	p.resume();
	EXPECT_TRUE(p.active());
	EXPECT_EQ(p.elapsed(), 7);
	advance_ticks(VI_TM_TDIFF{5});
	p.stop();
	EXPECT_TRUE(p.idle());
	EXPECT_EQ(g_last_meas, TEST_MEAS);
	EXPECT_EQ(g_last_dur, VI_TM_TDIFF{12}); // 7 + 5
	EXPECT_EQ(g_last_cnt, VI_TM_SIZE{2});
}
TEST_F(ProbeTest, PausedStopRecordsAccumulated) {
	// Pause without resume, then stop should record the accumulated time.
	set_ticks(VI_TM_TICK{0});
	auto p = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{1});
	advance_ticks(VI_TM_TDIFF{8});
	p.pause();
	p.stop();
	EXPECT_EQ(g_last_meas, TEST_MEAS);
	EXPECT_EQ(g_last_dur, VI_TM_TDIFF{8});
	EXPECT_EQ(g_last_cnt, VI_TM_SIZE{1});
}

TEST_F(ProbeTest, ElapsedReflectsRunningPausedIdle) {
	// Validate elapsed() in running, paused and after stop (idle).
	set_ticks(VI_TM_TICK{100});
	auto p = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{1});
	advance_ticks(VI_TM_TDIFF{4});
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{4});

	p.pause();
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{4});

	p.resume();
	advance_ticks(VI_TM_TDIFF{6});
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{10});

	p.stop();
	EXPECT_EQ(p.elapsed(), VI_TM_TDIFF{0});
}

TEST_F(ProbeTest, MoveConstructionTransfersOwnershipAndDoesNotDoubleRecord) {
	// Move-constructing from a running probe transfers responsibility to the new object.
	set_ticks(VI_TM_TICK{0});
	auto a = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{2});
	advance_ticks(VI_TM_TDIFF{5});
	auto b = std::move(a);
	EXPECT_TRUE(b.active());
	EXPECT_TRUE(a.idle());
	// moved-from 'a' should be idle; calling stop() on it should be harmless.
	a.stop();

	// Stopping 'b' should record a single measurement.
	b.stop();
	EXPECT_EQ(g_last_meas, TEST_MEAS);
	EXPECT_EQ(g_last_dur, VI_TM_TDIFF{5});
	EXPECT_EQ(g_last_cnt, VI_TM_SIZE{2});
}

TEST_F(ProbeTest, MoveAssignmentStopsTargetBeforeOverwrite) {
	// operator= should stop the target before overwriting it, so previous target's
	// measurement is recorded exactly once.
	set_ticks(VI_TM_TICK{0});
	auto a = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{1});
	advance_ticks(VI_TM_TDIFF{3}); // 'a' has run for 3
	auto b = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{2});
	advance_ticks(VI_TM_TDIFF{7}); // 'b' has run for 10 total

	// Move-assign 'a' into 'b'. operator= is expected to call stop() on the existing 'b'.
	b = std::move(a);

	// The measurement recorded by the stop inside operator= must reflect the previous target ('b').
	EXPECT_EQ(g_last_cnt, VI_TM_SIZE{2});
	EXPECT_EQ(g_last_dur, VI_TM_TDIFF{7});
	EXPECT_NE(g_last_meas, nullptr);
	EXPECT_TRUE(b.active());
	EXPECT_TRUE(a.idle());

	// Clear recorded measurement and stop the new 'b' (which now holds 'a' state).
	clear_last_measurement();
	advance_ticks(VI_TM_TDIFF{1}); // advance so duration increases from 3 to 4
	b.stop();

	EXPECT_EQ(g_last_meas, TEST_MEAS);
	EXPECT_EQ(g_last_dur, VI_TM_TDIFF{11});
	EXPECT_EQ(g_last_cnt, VI_TM_SIZE{1});
}

#ifndef NDEBUG
TEST_F(ProbeTest, DoublePauseTriggersDebugAssert) {
	assert(0 == errno);
//	vi_tmInit(nullptr, vi_tmDoNotReport);
	// Debug-only assert behavior. Adjust EXPECT_DEATH regex if your runtime prints something specific.
	auto p = vi_tm::probe_t::make_running(TEST_MEAS, VI_TM_SIZE{1});
	p.pause();
	EXPECT_TRUE(p.paused());
	EXPECT_DEATH({ p.pause(); }, ".*");
	p.resume();
	EXPECT_TRUE(p.active());
	EXPECT_DEATH({ p.resume(); }, ".*");
//    vi_tmShutdown();
	errno = 0;
}
#endif

TEST_F(ProbeTest, StopOnIdleDoesNotRecord) {
	// Ensure calling stop() on an idle/moved-from object does not record a new measurement.
	auto p = vi_tm::probe_t::make_paused(TEST_MEAS, VI_TM_SIZE{1});
	EXPECT_TRUE(p.paused());
	// First stop may record accumulated zero depending on paused ctor semantics.
	p.stop();
	clear_last_measurement();
	// Subsequent stop must be a no-op.
	p.stop();
	EXPECT_EQ(g_last_meas, UNDEF_MEAS);
}
