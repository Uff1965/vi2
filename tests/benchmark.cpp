#include "benchmark.h"

#include <vi_timing/vi_timing.hpp>
#include <benchmark/benchmark.h>

#include <ctime> // for timespec_get
#include <chrono> // for std::chrono

VI_TM_TICK stdclock(void) noexcept
{	timespec ts;
	(void)std::timespec_get(&ts, TIME_UTC);
	return 1'000'000'000U * ts.tv_sec + ts.tv_nsec;
}

static void BM_timespec_get(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(stdclock());
    }
}
BENCHMARK(BM_timespec_get);

VI_TM_TICK stdchrono(void) noexcept
{	const auto tp = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp).count();
}

static void BM_steady_clock(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(stdchrono());
    }
}
BENCHMARK(BM_steady_clock);

static void BM_vi_tmGetTicks(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(vi_tmGetTicks());
    }
}
BENCHMARK(BM_vi_tmGetTicks);
