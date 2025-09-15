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

#define VI_ID __LINE__
#define VI_STR_CONCAT_AUX( a, b ) a##b
#define VI_STR_CONCAT( a, b ) VI_STR_CONCAT_AUX( a, b )
#define VI_UNIC_ID( prefix ) VI_STR_CONCAT( prefix, VI_ID )
#define VI_STRINGIZE(x) #x

#ifdef VI_TM_DISABLE
	// Fallback macros for timing functions
#	define VI_TM_INIT(...) static const int vi_tm__UNIC_ID = 0
#	define VI_TM(...) const int VI_UNIC_ID(vi_tm__) = 0
#	define VI_TM_FUNC ((void)0)
#	define VI_TM_REPORT(...) ((void)0)
#	define VI_TM_RESET(...) ((void)0)
#	define VI_TM_FULLVERSION ""
#else
#	ifdef __cplusplus
#		include <cassert>
#		include <cstring>
#		include <string>
#		include <utility>

// By default, Visual Studio always returns the value 199711L for the __cplusplus preprocessor macro.
// The Microsoft-specific macro _MSVC_LANG also reports the version standard.
#		if __cplusplus < 201703L && _MSVC_LANG < 201703L
#			error "vi_timing requires C++17 or later."
#		endif

namespace vi_tm
{
	// probe_t class: A RAII-style class for measuring code execution time.
	// Unlike the API, this class is not thread-safe!!!
	class probe_t
	{	VI_TM_HMEAS meas_ = nullptr;
		VI_TM_SIZE cnt_ = 0U;
		VI_TM_TICK start_ = 0U; // Order matters!!! 'start_' must be initialized last!
	public:
		probe_t() = delete;
		probe_t(const probe_t &) = delete;
		probe_t(probe_t &&src) noexcept
		:	meas_{ std::exchange(src.meas_, nullptr) },
			cnt_{ std::exchange(src.cnt_, 0U) },
			start_{ src.start_ }
		{	assert(meas_);
		}
		explicit probe_t(VI_TM_HMEAS m, VI_TM_SIZE cnt = 1) noexcept
		:	meas_{ m },
			cnt_{ cnt }
		{	assert(meas_);
			if (cnt_)
			{	start_ = vi_tmGetTicks();
			}
		}
		~probe_t() { finish(); }
		void operator=(const probe_t &) = delete;
		probe_t &operator=(probe_t &&src) noexcept
		{	if (this != &src)
			{	meas_ = std::exchange(src.meas_, nullptr);
				cnt_ = std::exchange(src.cnt_, 0U);
				start_ = src.start_;
			}
			assert(meas_);
			return *this;
		};
		bool is_active() const noexcept
		{	return 0U != cnt_;
		}
		void start(VI_TM_SIZE cnt = 1U) noexcept
		{	assert(!is_active() && 0U != cnt); // Ensure that the measurer is not already running and that a valid cnt is provided.
			cnt_ = cnt;
			start_ = vi_tmGetTicks(); // Reset start time.
		}
		void stop() noexcept // Stop the measurer without saved time.
		{	cnt_ = 0U;
		}
		void finish()
		{	if (is_active())
			{	const auto finish = vi_tmGetTicks();
				vi_tmMeasurementAdd(meas_, finish - start_, cnt_);
				cnt_ = 0;
			}
		}
	}; // class probe_t

	inline std::string to_string(double val, unsigned char sig = 2U, unsigned char dec = 1U)
	{	std::string result;
		result.resize(sig + (9 + 1 + 1), '\0'); // "-00S.Se-308"
		const auto len = vi_tmF2A(result.data(), result.size(), val, sig, dec);
		result.resize(len - 1U);
		return result;
	}
} // namespace vi_tm

// VI_[N]DEBUG_ONLY macro: Expands to its argument only in debug builds, otherwise expands to nothing.
#	if VI_TM_DEBUG
#		define VI_TM_NDEBUG_ONLY(t)
#		define VI_TM_DEBUG_ONLY(t) t
#	else
#		define VI_TM_NDEBUG_ONLY(t) t
#		define VI_TM_DEBUG_ONLY(t)
#	endif

	// The VI_TM macro creates a probe_t object with a unique identifier based on the line number.
	// It stores the pointer to the named measurer entry in a static variable. Therefore, it cannot 
	// be called with different measurement names.
#	define VI_TM(...) \
		const auto VI_UNIC_ID(_vi_tm_) = [] (const char* name, VI_TM_SIZE cnt = 1) -> vi_tm::probe_t { \
			static const auto meas = vi_tmMeasurement(VI_TM_HGLOBAL, name); /* Static, so as not to waste resources on repeated searches for measurements by name. */ \
			VI_TM_DEBUG_ONLY \
			(	const char* registered_name = nullptr; \
				vi_tmMeasurementGet(meas, &registered_name, nullptr); \
				assert(registered_name && 0 == std::strcmp(name, registered_name) && \
					"One VI_TM macro cannot be reused with a different name value!"); \
			) \
			return vi_tm::probe_t{meas, cnt}; \
		}(__VA_ARGS__)

	// This macro is used to create a probe_t object with the function name as the measurement name.
#	define VI_TM_FUNC VI_TM(VI_FUNCNAME)

	// Generates a report for the global journal.
#	define VI_TM_REPORT(...) vi_tmReport(VI_TM_HGLOBAL, __VA_ARGS__)

	// Resets the data of the specified measure entry in global journal. The handle remains valid.
#	define VI_TM_RESET(name) vi_tmMeasurementReset(vi_tmMeasurement(VI_TM_HGLOBAL, (name)))

	// Full version string of the library (Example: "0.1.0.2506151515R static").
#	define VI_TM_FULLVERSION static_cast<const char*>(vi_tmStaticInfo(VI_TM_INFO_VERSION))
#endif // #ifdef __cplusplus
#endif // #ifdef VI_TM_DISABLE #else
#endif // #ifndef VI_TIMING_VI_TIMING_H
