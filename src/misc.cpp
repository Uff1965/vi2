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

#ifdef _WIN32
#	include <Windows.h> // SetThreadAffinityMask
#	include <processthreadsapi.h> // GetCurrentProcessorNumber
#elif defined (__linux__)
#	include <pthread.h> // For pthread_setaffinity_np.
#	include <sched.h> // For sched_getcpu.
#endif

#include <algorithm>
#include <array>
#include <atomic> // for atomic_bool
#include <cassert>
#include <cerrno>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <optional>
#include <string_view>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace ch = std::chrono;
using namespace std::literals;

namespace
{
#if !defined(VI_TM_VERSION_MAJOR)
	constexpr auto VI_TM_VERSION_MAJOR = 0;
#endif
#if !defined(VI_TM_VERSION_MINOR)
	constexpr auto VI_TM_VERSION_MINOR = 0;
#endif
#if !defined(VI_TM_VERSION_PATCH)
	constexpr auto VI_TM_VERSION_PATCH = 0;
#endif
#if !defined(VI_TM_GIT_DESCRIBE)
	constexpr const char VI_TM_GIT_DESCRIBE[] = "";
#endif
#if !defined(VI_TM_GIT_COMMIT)
	constexpr const char VI_TM_GIT_COMMIT[] = "";
#endif
#if !defined(VI_TM_GIT_DATETIME)
	constexpr const char VI_TM_GIT_DATETIME[] = "";
#endif

	// Keeps the CPU payload for a short period to simulate workload.
	void payload()
	{	volatile auto f = 0.0;
		for (auto n = 10'000U; n; --n)
		{	f = (f + std::sin(n) * std::cos(n)) / 1.0001;
			std::atomic_thread_fence(std::memory_order_relaxed);
		}
	};

	namespace affinity
	{	// The affinity namespace provides platform-specific utilities for setting and restoring
        // the CPU affinity of the CURRENT thread. This ensures that timing measurements are not
        // affected by thread migration between CPU cores, which can introduce variability.

#if defined(_WIN32)
		// On Windows, thread_affinity_mask_t is defined as DWORD_PTR, which is used to represent the thread affinity mask.
		using thread_affinity_mask_t = DWORD_PTR;
		constexpr DWORD_PTR AFFINITY_ZERO = 0U;

		// The set_affinity() function sets the current thread's affinity to the processor it is currently running on.
		// If successful, the previous affinity mask is returned as an optional value; otherwise, an empty optional is returned.
		std::optional<DWORD_PTR> set_affinity()
		{	const auto current_affinity = static_cast<DWORD_PTR>(1U) << GetCurrentProcessorNumber();
			if (const auto ret = SetThreadAffinityMask(GetCurrentThread(), current_affinity); verify(0U != ret))
			{	return ret; // Ok!
			}
			return std::nullopt;
		}

		// The restore_affinity() function restores the thread's affinity to a previous mask.
		// The function returns true if the restoration succeeds, otherwise false.
		bool restore_affinity(DWORD_PTR prev)
		{	return (0U == prev || verify(0 != SetThreadAffinityMask(GetCurrentThread(), prev)));
		}
#elif defined(__linux__)
		// Define the thread affinity mask type for Linux as cpu_set_t.
		using thread_affinity_mask_t = cpu_set_t;
		const auto AFFINITY_ZERO = [] { cpu_set_t result; CPU_ZERO(&result); return result; }();
			
		// Sets the current thread's CPU affinity to the core it is currently running on.
		// Returns the previous affinity mask as an optional value if successful, otherwise returns an empty optional.
		std::optional<cpu_set_t> set_affinity()
		{	const auto current_thread = pthread_self();
			if (cpu_set_t prev{}; verify(0 == pthread_getaffinity_np(current_thread, sizeof(prev), &prev)))
			{	if (const auto current_core = sched_getcpu(); verify(current_core >= 0))
				{	cpu_set_t current_affinity;
					CPU_ZERO(&current_affinity);
					CPU_SET(current_core, &current_affinity);
					if (verify(0 == pthread_setaffinity_np(current_thread, sizeof(current_affinity), &current_affinity)))
					{	return prev; // Ok!
					}
				}
			}
			return std::nullopt;
		}

		// Restores the thread's CPU affinity to a previously saved mask.
		// Returns true if the restoration succeeds, otherwise false.
		bool restore_affinity(cpu_set_t prev)
		{	return CPU_EQUAL(&prev, &AFFINITY_ZERO) || verify(0 == pthread_setaffinity_np(pthread_self(), sizeof(prev), &prev));
		}
#endif

		class affinity_fix_t
		{	std::size_t cnt_ = 0U;
			thread_affinity_mask_t previous_affinity_ = AFFINITY_ZERO;
			static thread_local affinity_fix_t instance_;

			affinity_fix_t(const affinity_fix_t &) = delete;
			affinity_fix_t &operator=(const affinity_fix_t &) = delete;
			affinity_fix_t() = default;
			~affinity_fix_t()
			{	assert(0U == cnt_);
			}
		public:
			static int fixate()
			{	if (0 == instance_.cnt_++)
				{	const auto prev = set_affinity();
					if (!verify(!!prev))
					{	return VI_EXIT_FAILURE;
					}
					instance_.previous_affinity_ = prev.value_or(AFFINITY_ZERO);
				}
				return VI_EXIT_SUCCESS;
			}
			static int restore()
			{	assert(instance_.cnt_ > 0);
				if (0 == --instance_.cnt_)
				{	if(!verify(restore_affinity(instance_.previous_affinity_)))
					{	return VI_EXIT_FAILURE;
					}
					instance_.previous_affinity_ = AFFINITY_ZERO;
				}
				return VI_EXIT_SUCCESS;
			}
		};
		thread_local affinity_fix_t affinity_fix_t::instance_;

	} // namespace affinity

	namespace to_str
	{	// The to_str namespace provides utilities for converting floating-point values to strings with SI prefixes or scientific notation.
		constexpr auto GROUP_SIZE = 3;

		// Returns the non-negative remainder of v divided by GROUP_SIZE.
		// This is a "floor modulus" operation, ensuring the result is always in [0, GROUP_SIZE).
		constexpr int group_mod(int v) noexcept
		{	const auto m = v % GROUP_SIZE;
			return m < 0 ? m + GROUP_SIZE : m;
		}
		static_assert
			(	group_mod(3) == 0 && group_mod(2) == 2 && group_mod(1) == 1 && group_mod(0) == 0 &&
				group_mod(-1) == 2 && group_mod(-2) == 1 && group_mod(-3) == 0
			);

		// "Floor Division". Returns the group index for a given exponent value.
		// Groups are in steps of GROUP_SIZE (e.g., 3 for SI prefixes).
		constexpr int group_div(int v) noexcept
		{	return (v - group_mod(v)) / GROUP_SIZE;
		};
		static_assert(group_div(9) == 3 && group_div(2) == 0 && group_div(0) == 0 && group_div(-1) == -1 && group_div(-6) == -2);

		struct factors_t { int exp_; char suffix_[3]; };
		constexpr factors_t factors[]
		{	{ -30, " q" }, // quecto
			{ -27, " r" }, // ronto
			{ -24, " y" }, // yocto
			{ -21, " z" }, // zepto
			{ -18, " a" }, // atto
			{ -15, " f" }, // femto
			{ -12, " p" }, // pico
			{ -9, " n" }, // nano
			{ -6, " u" }, // micro
			{ -3, " m" }, // milli
			{ 0, "  " },
			{ 3, " k" }, // kilo
			{ 6, " M" }, // mega
			{ 9, " G" }, // giga
			{ 12, " T" }, // tera
			{ 15, " P" }, // peta
			{ 18, " E" }, // exa
			{ 21, " Z" }, // zetta
			{ 24, " Y" }, // yotta
			{ 27, " R" }, // ronna
			{ 30, " Q" }, // quetta
		};
		constexpr auto suffix_size = "e-308"sv.size() + 1U; // The SI suffix buffer must contain at least 6 characters to accommodate the longest suffix and null-termination (e.g., " k" or "e+308").
		static_assert(0 == factors[0].exp_ % GROUP_SIZE); // The first factor must be a multiple of GROUP_SIZE.
		static_assert(0 == factors[std::size(factors) / 2].exp_); // The middle factor must be zero.
		static_assert((factors[std::size(factors) - 1].exp_ - factors[0].exp_) == (GROUP_SIZE * (std::size(factors) - 1))); // The last factor must be GROUP_SIZE * (number of factors - 1) away from the first factor.
		static_assert(sizeof(factors_t::suffix_)/sizeof(factors_t::suffix_[0]) < suffix_size);

		// Returns the appropriate SI suffix for a given group position.
		// If the group position does not match a known SI prefix, returns scientific notation (e.g., "e6").
		std::string get_suffix(int group_pos)
		{	std::string result(suffix_size, '\0');
			if (const int idx = (group_pos - factors[0].exp_) / GROUP_SIZE; idx >= 0 && static_cast<std::size_t>(idx) < std::size(factors))
			{	assert(factors[idx].exp_ == group_pos);
				result = factors[idx].suffix_;
			}
			else
			{	std::snprintf(result.data(), result.size(), "e%d", group_pos);
				result.shrink_to_fit();
			}
			return result;
		}

		// Converts a floating-point value to a tuple containing a rounded value and its SI suffix.
		// - val_org: The original value to convert.
		// - sig_pos: The position of the most significant digit (zero-based).
		// - dec: The number of decimal places to display.
		// Returns: (rounded value, SI suffix or scientific notation).
		std::tuple<double, std::string> to_string_aux2(double val_org, int sig_pos, unsigned char const dec)
		{	assert(std::abs(val_org) >= DBL_MIN && sig_pos >= dec);

			auto val = std::abs(val_org);
			auto fact = static_cast<int>(std::floor(std::log10(val)));
			// Adjust sig_pos to align groupings for SI prefixes
			if (auto d = group_mod(sig_pos - dec) - group_mod(fact); d > 0)
			{	sig_pos -= d;
			}

			const auto rounded_f = fact - sig_pos;
			{	// Scale value to the correct significant digit position
				auto exp = -rounded_f;
				while (exp >= DBL_MAX_10_EXP)
				{	static const auto MAX10 = std::pow(10, DBL_MAX_10_EXP);
					val *= MAX10;
					exp -= DBL_MAX_10_EXP;
				}
				val *= std::pow(10, exp);
			}

			val = std::round(val);
			// If rounding increased the digit count, adjust fact
			if (auto fact_rounded = static_cast<int>(std::floor(std::log10(val))); fact_rounded != sig_pos)
			{	assert(fact_rounded == sig_pos + 1);
				++fact;
			}

			// Determine SI group position and scale value accordingly
			const auto group_pos = (group_div(fact) - group_div(sig_pos - dec)) * GROUP_SIZE;
			val *= std::pow(10, rounded_f - group_pos);
			assert(0 == errno);
			return { std::copysign(val, val_org), get_suffix(group_pos) };
		}

		// Converts a floating-point value to a formatted string with SI suffix or scientific notation.
		// - val_org: The original value to convert.
		// - sig: Number of significant digits to display.
		// - dec: Number of decimal places to display.
		// Returns: Formatted string representation of the value.
		std::string to_string_aux(double val_org, unsigned char sig, unsigned char const dec)
		{	assert(0 == errno);
			assert(sig > dec);
			const auto [val, suffix] = std::isnormal(val_org) ?
				to_string_aux2(val_org, sig - 1, dec) :
				std::make_tuple(+0.0, "  ");

			std::string result(sig + (9 + 1), '\0'); // 2.1 -> "-  2.2e-308" -> 9 + 2; 6.2 -> "-  6666.66e-308" -> 9 + 6;
			const auto len = static_cast<std::size_t>(std::snprintf(result.data(), result.size(), "%.*f%s", dec, val, suffix.data()));
			if (verify(result.size() > len)) // Error will be converted too long unsigned integer.
			{	result.resize(len);
			}
			else
			{	result = "ERR";
			}
			return result;
		}
	} // namespace to_str
} // namespace

// Converts a double value to a formatted string with SI prefix or scientific notation.
// - val: The value to convert.
// - significant: Number of significant digits to display (must be > decimal).
// - decimal: Number of decimal places to display.
// Returns: Formatted string representation, or "ERR" if arguments are invalid.
std::string misc::to_string(double val, unsigned char significant, unsigned char decimal)
{	if (!verify(decimal < significant))
	{	return "ERR";
	}
	if (std::isnan(val))
	{	return "NaN";
	}
	if (std::isinf(val))
	{	return std::signbit(val) ? "-INF" : "INF";
	}
	return to_str::to_string_aux(val, significant, decimal);
}

// Sets the current thread's CPU affinity to the processor it is currently running on.
// Returns VI_EXIT_SUCCESS on success, or VI_EXIT_FAILURE on failure.
int VI_TM_CALL vi_CurrentThreadAffinityFixate()
{	return affinity::affinity_fix_t::fixate();
}

// Restores the current thread's CPU affinity to its previous setting.
// Returns VI_EXIT_SUCCESS on success, or VI_EXIT_FAILURE on failure.
int VI_TM_CALL vi_CurrentThreadAffinityRestore()
{	return affinity::affinity_fix_t::restore();
}

// Yields execution of the current thread, allowing other threads to run.
void VI_TM_CALL vi_ThreadYield(void) noexcept
{	std::this_thread::yield();
}

// Keeps the CPU payload for a specified duration using multiple threads to simulate workload and warm up the system.
// - threads_qty: Number of threads to use (0 means use hardware concurrency).
// - ms: Duration in milliseconds to keep the CPU payload.
// Returns: VI_EXIT_SUCCESS on success, VI_EXIT_FAILURE on error.
int  VI_TM_CALL vi_WarmUp(unsigned int threads_qty, unsigned int ms)
{	if (0 == ms)
	{	return VI_EXIT_SUCCESS;
	}

	if (threads_qty == 0U || threads_qty > std::thread::hardware_concurrency())
	{	threads_qty = std::thread::hardware_concurrency();
	}
	if (threads_qty)
	{	threads_qty--;
	}

	try
	{	std::atomic_bool done = false;
		std::vector<std::thread> additional_threads;
		additional_threads.reserve(static_cast<std::size_t>(threads_qty));
		for (unsigned i = 0; i < threads_qty; ++i)
		{	additional_threads.emplace_back([&done] { while (!done) payload(); });
		}

		for (const auto stop = ch::steady_clock::now() + ch::milliseconds{ ms }; ch::steady_clock::now() < stop;)
		{	payload();
		}

		done = true;
		for (auto &t : additional_threads)
		{	if (t.joinable())
			{	t.join();
			}
		}
	}
	catch (...)
	{	assert(false);
		return VI_EXIT_FAILURE;
	}

	return VI_EXIT_SUCCESS;
}

// vi_tmStaticInfo: Returns static information about the vi_timing library based on the requested info type.
// - info: The type of information to retrieve (see vi_tmInfo_e).
// Returns: A pointer to the requested static data (type depends on info), or nullptr if the info type is not recognized.
const void* VI_TM_CALL vi_tmStaticInfo(VI_TM_FLAGS info)
{	using namespace misc;
	switch (info)
	{
		case vi_tmInfoVer: // Returns a pointer to the version number (unsigned).
		{	static constexpr unsigned ver = (VI_TM_VERSION_MAJOR * 1'000U + VI_TM_VERSION_MINOR) * 10'000U + VI_TM_VERSION_PATCH;
			return &ver;
		}

		case vi_tmInfoBuildNumber: // Returns a pointer to the build number (unsigned).
		{	static const unsigned build = build_number_get();
			return &build;
		}

		case vi_tmInfoVersion: // Returns a pointer to a static string containing the full version (major.minor.patch.buildType libraryType).
		{
#if VI_TM_DEBUG
			static constexpr auto CONFIG = 'D';
#else
			static constexpr auto CONFIG = 'R';
#endif
#if VI_TM_SHARED
			static constexpr auto TYPE = "shared"sv;
#else
			static constexpr auto TYPE = "static"sv;
#endif

			static const auto version = []
				{	static_assert(VI_TM_VERSION_MAJOR <= 99 && VI_TM_VERSION_MINOR <= 999 && VI_TM_VERSION_PATCH <= 9999);
					std::array<char, "99.999.9999.YYMMDDHHmm"sv.size() + 1U + " "sv.size() + TYPE.size() + 1U> result;
					[[maybe_unused]] const auto sz = snprintf
					(	result.data(),
						result.size(),
						"%u.%u.%u.%u%c %s",
						VI_TM_VERSION_MAJOR,
						VI_TM_VERSION_MINOR,
						VI_TM_VERSION_PATCH,
						build_number_get(),
						CONFIG,
						TYPE.data()
					);
					assert(0 < sz && static_cast<std::size_t>(sz) < result.size());
					return result;
				}();
			return version.data();
		}

		case vi_tmInfoGitDescribe: // Returns a pointer to the Git describe string (e.g., "v0.10.0-3-g96b37d4-dirty").
			return VI_TM_GIT_DESCRIBE;

		case vi_tmInfoGitCommit: // Returns a pointer to the Git commit hash (e.g., "96b37d49d235140e86f6f6c246bc7f166ab773aa").
			return VI_TM_GIT_COMMIT;

		case vi_tmInfoGitDateTime: // Returns a pointer to the Git commit date and time string (e.g., "2025-07-26 18:17:04 +0300").
			return VI_TM_GIT_DATETIME;

		case vi_tmInfoResolution: // Returns a pointer to the clock resolution in ticks (double).
		{	static const double resolution = properties_t::props().clock_resolution_ticks_;
			return &resolution;
		}

		case vi_tmInfoDuration: // Returns a pointer to the measure duration with cache in ticks (double).
		{	static const double duration = properties_t::props().duration_threadsafe_;
			return &duration;
		}

		case vi_tmInfoDurationEx: // Returns a pointer to the extended measure duration in ticks (double).
		{	static const double duration_ex = properties_t::props().duration_ex_threadsafe_;
			return &duration_ex;
		}

		case vi_tmInfoOverhead: // Returns a pointer to the clock overhead in ticks (double).
		{	static const double overhead = properties_t::props().clock_overhead_ticks_;
			return &overhead;
		}

		case vi_tmInfoSecPerUnit: // Returns a pointer to the seconds per tick (double).
		{	static const double unit = properties_t::props().seconds_per_tick_.count();
			return &unit;
		}

		case vi_tmInfoFlags:
		{
			static const unsigned flags = 0U
#if VI_TM_DEBUG
				| vi_tmDebug
#endif
#if VI_TM_SHARED
				| vi_tmShared
#endif
#if VI_TM_THREADSAFE
				| vi_tmThreadsafe
#endif
#if VI_TM_STAT_USE_RAW
				| vi_tmStatUseBase
#endif
#if VI_TM_STAT_USE_RMSE
				| vi_tmStatUseRMSE
#endif
#if VI_TM_STAT_USE_FILTER
				| vi_tmStatUseFilter
#endif
#if VI_TM_STAT_USE_MINMAX
				| vi_tmStatUseMinMax
#endif
				;
			return &flags; // Returns a pointer to the flags that control the library behavior.
		}

		default: // If the info type is not recognized, assert and return nullptr.
			static_assert(vi_tmInfoCount_ == 12, "Not all vi_tmInfo_e enum values are processed in the function vi_tmStaticInfo.");
			assert(false); // If we reach this point, the info type is not recognized.
			return nullptr;
	}
} // vi_tmStaticInfo(vi_tmInfo_e info)

VI_TM_SIZE VI_TM_CALL vi_tmF2A(char *buff, VI_TM_SIZE sz, double val, unsigned char sig, unsigned char dec)
{	auto str = misc::to_string(val, sig, dec);
	const auto result = str.size() + 1U;
	
	if (nullptr != buff && 0U != sz)
	{	if (result > sz)
		{	str.resize(sz - 1U); // Resize to fit the buffer, leaving space for null-termination.
		}

		std::memcpy(buff, str.data(), str.size() + 1);
	}

	return result; // Return the size of the string that was copied or the required size if it didn't fit.
}
