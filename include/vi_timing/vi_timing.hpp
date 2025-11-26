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

/*****************************************************************************\
* This header defines the C++ RAII wrappers for vi_timing.
* Unlike the C API, these classes are not thread-safe,
* even when VI_TM_THREADSAFE is defined as true.
* 
* Designed for minimal overhead in practical use.
* In release builds, runtime checks that could slow down execution are omitted;
* debug builds provide necessary validation through assert().
\*****************************************************************************/

#ifndef VI_TIMING_VI_TIMING_H
#	define VI_TIMING_VI_TIMING_H
#	pragma once

#	include "vi_timing.h"
#	if defined(__has_include) && __has_include("vi_timing_aux.h")
#		include "vi_timing_aux.h"
#	endif

#	if defined(__COUNTER__) // MSVC, GCC, Clang and some other compilers support __COUNTER__
#		define VI_UNIC_ID( prefix ) VI_STR_CONCAT(prefix, VI_STR_CONCAT(__LINE__, VI_STR_CONCAT(_, __COUNTER__)))
#	else // Fallback to __LINE__ only which may cause collisions if multiple expansions occur on the same line
#		define VI_UNIC_ID( prefix ) VI_STR_CONCAT(prefix, __LINE__)
#	endif

#if defined(VI_TM_DISABLE) || !defined(__cplusplus)
#	// Fallback macros for timing functions
#	define VI_TM_H(hreg, ...) const int VI_UNIC_ID(vi_tm__) = 0
#	define VI_TM_SH(hreg, ...) const int VI_UNIC_ID(vi_tm__) = 0
#	define VI_TM_FUNC_H(hreg) const int VI_UNIC_ID(vi_tm__) = 0
#	define VI_TM_REPORT_H(hreg, ...) 0
#	define VI_TM_RESET_H(hreg, ...) (void)0
#	// Fallback macros for global registry timing functions
#	define VI_TM(...) VI_TM_H(0, __VA_ARGS__)
#	define VI_TM_S(...) VI_TM_SH(0, __VA_ARGS__)
#	define VI_TM_FUNC VI_TM_FUNC_H(0)
#	define VI_TM_REPORT(...) VI_TM_REPORT_H(0, __VA_ARGS__)
#	define VI_TM_RESET(...) VI_TM_RESET_H(0, __VA_ARGS__)
#	// Fallback macro for full version string and global init
#	define VI_TM_FULLVERSION ""
#	define VI_TM_GLOBALINIT(...) 0
#else
#	// Visual Studio historically leaves __cplusplus as 199711L;
#	// use _MSVC_LANG for actual MSVC standard (or enable /Zc:__cplusplus).
#	if __cplusplus < 201703L && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)
#		error "vi_timing requires C++17 or later."
#	endif

#	include <cassert> // assert
#	include <cstring> // std::strcmp
#	include <limits> // std::numeric_limits
#	include <optional>
#	include <string> // std::string
#	include <type_traits> // std::make_signed_t
#	include <utility> // std::exchange

namespace vi_tm
{
	struct init_t
	{	std::optional<std::string> title_;
		std::optional<std::string> footer_;
		std::optional<VI_TM_FLAGS> report_flags_;
	};

	// Auxiliary function to process a single parameter for global_init.
	// Used internally by global_init.
	template<typename T>
	VI_TM_RESULT init_aux(init_t &self, T &&v)
	{	VI_TM_RESULT result = VI_SUCCESS;
		if constexpr (std::is_same_v<std::decay_t<T>, vi_tmReportFlags_e>)
		{	self.report_flags_.emplace(v | self.report_flags_.value_or(0U));
		}
		else if constexpr (std::is_convertible_v<T, decltype(self.title_)::value_type>)
		{	if(!self.title_.has_value())
				self.title_.emplace(std::forward<T>(v));
			else if(!self.footer_.has_value())
				self.footer_.emplace(std::forward<T>(v));
			else
			{	assert(false); // Both title and footer are already set.
				result = 1 - __LINE__;
			}
		}
		else
		{	static_assert(false); // Unknown parameter type.
			result = 1 - __LINE__;
		}
		
		return result;
	}

	// Variadic global initialization function.
	// Accepts parameters in any order:
	// - vi_tmReportFlags_e (multiple allowed; separated by comma)
	// - const char* or std::string (first is title, second is footer)
	// Returns VI_SUCCESS on success; error code otherwise.
	// Asserts on invalid parameter types or too many string parameters.
	// Example usage:
	// vi_tm::global_init(vi_tmReportSortByName, vi_tmReportShowResolution, "My Timing Report", "End of Report");
	// vi_tm::global_init("My Timing Report", vi_tmReportShowDuration, vi_tmReportShowCalls);
	template<typename... Args>
	VI_TM_RESULT global_init(Args&&... args)
	{	init_t self;
		if constexpr (sizeof...(Args))
		{	for (auto r : { init_aux(self, std::forward<Args>(args))... })
				if (VI_FAILED(r))
				{	return r;
				}
		}
		return vi_tmGlobalInit
		(	self.report_flags_ ? *self.report_flags_ : vi_tmReportDefault,
			self.title_ ? self.title_->c_str() : nullptr,
			self.footer_ ? self.footer_->c_str() : nullptr
		);
	}

/// In class comment:
/// scoped_probe_t class: A RAII-style class for measuring code execution time.
/// Note: This class is not thread-safe. If shared across threads,
/// external synchronization is required even when VI_TM_THREADSAFE is enabled.
/// If VI_TM_THREADSAFE is enabled at the C layer, the registry/measurement updates are protected,
/// but the RAII object’s own state transitions must be externally synchronized when shared.
	class [[nodiscard]] scoped_probe_t
	{	using signed_tm_size_t = std::make_signed_t<VI_TM_SIZE>; // Signed type with the same size as VI_TM_SIZE

		// Invariants:
		//  - meas_ is a non-owning handle; caller retains ownership and must ensure validity.
		//  - cnt_and_state_ encodes state: >0 running (count = cnt_and_state_), <0 paused (count = -cnt_and_state_), 0 idle.
		//  - time_data_ is meaningful only when cnt_and_state_ != 0 (running or paused).
		VI_TM_HMEAS meas_{nullptr};
		signed_tm_size_t cnt_and_state_{0};
		VI_TM_TICK time_data_{VI_TM_TICK{ 0 }}; // Must be declared last - initializes after other members to minimize overhead between object construction and measurement start.

		// Private constructor used by factory methods
		explicit scoped_probe_t(VI_TM_HMEAS m, signed_tm_size_t cnt) noexcept
		:	meas_{ m },
			cnt_and_state_{ cnt },
			time_data_{ vi_tmGetTicks() }
		{/**/}
	public:
		scoped_probe_t() = delete;
		scoped_probe_t(const scoped_probe_t &) = delete;
		scoped_probe_t& operator=(const scoped_probe_t &) = delete;

		// === State checks ===

		[[nodiscard]] bool idle() const noexcept { return cnt_and_state_ == 0; }
		[[nodiscard]] bool active() const noexcept { return cnt_and_state_ > 0; }
		[[nodiscard]] bool paused() const noexcept { return cnt_and_state_ < 0; }

		// === Factory methods ===

		/// Create a running probe (started immediately).
		[[nodiscard]] static scoped_probe_t make_running(VI_TM_HMEAS m, VI_TM_SIZE cnt = 1) noexcept
		{	assert(!!m && !!cnt && cnt <= static_cast<VI_TM_SIZE>(std::numeric_limits<signed_tm_size_t>::max()));
			// cnt must fit into signed_tm_size_t; caller is responsible for sane values.
			return scoped_probe_t{ m, static_cast<signed_tm_size_t>(cnt) };
		}

		/// Create a paused probe (not started yet).
		[[nodiscard]] static scoped_probe_t make_paused(VI_TM_HMEAS m, VI_TM_SIZE cnt = 1) noexcept
		{	assert(!!m && !!cnt && cnt <= static_cast<VI_TM_SIZE>(std::numeric_limits<signed_tm_size_t>::max()));
			// cnt must fit into signed_tm_size_t; caller is responsible for sane values.
			return scoped_probe_t{ m, -static_cast<signed_tm_size_t>(cnt) };
		}

		// === Move support ===

		scoped_probe_t(scoped_probe_t &&s) noexcept
		:	meas_{std::exchange(s.meas_, nullptr)},
			cnt_and_state_{ std::exchange(s.cnt_and_state_, signed_tm_size_t{ 0 }) },
			time_data_{std::exchange(s.time_data_, VI_TM_TICK{ 0 })}
		{
		}

		scoped_probe_t& operator =(scoped_probe_t &&s) noexcept
		{	if (&s != this)
			{	stop();
				meas_ = std::exchange(s.meas_, nullptr);
				cnt_and_state_ = std::exchange(s.cnt_and_state_, signed_tm_size_t{ 0 });
				time_data_ = std::exchange(s.time_data_, VI_TM_TICK{ 0 });
			}
			return *this;
		}

		// === RAII cleanup ===

		~scoped_probe_t() noexcept
		{	stop();
		}

		// === Control ===

		/// Pause a running probe (accumulate elapsed time).
		void pause() noexcept
		{	const auto t = vi_tmGetTicks(); // Read ticks first to avoid introducing measurement overhead in conditional branch
			assert(active());
			if(active())
			{	time_data_ = t - time_data_;
				cnt_and_state_ = -cnt_and_state_;
			}
		}

		/// Resume a paused probe (continue from accumulated time).
		void resume() noexcept
		{	assert(paused());
			if (paused())
			{	cnt_and_state_ = -cnt_and_state_;
				time_data_ = vi_tmGetTicks() - time_data_;
			}
		}

		/// Stop probe and record measurement.
		void stop() noexcept
		{	const auto t = vi_tmGetTicks(); // Read ticks first to avoid introducing measurement overhead in conditional branch
			assert(idle() || !!meas_);
			if (active())
			{	vi_tmMeasurementAdd(meas_, t - time_data_, cnt_and_state_);
			}
			else if (paused())
			{	vi_tmMeasurementAdd(meas_, time_data_, -cnt_and_state_);
			}
			cnt_and_state_ = 0; // Set idle state.
		}

		/// Obtaining the current accumulated time (for debugging/monitoring)
		[[nodiscard]] VI_TM_TDIFF elapsed() const noexcept
		{	if (paused()) // The measurement will probably be paused before the 'function'elapsed()' is called.
			{	return time_data_;
			}
			if (active())
			{	return vi_tmGetTicks() - time_data_;
			}
			assert(false);
			return VI_TM_TDIFF{ 0 };
		}

		class scoped_pause_t;
		class scoped_resume_t;

		// scoped_pause_t: RAII-style pause/resume helper for scoped_probe_t.
		class [[nodiscard]] scoped_pause_t
		{	scoped_probe_t &p_;
		public:
			explicit scoped_pause_t(scoped_probe_t &p) noexcept: p_{ p } { p_.pause(); }
			~scoped_pause_t() noexcept { p_.resume(); }
			scoped_pause_t(const scoped_pause_t &) = delete;
			scoped_pause_t &operator=(const scoped_pause_t &) = delete;
			[[nodiscard]] scoped_resume_t scoped_resume() noexcept { return scoped_resume_t{ p_ }; }
		};

		// scoped_resume_t: RAII-style resume/pause helper for scoped_probe_t.
		class [[nodiscard]] scoped_resume_t
		{	scoped_probe_t &p_;
		public:
			explicit scoped_resume_t(scoped_probe_t &p) noexcept: p_{ p } { p_.resume(); }
			~scoped_resume_t() noexcept { p_.pause(); }
			scoped_resume_t(const scoped_resume_t &) = delete;
			scoped_resume_t &operator=(const scoped_resume_t &) = delete;
			[[nodiscard]] scoped_pause_t scoped_pause() noexcept { return scoped_pause_t{ p_ }; }
		};

		[[nodiscard]] scoped_resume_t scoped_resume() noexcept { return scoped_resume_t{ *this }; }
		[[nodiscard]] scoped_pause_t scoped_pause() noexcept { return scoped_pause_t{ *this }; }
	}; // class scoped_probe_t

	[[nodiscard]] inline std::string to_string(double val, unsigned char sig = 2U, unsigned char dec = 1U)
	{	std::string result;
		result.resize(sig + (9 + 1 + 1), '\0'); // "-00S.Se-308"
		if (const auto len = vi_tmF2A(result.data(), result.size(), val, sig, dec); len > 1)
		{	result.resize(len - 1);
		}
		else
		{	assert(false);
			result.clear();
		}
		return result;
	}
} // namespace vi_tm

#// VI_[N]DEBUG_ONLY macro: Expands to its argument only in debug builds, otherwise expands to nothing.
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
/// used to construct a RAII-style `vi_tm::scoped_probe_t` that starts immediately.
/// Important: This macro relies on auto-generated identifiers (__LINE__, __COUNTER__).
/// Uniqueness is not guaranteed in all cases. If a conflict occurs,
/// declare a vi_tm::scoped_probe_t manually.
/// </remarks>
#	define VI_TM_H(hreg, ...) \
		const auto VI_UNIC_ID(_vi_tm_) = [] (const char* name, VI_TM_SIZE cnt = 1) -> vi_tm::scoped_probe_t \
		{	const auto meas = vi_tmRegistryGetMeas((hreg), name); \
			return vi_tm::scoped_probe_t::make_running(meas, cnt); \
		}(__VA_ARGS__)
#
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
/// 
/// Important: the registry handle (hreg) must remain alive for the entire lifetime
/// of this translation unit’s static storage; the cached measurement handle 'meas'
/// will dangle if 'vi_tmRegistryClose(hreg)' is called earlier.
/// 
/// Important: This macro relies on auto-generated identifiers (__LINE__, __COUNTER__).
/// Uniqueness is not guaranteed in all cases. If a conflict occurs,
/// declare a vi_tm::scoped_probe_t manually.
/// </remarks>
#	define VI_TM_SH(hreg, ...) \
		const auto VI_UNIC_ID(_vi_tm_) = [] (const char* name, VI_TM_SIZE cnt = 1) -> vi_tm::scoped_probe_t \
		{	static const auto meas = vi_tmRegistryGetMeas((hreg), name); /* Static, so as not to waste resources on repeated searches for measurements by name. */ \
			VI_TM_DEBUG_ONLY \
			(	const char* registered_name = nullptr; \
				vi_tmMeasurementGet(meas, &registered_name, nullptr); \
				assert(registered_name && 0 == std::strcmp(name, registered_name) && \
					"One VI_TM macro cannot be reused with a different name value!"); \
			) \
			return vi_tm::scoped_probe_t::make_running(meas, cnt); \
		}(__VA_ARGS__)
#
#	// This macro is used to create a scoped_probe_t object with the function name as the measurement name.
#	define VI_TM_FUNC_H(hreg) VI_TM_SH((hreg), VI_FUNCNAME, 1U)
#	// Generates a report for the registry.
#	define VI_TM_REPORT_H(hreg, ...) vi_tmRegistryReport((hreg), __VA_ARGS__)
#	// Resets the data of the specified measure entry in registry. The handle remains valid.
#	define VI_TM_RESET_H(hreg, name) vi_tmMeasurementReset(vi_tmRegistryGetMeas((hreg), (name)))
#
#	// Macros for global registry timing functions.
#	define VI_TM(...) VI_TM_H(VI_TM_HGLOBAL, __VA_ARGS__)
#	define VI_TM_S(...) VI_TM_SH(VI_TM_HGLOBAL, __VA_ARGS__)
#	define VI_TM_FUNC VI_TM_FUNC_H(VI_TM_HGLOBAL)
#	define VI_TM_REPORT(...) VI_TM_REPORT_H(VI_TM_HGLOBAL, __VA_ARGS__)
#	define VI_TM_RESET(...) VI_TM_RESET_H(VI_TM_HGLOBAL, __VA_ARGS__)
#
#	// Full version string of the library (Example: "0.1.0.2506151515R static").
#	define VI_TM_FULLVERSION static_cast<const char*>(vi_tmStaticInfo(vi_tmInfoVersion))
#	// Initializes the global timing registry with optional reporting.
#	define VI_TM_GLOBALINIT(...) vi_tm::global_init(__VA_ARGS__)

#endif // #if !defined(VI_TM_DISABLE) && defined(__cplusplus)
#endif // #ifndef VI_TIMING_VI_TIMING_H
