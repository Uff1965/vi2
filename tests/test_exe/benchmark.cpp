#include "benchmark.h"

#include <vi_timing/vi_timing.hpp>
#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <ctime> // for timespec_get
#include <chrono> // for std::chrono
#include <random>

const auto arr = []
	{	constexpr double mean = 100e6;
		constexpr double stddev = 0.15 * mean;
		constexpr unsigned N = 1'000;
		std::array<VI_TM_TDIFF, N> result;
		std::normal_distribution<> dist(mean, stddev);
		std::mt19937 gen; // gen(std::random_device{}());
		std::generate(result.begin(), result.end(), [&dist, &gen] { return static_cast<VI_TM_TDIFF>(dist(gen)); });
		return result;
	}();

static void BM_benchmark_ovh(benchmark::State &state)
{	static std::size_t n = 0;
	for (auto _ : state)
	{	auto tmp = arr[n++ % arr.size()];
		benchmark::DoNotOptimize(tmp);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_benchmark_ovh);

static void BM_timespec_get(benchmark::State& state)
{
	static auto cclock = +[]() noexcept ->VI_TM_TICK
		{	VI_TM_S("std::timespec_get");
			timespec ts;
			(void)std::timespec_get(&ts, TIME_UTC);
			return 1'000'000'000U * ts.tv_sec + ts.tv_nsec; //-V104
		};

    for (auto _ : state) {
        benchmark::DoNotOptimize(cclock());
		benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_timespec_get);

static void BM_steady_clock_now(benchmark::State& state)
{
	static auto now = +[]() noexcept->VI_TM_TICK
		{	VI_TM_S("steady_clock");
			const auto tp = std::chrono::steady_clock::now().time_since_epoch();
			return std::chrono::duration_cast<std::chrono::nanoseconds>(tp).count();
		};

    for (auto _ : state) {
        benchmark::DoNotOptimize(now());
		benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_steady_clock_now);

static void BM_vi_tmGetTicks(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(vi_tmGetTicks());
		benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_vi_tmGetTicks);

static void BM_vi_tmRegistryGetMeas(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	for (auto _ : state)
	{	auto m = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "xxxx");
		benchmark::DoNotOptimize(m);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_vi_tmRegistryGetMeas);

static void BM_vi_tmMeasurementAdd(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	static std::size_t n = 0;
	auto m = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "xxxx");
	for (auto _ : state)
	{	vi_tmMeasurementAdd(m, arr[n++ % arr.size()], 1);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_vi_tmMeasurementAdd);

static void BM_probe_make_paused(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	auto m = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "xxxx");
	for (auto _ : state)
	{	auto probe = vi_tm::scoped_probe_t::make_paused(m);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_probe_make_paused);

static void BM_probe_resume_pause(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	auto m = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "xxxx");
	auto probe = vi_tm::scoped_probe_t::make_paused(m);
	for (auto _ : state)
	{	
		probe.resume();
		probe.pause();
		benchmark::ClobberMemory();
	}
	probe.stop();
}
BENCHMARK(BM_probe_resume_pause);

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
			benchmark::ClobberMemory();
		}
	}
	BENCHMARK(BM_RPi4);
#endif

static void BM_VI_TM(benchmark::State& state) {
	VI_TM_RESET("xxxx");
    for (auto _ : state) {
        VI_TM("xxxx");
		benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_VI_TM);

static void BM_vi_tm(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	static std::size_t n = 0;
	for (auto _ : state)
	{
		const auto meas_ = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "xxxx");
		const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		benchmark::DoNotOptimize(finish - start);
		vi_tmMeasurementAdd(meas_, arr[n++ % arr.size()], 1);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_vi_tm);

static void BM_VI_TM_S(benchmark::State& state) {
	VI_TM_RESET("xxxx");
    for (auto _ : state) {
        VI_TM_S("xxxx");
		benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_VI_TM_S);

static void BM_vi_tm_S(benchmark::State &state)
{	VI_TM_RESET("xxxx");
	static std::size_t n = 0;

	for (auto _ : state)
	{
		static const auto meas_ = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "xxxx");
		const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		benchmark::DoNotOptimize(finish - start);
		vi_tmMeasurementAdd(meas_, arr[n++ % arr.size()], 1);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_vi_tm_S);
