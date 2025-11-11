// vi_timing_c_api_test.cpp
// GoogleTest unit tests for the C API declared in vi_timing_c.h
//
// Build: link with your vi_timing implementation and gtest libraries.
// Example (CMake): add executable that links gtest_main and your timing library.

#include <gtest/gtest.h>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <tuple>

#include <vi_timing/vi_timing.h>

// Helper: small RAII wrapper for vi_tmInit/vi_tmShutdown
struct ViTmInitGuard {
	ViTmInitGuard(const char* title = "Test report:\n") {
		vi_tmInit(title, vi_tmReportDefault, 0);
	}
	~ViTmInitGuard() {
		vi_tmShutdown();
	}
};

// Generate a unique measurement name using test name + counter
std::string make_name(const char* base) {
	static std::atomic<size_t> counter{0};
	return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

// Simple utility to fetch stats for a measurement handle
static vi_tmStats_t get_stats(VI_TM_HMEAS m) {
	vi_tmStats_t result{};
	vi_tmStatsReset(&result);
	vi_tmMeasurementGet(m, nullptr, &result);
	return result;
}

const vi_tmStats_t STAT_ZERO = []
	{	vi_tmStats_t result;
		vi_tmStatsReset(&result);
		return result;
	}();

bool operator==(const vi_tmStats_t& a, const vi_tmStats_t& b)
{
	return std::make_tuple
	(	a.calls_
#if VI_TM_STAT_USE_RAW
		, a.cnt_, a.sum_
#endif
#if VI_TM_STAT_USE_RMSE
		, a.flt_calls_, a.flt_cnt_, a.flt_avg_, a.flt_ss_
#endif
#if VI_TM_STAT_USE_MINMAX
		, a.min_, a.max_
#endif
	) ==
	std::make_tuple
	(	b.calls_
#if VI_TM_STAT_USE_RAW
		, b.cnt_, b.sum_
#endif
#if VI_TM_STAT_USE_RMSE
		, b.flt_calls_, b.flt_cnt_, b.flt_avg_, b.flt_ss_
#endif
#if VI_TM_STAT_USE_MINMAX
		, b.min_, b.max_
#endif
	);
}

// ---------------------------------------------------------------------------
// Basic API tests
// ---------------------------------------------------------------------------

TEST(ViTimingBasic, GetTicksIsMonotonic) {
	// vi_tmGetTicks should be monotonic (non-decreasing)
	VI_TM_TICK a = vi_tmGetTicks();
	// short sleep to allow tick to advance on systems with coarse resolution
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	VI_TM_TICK b = vi_tmGetTicks();
	EXPECT_LE(a, b);
}

TEST(ViTimingInitShutdown, InitShutdownSucceeds) {
	// Basic init/shutdown should succeed (no crash) and be idempotent-ish.
	EXPECT_EQ(VI_TM_RESULT(0), vi_tmInit(nullptr, vi_tmDoNotReport) );
	vi_tmShutdown();
	// Init again
	EXPECT_EQ(VI_TM_RESULT(0), vi_tmInit("init test2\n", vi_tmDoNotReport, 0) );
	vi_tmShutdown();
}

TEST(ViTimingJournal, CreateResetCloseAndGlobal) {
	ViTmInitGuard guard;
	// Create journal
	VI_TM_HREG j = vi_tmJournalCreate();
	ASSERT_NE(j, nullptr);
	// Create measurement handle in journal
	const auto meas_name = make_name("test_meas");
	VI_TM_HMEAS mh = vi_tmJournalGetMeas(j, meas_name.c_str());
	ASSERT_NE(mh, nullptr);
	// Reset journal (should not crash)
	vi_tmJournalReset(j);
	vi_tmStats_t s;
	vi_tmStatsReset(&s);
	const char *name = nullptr;
	vi_tmMeasurementGet(mh, &name, &s); // should still be valid
	EXPECT_STREQ(name, meas_name.c_str());
	EXPECT_EQ(s, STAT_ZERO);
	// Close journal
	vi_tmJournalClose(j);
	// Global journal handle constant should be defined
	EXPECT_EQ(VI_TM_HGLOBAL, VI_TM_HGLOBAL);
}

TEST(ViTimingStatsHelpers, StatsAddAndMergeReset) {
	ViTmInitGuard guard;

	vi_tmStats_t a{};
	vi_tmStatsReset(&a);
	vi_tmStatsAdd(&a, VI_TM_TDIFF{7}, VI_TM_SIZE{1});
	vi_tmStatsAdd(&a, VI_TM_TDIFF{3}, VI_TM_SIZE{2});

	vi_tmStats_t b{};
	vi_tmStatsReset(&b);
	vi_tmStatsAdd(&b, VI_TM_TDIFF{10}, VI_TM_SIZE{1});
	const vi_tmStats_t b_before = b;

	vi_tmStatsMerge(&a, &b);

	EXPECT_EQ(b, b_before);
	EXPECT_EQ(a.calls_, VI_TM_SIZE{ 3 });
	EXPECT_EQ(VI_TM_RESULT(0), vi_tmStatsIsValid(&a));
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(a.cnt_, VI_TM_SIZE{ 4 });
	EXPECT_EQ(a.sum_, VI_TM_TDIFF{20});
#endif

	// Merge/Reset should not crash
	vi_tmStatsReset(&a);
}

#if 0
TEST(ViTimingMeasurementResetMerge, MeasurementResetAndMerge) {
	ViTmInitGuard guard;
	const std::string n1 = make_name("m1");
	const std::string n2 = make_name("m2");
	VI_TM_HMEAS m1 = vi_tmJournalGetMeas(VI_TM_HGLOBAL, n1.c_str());
	VI_TM_HMEAS m2 = vi_tmJournalGetMeas(VI_TM_HGLOBAL, n2.c_str());
	ASSERT_NE(m1, nullptr);
	ASSERT_NE(m2, nullptr);

	// Add to both
	vi_tmMeasurementAdd(m1, VI_TM_TDIFF{4}, VI_TM_SIZE{1});
	vi_tmMeasurementAdd(m2, VI_TM_TDIFF{6}, VI_TM_SIZE{2});

	// Get stats
	vi_tmStats_t s1 = get_stats(m1);
	vi_tmStats_t s2 = get_stats(m2);

	// Merge s2 into m1 by using vi_tmMeasurementMerge (API provided)
	vi_tmMeasurementMerge(m1, &s2);

	// After merge, m1 stats should reflect non-zero values
	vi_tmStats_t s1_after = get_stats(m1);
#if VI_TM_STAT_USE_RAW
	EXPECT_GE(s1_after.cnt_, s1.cnt_);
	EXPECT_GE(s1_after.sum_, s1.sum_);
#endif

	// Reset m1 and ensure stats are valid and essentially zeroed or resettable
	vi_tmMeasurementReset(m1);
	vi_tmStats_t s1_reset = get_stats(m1);
#if VI_TM_STAT_USE_RAW
	EXPECT_EQ(s1_reset.cnt_, VI_TM_SIZE{0});
#endif
}

TEST(ViTimingJournalEnumerate, EnumerateMeasurements) {
	ViTmInitGuard guard;
	VI_TM_HREG j = vi_tmJournalCreate();
	ASSERT_NE(j, nullptr);

	const std::string n1 = make_name("enum1");
	const std::string n2 = make_name("enum2");

	VI_TM_HMEAS m1 = vi_tmJournalGetMeas(j, n1.c_str());
	VI_TM_HMEAS m2 = vi_tmJournalGetMeas(j, n2.c_str());
	ASSERT_NE(m1, nullptr);
	ASSERT_NE(m2, nullptr);

	// collect enumerated handles into a vector via callback
	struct Ctx { std::vector<VI_TM_HMEAS> v; };
	Ctx ctx;

	auto cb = [](VI_TM_HMEAS meas, void* c)->VI_TM_RESULT {
		auto ctx = static_cast<Ctx*>(c);
		ctx->v.push_back(meas);
		return 0;
	};

	// Call enumerate
	VI_TM_RESULT res = vi_tmJournalEnumerateMeas(j, cb, &ctx);
	EXPECT_EQ(res, VI_TM_RESULT(0));
	// Both created measurements should appear (order not guaranteed)
	EXPECT_GE(ctx.v.size(), size_t(2));

	vi_tmJournalClose(j);
}
#endif

TEST(ViTimingMeasurementAddAndGet, AddsAndAggregatesStats) {
	ViTmInitGuard guard;
	const std::string name = make_name("add_get");
	VI_TM_HMEAS m = vi_tmJournalGetMeas(VI_TM_HGLOBAL, name.c_str());
	ASSERT_NE(m, nullptr);

	clearerr(stdout); // harmless

	// Add several measurements
	vi_tmMeasurementAdd(m, VI_TM_TDIFF{10}, VI_TM_SIZE{1});
	vi_tmMeasurementAdd(m, VI_TM_TDIFF{20}, VI_TM_SIZE{2});
	vi_tmMeasurementAdd(m, VI_TM_TDIFF{5}, VI_TM_SIZE{1});

	// Query stats and validate aggregation fields that should be present
	vi_tmStats_t s = get_stats(m);

	EXPECT_EQ(s.calls_, (VI_TM_SIZE)3);

#if VI_TM_STAT_USE_RAW
	// raw fields available: cnt_ and sum_
	EXPECT_EQ(s.cnt_, VI_TM_SIZE{4});
	EXPECT_EQ(s.sum_, VI_TM_TDIFF{35});
#endif

#if VI_TM_STAT_USE_RMSE
	// RMSE-related fields should be present and consistent-ish
	EXPECT_FLOAT_EQ (s.flt_calls_, 3.0);
	EXPECT_FLOAT_EQ(s.flt_cnt_, 4.0);
#	if VI_TM_STAT_USE_FILTER
	EXPECT_FLOAT_EQ(s.flt_avg_, 8.75);
	EXPECT_FLOAT_EQ(s.flt_ss_, 18.75);
#	else
	EXPECT_FLOAT_EQ(s.flt_avg_, 10.0);
	EXPECT_FLOAT_EQ(s.flt_ss_, 0.0);
#	endif
#endif

#if VI_TM_STAT_USE_MINMAX
	EXPECT_FLOAT_EQ(s.min_, 5.0)
	EXPECT_FLOAT_EQ(s.max_, 10.0);
#endif
	// Stats validity helper
	EXPECT_EQ(VI_TM_RESULT(0), vi_tmStatsIsValid(&s));
}

// ---------------------------------------------------------------------------
// Notes:
// - These tests rely on the real implementation being linked. If you prefer
//   deterministic unit tests that don't depend on the real library, provide
//   controllable fakes for vi_tmGetTicks and vi_tmMeasurementAdd, or use the
//   DI approach described earlier.
// - Some expectations are guarded by compile-time flags in the header
//   (VI_TM_STAT_USE_RAW, VI_TM_STAT_USE_RMSE). Adjust asserts if your build
//   configuration differs.
// ---------------------------------------------------------------------------
