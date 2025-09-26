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

#include "build_number_generator.h" // For build number generation.
#include "misc.h"
#include <vi_timing/vi_timing.h>

#include <algorithm> // std::min_element, std::max_element
#include <cassert> // assert()
#include <chrono> // std::chrono::milliseconds
#include <cmath> // std::sqrt
#include <cstdint> // std::uint64_t, std::size_t
#include <cstring>
#include <memory> // std::unique_ptr
#include <mutex> // std::mutex, std::lock_guard
#include <new>
#include <numeric> // std::accumulate
#include <string> // std::string
#include <unordered_map> // unordered_map: "does not invalidate pointers or references to elements".
#include <utility>

#if !VI_TM_STAT_USE_RMSE && VI_TM_STAT_USE_FILTER
#	error "The filter is only available when RMSE is enabled."
#endif

#if VI_TM_THREADSAFE
#	ifdef __STDC_NO_ATOMICS__
		//	At the moment Atomics are available in Visual Studio 2022 with the /experimental:c11atomics flag.
		//	"we left out support for some C11 optional features such as atomics" [Microsoft
		//	https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2]
#		error "Atomic objects and the atomic operation library are not supported."
#	endif

#	include <atomic> // std::atomic
#	include <thread> // std::this_thread::yield()

// define CPU_RELAX for adaptive_mutex_t.
#	if defined(_MSC_VER)
#		include <intrin.h>
#		define CPU_RELAX() _mm_pause()
#	elif defined(__i386__) || defined(__x86_64__)
#		include <immintrin.h>
#		define CPU_RELAX() _mm_pause() // __builtin_ia32_pause()
#	elif defined(__arm__) || defined(__aarch64__)
#		define CPU_RELAX() __asm__ __volatile__("yield") // For ARM architectures: use yield hint
#	else
#		define CPU_RELAX() std::this_thread::yield() // Fallback
#	endif

namespace
{
	// A mutex optimized for short captures, using spin-waiting and yielding to reduce contention.
	// This mutex is designed to be used in scenarios where the lock is held for a very short time,
	// minimizing the overhead of locking and unlocking.
	class adaptive_mutex_t
	{	std::atomic_flag locked_ = ATOMIC_FLAG_INIT;
	public:
		adaptive_mutex_t() noexcept = default;
		adaptive_mutex_t(const adaptive_mutex_t&) = delete;
		adaptive_mutex_t& operator=(const adaptive_mutex_t&) = delete;

		void lock() noexcept
		{	constexpr unsigned SPIN_LIMIT = 50;
			constexpr unsigned YIELD_LIMIT = 100;

			for(unsigned spins = 0; locked_.test_and_set(std::memory_order_acquire); ++spins)
			{	if (spins < SPIN_LIMIT)
				{	CPU_RELAX(); // Spin-wait with a CPU relaxation hint.
				}
				else if (spins < SPIN_LIMIT + YIELD_LIMIT)
				{	std::this_thread::yield(); // Yield the thread to allow other threads to run.
				}
				else
				{	const auto sleep_ms = 1U << std::min(spins - (SPIN_LIMIT + YIELD_LIMIT), 5U); // exponential back-off
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
				}
			}
		}
		void unlock() noexcept { locked_.clear(std::memory_order_release); }
		bool try_lock() noexcept { return !locked_.test_and_set(std::memory_order_acquire); }
	};
}
#	define VI_TM_THREADSAFE_ONLY(t) t
#else
#	define VI_TM_THREADSAFE_ONLY(t)
#endif

// Checking for FMA support by the compiler/platform
#if defined(__FMA__) || (defined(_MSC_VER) && (defined(__AVX2__) || defined(__AVX512F__)))
	#define FMA(x, y, z) std::fma((x), (y), (z))
#else
	#define FMA(x, y, z) ((x) * (y) + (z))
#endif

namespace
{
	using fp_limits_t = std::numeric_limits<VI_TM_FP>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr auto fp_ONE = static_cast<VI_TM_FP>(1);

#ifdef __cpp_lib_hardware_interference_size
	using std::hardware_constructive_interference_size;
#else
	// Fallback for C++ versions that do not support std::hardware_constructive_interference_size.
	// __cpp_lib_hardware_interference_size support was added in GCC v12.1 (Ubuntu 22.10 and Debian 12).
	// Default cache line size for x86-64 architecture, typically 64 bytes.
	constexpr std::size_t hardware_constructive_interference_size = 64;
#endif

	/// <summary>
	/// meterage_t is a class for collecting and managing timing measurement statistics.
	/// </summary>
	/// <remarks>
	/// This class encapsulates a vi_tmMeasurementStats_t structure, which stores statistics such as
	/// the number of calls, total and filtered event counts, sum of durations, minimum/maximum/average
	/// values, and variance.
	/// <para>
	/// Note: If the corresponding preprocessor macros (VI_TM_STAT_USE_RAW, VI_TM_STAT_USE_RMSE, VI_TM_STAT_USE_MINMAX) are defined and set to a TRUE value only.
	/// </para>
	/// </remarks>
	/// <para>Main features:</para>
	/// <list type="bullet">
	///   <item><description>add()    : Add a new measurement.</description></item>
	///   <item><description>merge()  : Merge statistics from another structure.</description></item>
	///   <item><description>get()    : Get the current statistics.</description></item>
	///   <item><description>reset()  : Reset all statistics.</description></item>
	/// </list>
	/// <para>
	/// <b>Thread safety:</b> All methods are thread-safe only if the macro <c>VI_TM_THREADSAFE</c> is defined and set to a nonzero value.
	/// In this case, access to the data is protected by an adaptive mutex.
	/// The class is aligned to the hardware cache line size to minimize false sharing in multithreaded scenarios.
	/// </para>
	class alignas(hardware_constructive_interference_size) meterage_t
	{	static_assert(std::is_standard_layout_v<vi_tmMeasurementStats_t>); // Ensure standard layout for compatibility with C.
		vi_tmMeasurementStats_t stats_;
		VI_TM_THREADSAFE_ONLY(mutable adaptive_mutex_t mtx_);
	public:
		meterage_t() noexcept { vi_tmStatsReset(&stats_); }
		void add(VI_TM_TDIFF val, VI_TM_SIZE cnt) noexcept;
		void merge(const vi_tmMeasurementStats_t &src) noexcept;
		vi_tmMeasurementStats_t get() const noexcept;
		void reset() noexcept;
	};

	using storage_t = std::unordered_map<std::string, meterage_t>;
	using jrn_finalizer_ctx_t = void*;
	using jrn_finalizer_fn_t = int(*)(vi_tmMeasurementsJournal_t*, jrn_finalizer_ctx_t);
	using jrn_finalizer_t = std::pair<jrn_finalizer_fn_t, jrn_finalizer_ctx_t>;
}

// 'vi_tmMeasurement_t' is simply an alias for a measurement entry in the storage map.
// It inherits from 'storage_t::value_type', which is typically 'std::pair<const std::string, meterage_t>'.
struct vi_tmMeasurement_t: storage_t::value_type {/**/};
static_assert
	(	sizeof(vi_tmMeasurement_t) == sizeof(storage_t::value_type) &&
		alignof(vi_tmMeasurement_t) == alignof(storage_t::value_type),
		"'vi_tmMeasurement_t' should simply be a synonym for 'storage_t::value_type'."
	);

struct vi_tmMeasurementsJournal_t
{
private:
	static constexpr auto MAX_LOAD_FACTOR = 0.7F;
	static constexpr size_t DEFAULT_STORAGE_CAPACITY = 64U;
protected:
	storage_t storage_;
	VI_TM_THREADSAFE_ONLY(mutable adaptive_mutex_t storage_guard_);
public:
	vi_tmMeasurementsJournal_t(const vi_tmMeasurementsJournal_t &) = delete;
	vi_tmMeasurementsJournal_t& operator=(const vi_tmMeasurementsJournal_t &) = delete;
	vi_tmMeasurementsJournal_t();
	~vi_tmMeasurementsJournal_t() = default;
	auto& try_emplace(const char *name); // Get a reference to the measurement by name, creating it if it does not exist.
	int for_each_measurement(vi_tmMeasEnumCb_t fn, void *ctx); // Calls the function fn for each measurement in the journal, while this function returns 0. Returns the return code of the function fn if it returned a nonzero value, or 0 if all measurements were processed.
};

vi_tmMeasurementsJournal_t::vi_tmMeasurementsJournal_t()
{	storage_.max_load_factor(MAX_LOAD_FACTOR);
	storage_.reserve(DEFAULT_STORAGE_CAPACITY);
}

inline void meterage_t::reset() noexcept
{	VI_TM_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmStatsReset(&stats_);
}

inline void meterage_t::add(VI_TM_TDIFF v, VI_TM_SIZE n) noexcept
{	VI_TM_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmStatsAdd(&stats_, v, n);
}

inline void meterage_t::merge(const vi_tmMeasurementStats_t &src) noexcept
{	VI_TM_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	vi_tmStatsMerge(&stats_, &src);
}

inline vi_tmMeasurementStats_t meterage_t::get() const noexcept
{	VI_TM_THREADSAFE_ONLY(std::lock_guard lg(mtx_));
	return stats_;
}

inline auto& vi_tmMeasurementsJournal_t::try_emplace(const char *name)
{	assert(name);
	VI_TM_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	auto [iterator, _] = storage_.try_emplace(name);
	return *iterator;
}

int vi_tmMeasurementsJournal_t::for_each_measurement(vi_tmMeasEnumCb_t fn, void *ctx)
{	VI_TM_THREADSAFE_ONLY(std::lock_guard lock{ storage_guard_ });
	assert(fn);
	for (auto &it : storage_)
	{	if (!it.first.empty())
		{	if (const auto breaker = fn(static_cast<VI_TM_HMEAS>(&it), ctx))
			{	return breaker;
			}
		}
	}
	return 0;
}

//vvvv API Implementation vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

VI_TM_RESULT VI_TM_CALL vi_tmStatsIsValid(const vi_tmMeasurementStats_t *meas) noexcept
{	if(!verify(!!meas))
	{	return VI_EXIT_FAILURE;
	}

#if VI_TM_STAT_USE_RAW
	if (!verify((0U != meas->cnt_) == (0U != meas->calls_))) return VI_EXIT_FAILURE; // cnt_ and calls_ must be both zero or both non-zero.
	if (!verify(meas->cnt_ >= meas->calls_)) return VI_EXIT_FAILURE; // cnt_ must be greater than or equal to calls_.
#endif

#if VI_TM_STAT_USE_MINMAX
	if (meas->calls_ == 0U)
	{	if (!verify(meas->min_ == fp_limits_t::infinity())) return VI_EXIT_FAILURE; // If calls_ is zero, min_ must be infinity.
		if (!verify(meas->max_ == -fp_limits_t::infinity())) return VI_EXIT_FAILURE; // If calls_ is zero, max_ must be negative infinity. 
	}
	else
	{	if (!verify(meas->min_ != fp_limits_t::infinity())) return VI_EXIT_FAILURE; // min_ must not be infinity.
		if (meas->calls_ == 1U)
		{	if (!verify(meas->min_ == meas->max_)) return VI_EXIT_FAILURE; // If there is only one call, min_ and max_ must be equal.
		}
		else
		{	if (!verify(meas->min_ <= meas->max_)) return VI_EXIT_FAILURE; // min_ must be less than or equal to max_.
		}
	}
#endif

#if VI_TM_STAT_USE_RAW && VI_TM_STAT_USE_MINMAX
	if (meas->calls_ == 1U && !verify(static_cast<VI_TM_FP>(meas->sum_) == meas->min_ * meas->cnt_)) return VI_EXIT_FAILURE; // If there is only one call, sum_ must equal min_ multiplied by cnt_.
	if (meas->calls_ >= 1U && !verify(static_cast<VI_TM_FP>(meas->sum_) >= meas->max_)) return VI_EXIT_FAILURE; // sum_ must be greater than or equal to max_.
#endif

#if VI_TM_STAT_USE_RMSE
	if (!verify(meas->flt_calls_ <= meas->calls_)) return VI_EXIT_FAILURE; // flt_calls_ must be less than or equal to calls_.
	if (!verify((fp_ZERO != meas->flt_cnt_) == (0U != meas->flt_calls_))) return VI_EXIT_FAILURE; // flt_cnt_ and flt_calls_ must be both zero or both non-zero.
	if (!verify(meas->flt_cnt_ >= static_cast<VI_TM_FP>(meas->flt_calls_))) return VI_EXIT_FAILURE; // flt_cnt_ must be greater than or equal to flt_calls_.
	if (VI_TM_FP _; !verify(std::modf(meas->flt_cnt_, &_) == fp_ZERO)) return VI_EXIT_FAILURE; // flt_cnt_ must be an integer value.
	if (!verify(meas->flt_avg_ >= fp_ZERO)) return VI_EXIT_FAILURE; // flt_avg_ must be non-negative.
	if (!verify(meas->flt_ss_ >= fp_ZERO)) return VI_EXIT_FAILURE; // flt_ss_ must be non-negative.

	if (meas->flt_cnt_ == fp_ZERO)
	{	if (!verify(meas->flt_avg_ == fp_ZERO)) return VI_EXIT_FAILURE; // If flt_cnt_ is zero, flt_avg_ must also be zero.
		if (!verify(meas->flt_ss_ == fp_ZERO)) return VI_EXIT_FAILURE; // If flt_cnt_ is zero, flt_ss_ must also be zero.
	}
	else if (meas->flt_cnt_ == fp_ONE)
	{	if (!verify(meas->flt_ss_ == fp_ZERO)) return VI_EXIT_FAILURE;
	}
#endif

#if VI_TM_STAT_USE_RAW && VI_TM_STAT_USE_RMSE
	if (!verify(meas->flt_cnt_ <= static_cast<VI_TM_FP>(meas->cnt_))) return VI_EXIT_FAILURE; // flt_cnt_ must be less than or equal to cnt_.
#endif

#if VI_TM_STAT_USE_MINMAX && VI_TM_STAT_USE_RMSE
	if (meas->flt_calls_ > 0U)
	{	constexpr auto fp_EPSILON = fp_limits_t::epsilon();
		if (!verify((meas->min_ - meas->flt_avg_) / meas->flt_avg_ < fp_EPSILON)) return VI_EXIT_FAILURE;
		if (!verify((meas->flt_avg_ - meas->max_) / meas->flt_avg_ < fp_EPSILON)) return VI_EXIT_FAILURE;
	}
#endif

	return VI_EXIT_SUCCESS;
}

void VI_TM_CALL vi_tmStatsReset(vi_tmMeasurementStats_t *meas) noexcept
{	if (!verify(!!meas))
	{	return;
	}

	meas->calls_ = 0U;
#if VI_TM_STAT_USE_RAW
	meas->cnt_ = 0U;
	meas->sum_ = 0U;
#endif
#if VI_TM_STAT_USE_MINMAX
	meas->min_ = fp_limits_t::infinity();
	meas->max_ = -fp_limits_t::infinity();
#endif
#if VI_TM_STAT_USE_RMSE
	meas->flt_calls_ = 0U;
	meas->flt_cnt_ = fp_ZERO;
	meas->flt_avg_ = fp_ZERO;
	meas->flt_ss_ = fp_ZERO;
#endif
	assert(VI_EXIT_SUCCESS == vi_tmStatsIsValid(meas));
}

void VI_TM_CALL vi_tmStatsAdd(vi_tmMeasurementStats_t *meas, VI_TM_TDIFF dur, VI_TM_SIZE cnt) noexcept
{	(void)dur;
	if (!verify(!!meas) || 0U == cnt)
	{	return;
	}
	assert(VI_EXIT_SUCCESS == vi_tmStatsIsValid(meas));

#if VI_TM_STAT_USE_RMSE || VI_TM_STAT_USE_MINMAX
	const auto f_cnt = static_cast<VI_TM_FP>(cnt);
	const auto f_val = static_cast<VI_TM_FP>(dur) / f_cnt;
#endif

	if (0U == meas->calls_++)
	{	// No complex calculations are required for the first (and possibly only) call.
#if VI_TM_STAT_USE_RAW
		meas->cnt_ = cnt;
		meas->sum_ = dur;
#endif
#if VI_TM_STAT_USE_MINMAX
		meas->min_ = f_val;
		meas->max_ = f_val;
#endif
#if VI_TM_STAT_USE_RMSE
		meas->flt_calls_ = 1U; // The first call cannot be filtered.
		meas->flt_cnt_ = f_cnt;
		meas->flt_avg_ = f_val; // The first value is the mean.
#endif
	}
	else
	{
#if VI_TM_STAT_USE_RAW
		meas->cnt_ += cnt;
		meas->sum_ += dur;
#endif
#if VI_TM_STAT_USE_MINMAX
		if (f_val < meas->min_) { meas->min_ = f_val; }
		if (f_val > meas->max_) { meas->max_ = f_val; }
#endif
#if VI_TM_STAT_USE_RMSE
		const auto deviation = f_val - meas->flt_avg_; // Difference from the mean value.
#	if VI_TM_STAT_USE_FILTER
		constexpr VI_TM_FP K = 2.5; // Threshold for outliers.
		if(	dur <= 1U || // The measurable interval is probably smaller than the resolution of the clock.
			FMA(deviation * deviation, meas->flt_cnt_, - K * K * meas->flt_ss_) < fp_ZERO || // Sigma clipping to avoids outliers.
			deviation < fp_ZERO || // The minimum value is usually closest to the true value. "deviation < .0" - for some reason slowly!!!
			meas->flt_calls_ <= 2U || // If we have less than 2 measurements, we cannot calculate the standard deviation.
			meas->flt_ss_ <= 1.0 // A pair of zero initial measurements will block the addition of other.
		)
#	endif
		{	meas->flt_cnt_ += f_cnt;
			meas->flt_avg_ = FMA(deviation, f_cnt / meas->flt_cnt_, meas->flt_avg_);
			meas->flt_ss_ = FMA(deviation * (f_val - meas->flt_avg_), f_cnt, meas->flt_ss_);
			meas->flt_calls_++;
		}
#endif
	}
	assert(VI_EXIT_SUCCESS == vi_tmStatsIsValid(meas));
}

void VI_TM_CALL vi_tmStatsMerge(vi_tmMeasurementStats_t* VI_RESTRICT dst, const vi_tmMeasurementStats_t* VI_RESTRICT src) noexcept
{	if(!verify(!!dst) || !verify(!!src) || dst == src || 0U == src->calls_)
	{	return;
	}

	assert(VI_EXIT_SUCCESS == vi_tmStatsIsValid(dst));
	assert(VI_EXIT_SUCCESS == vi_tmStatsIsValid(src));

	dst->calls_ += src->calls_;
#if VI_TM_STAT_USE_RAW
	dst->cnt_ += src->cnt_;
	dst->sum_ += src->sum_;
#endif
#if VI_TM_STAT_USE_MINMAX
	if(src->min_ < dst->min_)
	{	dst->min_ = src->min_;
	}
	if(src->max_ > dst->max_)
	{	dst->max_ = src->max_;
	}
#endif
#if VI_TM_STAT_USE_RMSE
	if (src->flt_cnt_ > fp_ZERO)
	{	const auto new_cnt_reverse = fp_ONE / (dst->flt_cnt_ + src->flt_cnt_);
		const auto diff_mean = src->flt_avg_ - dst->flt_avg_;
		dst->flt_avg_ = FMA(dst->flt_avg_, dst->flt_cnt_, src->flt_avg_ * src->flt_cnt_) * new_cnt_reverse;
		dst->flt_ss_ = FMA(dst->flt_cnt_ * diff_mean, src->flt_cnt_ * diff_mean * new_cnt_reverse, dst->flt_ss_ + src->flt_ss_);
		dst->flt_cnt_ += src->flt_cnt_;
		dst->flt_calls_ += src->flt_calls_;
	}
#endif
	assert(VI_EXIT_SUCCESS == vi_tmStatsIsValid(dst));
}

VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate()
{	try
	{	return new vi_tmMeasurementsJournal_t;
	}
	catch (const std::bad_alloc &)
	{	assert(false);
		return nullptr;
	}
}

void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR journal)
{	delete journal;
}

void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR journal) noexcept
{	vi_tmJournalEnumerateMeas(journal, [](VI_TM_HMEAS m, void *) { vi_tmMeasurementReset(m); return 0; }, nullptr);
}

VI_TM_RESULT VI_TM_CALL vi_tmJournalEnumerateMeas(VI_TM_HJOUR journal, vi_tmMeasEnumCb_t fn, void *ctx)
{	return misc::from_handle(journal)->for_each_measurement(fn, ctx);
}

VI_TM_HMEAS VI_TM_CALL vi_tmJournalGetMeas(VI_TM_HJOUR journal, const char *name)
{	return static_cast<VI_TM_HMEAS>(&misc::from_handle(journal)->try_emplace(name));
}

void VI_TM_CALL vi_tmMeasurementAdd(VI_TM_HMEAS meas, VI_TM_TDIFF tick_diff, VI_TM_SIZE cnt) noexcept
{	if (verify(meas)) { meas->second.add(tick_diff, cnt); }
}

void VI_TM_CALL vi_tmMeasurementMerge(VI_TM_HMEAS meas, const vi_tmMeasurementStats_t *src) noexcept
{	if (verify(meas)) { meas->second.merge(*src); }
}

void VI_TM_CALL vi_tmMeasurementGet(VI_TM_HMEAS meas, const char* *name, vi_tmMeasurementStats_t *data)
{	if (verify(meas))
	{	if (name) { *name = meas->first.c_str(); }
		if (data) { *data = meas->second.get(); }
	}
}

void VI_TM_CALL vi_tmMeasurementReset(VI_TM_HMEAS meas)
{	if (verify(meas)) { meas->second.reset(); }
}
//^^^API Implementation ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
