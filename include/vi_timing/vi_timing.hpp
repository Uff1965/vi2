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

#ifndef VI_TIMING_VI_TIMING_H
#	define VI_TIMING_VI_TIMING_H
#	pragma once

#	include "vi_timing.h"
#	include "vi_timing_aux.h"

#	define VI_STR_CONCAT_AUX( a, b ) a##b
#	define VI_STR_CONCAT( a, b ) VI_STR_CONCAT_AUX( a, b )
#	ifdef __COUNTER__
#		define VI_UNIC_ID( prefix ) VI_STR_CONCAT( prefix, __COUNTER__ )
#	else
#		define VI_UNIC_ID( prefix ) VI_STR_CONCAT( prefix, __LINE__ )
#	endif

#if defined(VI_TM_DISABLE)
#	// Fallback macros for timing functions
#	define VI_TM_INIT(...) static const int vi_tm__UNIC_ID = 0
#	define VI_TM(...) const int VI_UNIC_ID(vi_tm__) = 0
#	define VI_TM_S(...) const int VI_UNIC_ID(vi_tm__) = 0
#	define VI_TM_FUNC ((void)0)
#	define VI_TM_REPORT(...) ((void)0)
#	define VI_TM_RESET(...) ((void)0)
#	define VI_TM_FULLVERSION ""
#elif defined(__cplusplus)
#	// Visual Studio historically leaves __cplusplus as 199711L;
#	// use _MSVC_LANG for actual MSVC standard (or enable /Zc:__cplusplus).
#	if __cplusplus < 201703L && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)
#		error "vi_timing requires C++17 or later."
#	endif

#	include <cassert>
#	include <cstdint>
#	include <cstring>
#	include <limits>
#	include <string>
#	include <type_traits>
#	include <utility>

namespace vi_tm
{
	class init_t
	{
		std::string title_ = "Timing report:\n";
		VI_TM_FLAGS report_flags_ = vi_tmReportDefault;
		VI_TM_FLAGS flags_ = 0;

		init_t(const init_t &) = delete;
		init_t& operator=(const init_t &) = delete;

		template<typename T>
		VI_TM_RESULT init_aux(T &&v)
		{	VI_TM_RESULT result = 0;
			if constexpr (std::is_same_v<std::decay_t<T>, vi_tmReportFlags_e>)
			{	report_flags_ |= v;
			}
			else if constexpr (std::is_same_v<std::decay_t<T>, vi_tmInitFlags_e>)
			{	flags_ |= v;
			}
			else if constexpr (std::is_same_v<T, decltype(title_)>)
			{	title_ = std::forward<T>(v);
			}
			else if constexpr (std::is_convertible_v<T, decltype(title_)>)
			{	title_ = v;
			}
			else
			{	assert(false); // Unknown parameter type.
				result = 0 - __LINE__;
			}

			return result;
		}

		template<typename... Args>
		VI_TM_RESULT init(Args&&... args)
		{	VI_TM_RESULT result = 0;
			((result |= init_aux(std::forward<Args>(args))), ...);
			result |= vi_tmInit(title_.c_str(), report_flags_, flags_);
			return result;
		}
	public:
		init_t() { auto ret = init(); (void)ret; assert(0 == ret); } // Default flags and other settings.
		template<typename... Args> explicit init_t(Args&&... args)
			: report_flags_{0U}
		{	init(std::forward<Args>(args)...);
		}
		~init_t()
		{	vi_tmShutdown();
		}
	}; // class init_t

	// probe_t class: A RAII-style class for measuring code execution time.
	// Unlike the API, this class is not thread-safe!!!
	class probe_t
	{	using signed_tm_size_t = std::make_signed_t<VI_TM_SIZE>; // Signed type with the same size as VI_TM_SIZE
		struct paused_tag {}; // Tag type for paused constructor

		// Invariants:
		//  - meas_ is a non-owning handle; caller retains ownership and must ensure validity.
		//  - cnt_ encodes state: >0 running (count = cnt_), <0 paused (count = -cnt_), 0 idle.
		//  - start_ is meaningful only when cnt_ != 0 (running or paused).
		VI_TM_HMEAS meas_{nullptr};
		signed_tm_size_t cnt_{0};
		VI_TM_TICK start_{VI_TM_TICK{ 0 }}; // Must be declared last - initializes after other members to minimize overhead between object construction and measurement start.

		// Private constructor used by factory methods
		explicit probe_t(paused_tag, VI_TM_HMEAS m, VI_TM_SIZE cnt) noexcept
		:	meas_{ m },
			cnt_{ -static_cast<signed_tm_size_t>(cnt) }
		{/**/}
		explicit probe_t(VI_TM_HMEAS m, VI_TM_SIZE cnt) noexcept
		:	meas_{ m },
			cnt_{ static_cast<signed_tm_size_t>(cnt) },
			start_{ vi_tmGetTicks() }
		{/**/}
	public:
		probe_t() = delete;
		probe_t(const probe_t &) = delete;
		probe_t& operator=(const probe_t &) = delete;

		// === Factory methods ===

		/// Create a running probe (started immediately).
		[[nodiscard]] static probe_t make_running(VI_TM_HMEAS m, VI_TM_SIZE cnt = 1) noexcept
		{	assert(!!m && cnt != 0 && cnt <= static_cast<VI_TM_SIZE>(std::numeric_limits<signed_tm_size_t>::max()));
			// cnt must fit into signed_tm_size_t; caller is responsible for sane values.
			return probe_t{ m, cnt };
		}

		/// Create a paused probe (not started yet).
		[[nodiscard]] static probe_t make_paused(VI_TM_HMEAS m, VI_TM_SIZE cnt = 1) noexcept
		{	assert(!!m && cnt != 0 && cnt < static_cast<VI_TM_SIZE>(std::numeric_limits<signed_tm_size_t>::max()));
			// cnt must fit into signed_tm_size_t; caller is responsible for sane values.
			return probe_t{ paused_tag{}, m, cnt };
		}

		// === Move support ===

		probe_t(probe_t &&s) noexcept
		:	meas_{std::exchange(s.meas_, nullptr)},
			cnt_{ std::exchange(s.cnt_, signed_tm_size_t{ 0 }) },
			start_{std::exchange(s.start_, VI_TM_TICK{ 0 })}
		{
		}

		probe_t& operator =(probe_t &&s) noexcept
		{	if (&s != this)
			{	stop();
				meas_ = std::exchange(s.meas_, nullptr);
				cnt_ = std::exchange(s.cnt_, signed_tm_size_t{ 0 });
				start_ = std::exchange(s.start_, VI_TM_TICK{ 0 });
			}
			return *this;
		}

		// === RAII cleanup ===

		~probe_t() noexcept
		{	stop();
		}

		// === Control ===

		/// Pause a running probe (accumulate elapsed time).
		void pause() noexcept
		{	const auto t = vi_tmGetTicks();
			assert(active());
			if (cnt_ > 0)
			{	start_ = t - start_;
				cnt_ = -cnt_;
			}
		}

		/// Resume a paused probe (continue from accumulated time).
		void resume() noexcept
		{	assert(paused());
			if (cnt_ < 0)
			{	cnt_ = -cnt_;
				start_ = vi_tmGetTicks() - start_;
			}
		}

		/// Stop probe and record measurement.
		void stop() noexcept
		{	assert(!cnt_ || !!meas_);
			if (cnt_ > 0)
			{	const auto t = vi_tmGetTicks(); // Read ticks first to avoid introducing measurement overhead in conditional branch
				vi_tmMeasurementAdd(meas_, t - start_, cnt_);
			}
			else if (cnt_ < 0)
			{	vi_tmMeasurementAdd(meas_, start_, -cnt_);
			}
			cnt_ = 0; // Set idle state.
		}

		/// Obtaining the current accumulated time (for debugging/monitoring)
		[[nodiscard]] VI_TM_TDIFF elapsed() const noexcept
		{	if (cnt_ > 0)
			{	return vi_tmGetTicks() - start_;
			}
			else if (cnt_ < 0)
			{	return start_;
			}
			return VI_TM_TDIFF{ 0 };
		}

		// === State checks ===

		[[nodiscard]] bool idle() const noexcept { return cnt_ == 0; }
		[[nodiscard]] bool active() const noexcept { return cnt_ > 0; }
		[[nodiscard]] bool paused() const noexcept { return cnt_ < 0; }
	}; // class probe_t

	inline std::string to_string(double val, unsigned char sig = 2U, unsigned char dec = 1U)
	{	std::string result;
		result.resize(sig + (9 + 1 + 1), '\0'); // "-00S.Se-308"
		const auto len = vi_tmF2A(result.data(), result.size(), val, sig, dec);
		result.resize(len - 1U);
		return result;
	}
} // namespace vi_tm

// Initializes the global registry and sets up the report callback.
#	define VI_TM_INIT(...) vi_tm::init_t vi_tm__UNIC_ID {__VA_ARGS__}

// VI_[N]DEBUG_ONLY macro: Expands to its argument only in debug builds, otherwise expands to nothing.
#	if VI_TM_DEBUG
#		define VI_TM_DEBUG_ONLY(t) t
#	else
#		define VI_TM_DEBUG_ONLY(t)
#	endif

	/// <summary>
	/// Starts a scoped timing probe for high-resolution profiling.
	/// </summary>
	/// <param name="name">
	/// A literal, null-terminated string that names this profiling scope.
	/// </param>
	/// <param name="cnt">
	/// Optional count multiplier for this probe (default is 1).
	/// </param>
	/// <remarks>
	/// The macro defines a unique probe object whose underlying measurement handle
	/// is obtained from the global registry. The handle is looked up by name and
	/// used to construct a RAII-style `vi_tm::probe_t` that starts immediately.
	/// </remarks>
#	define VI_TM(...) \
		const auto VI_UNIC_ID(_vi_tm_) = [] (const char* name, VI_TM_SIZE cnt = 1) -> vi_tm::probe_t \
		{	const auto meas = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, name); \
			return vi_tm::probe_t::make_running(meas, cnt); \
		}(__VA_ARGS__)

	/// <summary>
	/// See <see cref="VI_TM"/> for the full description. Starts a scoped timing probe with a cached measurement lookup.
	/// </summary>
	/// <remarks>
	/// Behavior is the same as <see cref="VI_TM"/> except that the measurement handle is cached
	/// in a static variable to avoid repeated lookups by name.
	///
	/// Important: each invocation of <c>VI_TM_S</c> at the same source location MUST use the
	/// exact same <paramref name="name"/> and <paramref name="cnt"/> values. Reusing the macro
	/// at the same location with different arguments is forbidden and will produce inconsistent
	/// or invalid profiling data.
	/// </remarks>
#	define VI_TM_S(...) \
		const auto VI_UNIC_ID(_vi_tm_) = [] (const char* name, VI_TM_SIZE cnt = 1) -> vi_tm::probe_t \
		{	static const auto meas = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, name); /* Static, so as not to waste resources on repeated searches for measurements by name. */ \
			VI_TM_DEBUG_ONLY \
			(	const char* registered_name = nullptr; \
				vi_tmMeasurementGet(meas, &registered_name, nullptr); \
				assert(registered_name && 0 == std::strcmp(name, registered_name) && \
					"One VI_TM macro cannot be reused with a different name value!"); \
			) \
			return vi_tm::probe_t::make_running(meas, cnt); \
		}(__VA_ARGS__)

	// This macro is used to create a probe_t object with the function name as the measurement name.
#	define VI_TM_FUNC VI_TM_S(VI_FUNCNAME, 1U)
	// Generates a report for the global registry.
#	define VI_TM_REPORT(...) vi_tmReport(VI_TM_HGLOBAL, __VA_ARGS__)
	// Resets the data of the specified measure entry in global registry. The handle remains valid.
#	define VI_TM_RESET(name) vi_tmMeasurementReset(vi_tmRegistryGetMeas(VI_TM_HGLOBAL, (name)))
	// Full version string of the library (Example: "0.1.0.2506151515R static").
#	define VI_TM_FULLVERSION static_cast<const char*>(vi_tmStaticInfo(vi_tmInfoVersion))
#endif // #ifdef __cplusplus
#endif // #ifndef VI_TIMING_VI_TIMING_H
