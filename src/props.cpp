// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "build_number_generator.h"
#include "misc.h"
#include <vi_timing/vi_timing.h>
#include <vi_timing/vi_timing_aux.h>

#include <algorithm> // For std::nth_element
#include <array>
#include <cassert>
#include <chrono> // For std::chrono::steady_clock, std::chrono::duration, std::chrono::milliseconds
#include <functional> // For std::invoke_result_t
#include <iterator>
#include <thread> // For std::this_thread::yield()
#include <utility> // For std::pair, std::index_sequence, std::make_index_sequence, std::forward, std::invoke

using namespace std::chrono_literals;
namespace ch = std::chrono;

namespace
{	constexpr auto now = ch::steady_clock::now;
	using time_point_t = std::invoke_result_t<decltype(now)>;

	constexpr auto CACHE_WARMUP = 6U;
	constexpr char SERVICE_NAME[] = "Bla-bla-bla-bla"; // A service item name for the journal (SSO size).
	constexpr std::array SANDBOX_NAMES
	{	"foo", "bar", "baz", "qux", "quux",
		"corge", "grault", "garply", "waldo", "fred",
		"plugh", "xyzzy", "thud", "hoge", "fuga",
	};

	auto start_tick()
	{	VI_TM_TICK result;
		const auto prev = vi_tmGetTicks();
		do
		{	result = vi_tmGetTicks();
		} while (prev == result); // Wait for the start of a new time interval.
		return result;
	}

	auto start_now()
	{	time_point_t result;
		const auto prev = now();
		do
		{	result = now();
		} while (prev == result); // Wait for the start of a new time interval.
		return result;
	}

	auto create_journal()
	{	std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)> result
		{	vi_tmJournalCreate(), &vi_tmJournalClose
		};

		if (auto j = result.get(); verify(!!j))
		{	(void)vi_tmJournalGetMeas(j, SERVICE_NAME);
			for (auto n : SANDBOX_NAMES)
			{	(void)vi_tmJournalGetMeas(j, n);
			}
		}
		return result;
	}

	template <typename F, typename... Args, std::size_t... Is>
	constexpr auto multiple_invoke_aux(std::index_sequence<Is...>, F *fn, Args&&... args)
	{	using return_t = std::invoke_result_t<F, Args...>;
		if constexpr (std::is_void_v<return_t>)
		{	((static_cast<void>(Is), std::invoke(fn, args...)), ...);
		}
		else
		{	volatile return_t results[sizeof...(Is)] {(static_cast<void>(Is), std::invoke(fn, args...))...};
			return results[sizeof...(Is) - 1U]; // Return the last result.
		}
	}

	// Invoke a F function N times without overhead costs for organizing the cycle.
	template <unsigned N, typename F, typename... Args>
	constexpr auto multiple_invoke(F *fn, Args&&... args)
	{	static_assert(N > 0);
		return multiple_invoke_aux<F, Args...>(std::make_index_sequence<N>{}, fn, std::forward<Args>(args)...);
	}

	template<typename C>
	auto median_part(C &c, std::size_t skip = 0U)
	{	assert(std::size(c) > skip);
		auto const b = std::next(std::begin(c) + skip);
		const auto n = std::size(c) - skip;
		auto mid = std::next(b, n / 2);
		std::nth_element(b, mid, std::end(c));

		return (n % 2) != 0 ? *mid : (*mid + *std::max_element(b, mid)) / 2;
	}

	template <unsigned N, typename F, typename... Args>
	double calc_duration_ticks(F *fn, Args&&... args)
	{	constexpr auto SIZE = 31U;
		std::array<VI_TM_TICK, SIZE + CACHE_WARMUP> diff;
		constexpr auto REPEAT = 512U;
		
		std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
		for (auto &d : diff)
		{	const auto s = start_tick();
			for (auto rpt = 0U; rpt < REPEAT; rpt++)
			{	multiple_invoke<N, F, Args...>(fn, args...);
			}
			const auto f = vi_tmGetTicks();
			d = f - s;
		}

		// First CACHE_WARMUP elements are for warming up the cache, so we ignore them.
		// Obtain the median value among the remaining ones.
		return static_cast<double>(median_part(diff, CACHE_WARMUP)) / static_cast<double>(REPEAT);
	}

	template <typename F, typename... Args>
	double calc_diff_ticks(F *fn, Args&&... args)
	{	constexpr auto BASE = 2U;
		constexpr auto EXTRA = 5U;
		const double full = calc_duration_ticks<BASE + EXTRA>(fn, args...);
		const double base = calc_duration_ticks<BASE>(fn, args...);
		return (full - base) / static_cast<double>(EXTRA);
	}

	void body_duration(VI_TM_HJOUR journal, const char* name)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		const auto h = vi_tmJournalGetMeas(journal, name);
		vi_tmMeasurementAdd(h, finish - start, 1U);
	};

	void body_measuring_with_caching(VI_TM_HMEAS m)
	{	const auto start = vi_tmGetTicks();
		const auto finish = vi_tmGetTicks();
		vi_tmMeasurementAdd(m, finish - start, 1U);
	};

	double meas_resolution()
	{	constexpr auto N = 8U;
		constexpr auto SIZE = 17U;
		std::array<VI_TM_TICK, SIZE + CACHE_WARMUP> arr;
		std::this_thread::yield(); // Reduce likelihood of thread interruption during measurement.
		for (auto &item : arr)
		{	const auto first = vi_tmGetTicks();
			auto last = first;
			for (auto cnt = N; cnt; )
			{	if (const auto current = vi_tmGetTicks(); current != last)
				{	last = current;
					--cnt;
				}
			}
			item = last - first;
		}
		// First CACHE_WARMUP elements are for warming up the cache, so we ignore them.
		return static_cast<double>(median_part(arr, CACHE_WARMUP)) / static_cast<double>(N);
	}

	auto meas_seconds_per_tick()
	{	time_point_t c_time;
		VI_TM_TICK c_ticks;
		auto const s_time = start_now();
		auto const s_ticks = vi_tmGetTicks();
		auto const stop = s_time + 10ms;
		do
		{	c_time = start_now();
			c_ticks = vi_tmGetTicks();
		}
		while (c_time < stop || c_ticks - s_ticks < 10);

		return ch::duration<double>{ c_time - s_time } / (c_ticks - s_ticks);
	}

	auto meas_cost_calling_tick_function()
	{	return calc_diff_ticks(vi_tmGetTicks);
	}

	auto meas_duration_with_caching()
	{	double result{};
		if (const auto journal = create_journal(); verify(!!journal))
		{	if (const auto m = vi_tmJournalGetMeas(journal.get(), SERVICE_NAME); verify(!!m))
			{	result = calc_diff_ticks(body_measuring_with_caching, m);
			}
		}
		return result;
	}

	auto meas_duration()
	{	auto journal = create_journal();
		return (verify(!!journal)) ? calc_diff_ticks(body_duration, journal.get(), SERVICE_NAME) : 0.0;
	}
} // namespace

const misc::properties_t&
misc::properties_t::props()
{	static const properties_t self;
	return self;
}

misc::properties_t::properties_t()
{
	struct affinity_guard_t // RAII guard to fixate the current thread's affinity.
	{	affinity_guard_t() { vi_CurrentThreadAffinityFixate(); }
		~affinity_guard_t() { vi_CurrentThreadAffinityRestore(); }
	} affinity_guard; // Fixate the current thread's affinity to avoid issues with clock resolution measurement.

	vi_WarmUp(1, 500);

	clock_resolution_ticks_ = meas_resolution(); // The resolution of the clock in ticks.
	seconds_per_tick_ = meas_seconds_per_tick(); // The duration of a single tick in seconds.
	clock_overhead_ticks_ = meas_cost_calling_tick_function(); // The cost of a single call of vi_tmGetTicks.
	duration_threadsafe_ = meas_duration_with_caching(); // The cost of a single measurement with preservation in ticks.
	duration_ex_threadsafe_ = meas_duration(); // The cost of a single measurement in ticks.
}
