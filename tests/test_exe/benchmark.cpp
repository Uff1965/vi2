// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "benchmark.h"

#include <vi_timing/vi_timing.hpp>
#include <benchmark/benchmark.h>

#include <ctime> // for timespec_get
#include <chrono> // for std::chrono

VI_TM_TICK stdclock(void) noexcept
{	VI_TM_S("std::timespec_get");
	timespec ts;
	(void)std::timespec_get(&ts, TIME_UTC);
	return 1'000'000'000U * ts.tv_sec + ts.tv_nsec; //-V104
}

static void BM_timespec_get(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(stdclock());
    }
}
BENCHMARK(BM_timespec_get);

VI_TM_TICK stdchrono(void) noexcept {
	VI_TM_S("steady_clock");
	const auto tp = std::chrono::steady_clock::now().time_since_epoch();
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

static void BM_probe(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	auto m = vi_tmJournalGetMeas(VI_TM_HGLOBAL, "xxxx");
	for (auto _ : state)
	{	vi_tm::probe_t probe{ m };
	}
}
BENCHMARK(BM_probe);

static void BM_probe_ext(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	auto m = vi_tmJournalGetMeas(VI_TM_HGLOBAL, "xxxx");
	for (auto _ : state)
	{	vi_tm::probe_t probe{ m };
	}
}
BENCHMARK(BM_probe_ext);

static void BM_probe_ext_pr(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	auto m = vi_tmJournalGetMeas(VI_TM_HGLOBAL, "xxxx");
	vi_tm::probe_t probe{ std::true_type{}, m};
	for (auto _ : state)
	{	
		probe.resume();
		probe.pause();
	}
	probe.stop();
}
BENCHMARK(BM_probe_ext_pr);

static void BM_vi_tm(benchmark::State& state) {
	VI_TM_RESET("xxxx");
    for (auto _ : state) {
		auto m = vi_tmJournalGetMeas(VI_TM_HGLOBAL, "xxxx");
        const auto s = vi_tmGetTicks();
        const auto f = vi_tmGetTicks();
		vi_tmMeasurementAdd(m, f - s, 1U);
    }
}
BENCHMARK(BM_vi_tm);

static void BM_VI_TM(benchmark::State& state) {
	VI_TM_RESET("xxxx");
    for (auto _ : state) {
        VI_TM("xxxx");
    }
}
BENCHMARK(BM_VI_TM);

static void BM_vi_tm_S(benchmark::State& state) {
	VI_TM_RESET("xxxx");
    for (auto _ : state) {
		static auto m = vi_tmJournalGetMeas(VI_TM_HGLOBAL, "xxxx");
        const auto s = vi_tmGetTicks();
        const auto f = vi_tmGetTicks();
		vi_tmMeasurementAdd(m, f - s, 1U);
    }
}
BENCHMARK(BM_vi_tm_S);

static void BM_VI_TM_S(benchmark::State& state) {
	VI_TM_RESET("xxxx");
    for (auto _ : state) {
        VI_TM_S("xxxx");
    }
}
BENCHMARK(BM_VI_TM_S);

#if __ARM_ARCH >= 8 // ARMv8 (RaspberryPi4)
	VI_TM_TICK VI_TM_CALL RPi4(void) noexcept
	{	uint64_t result;
		asm volatile
		(	// too slow: "dmb ish\n\t" // Ensure all previous memory accesses are complete before reading the timer
			"isb\n\t" // Ensure the instruction stream is synchronized
			"mrs %0, cntvct_el0\n\t" // Read the current value of the system timer
			"isb\n\t" // Ensure the instruction stream is synchronized again
			: "=r"(result) // Output operand: result will hold the current timer value
			: // No input operands
			: "memory" // Clobber memory to ensure the compiler does not reorder instructions
		);
		return result;
	}
	
	static void BM_RPi4(benchmark::State& state) {
		for (auto _ : state) {
			benchmark::DoNotOptimize(RPi4());
		}
	}
	BENCHMARK(BM_RPi4);
#endif
