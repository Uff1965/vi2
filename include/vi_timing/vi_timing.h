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

#ifndef VI_TIMING_VI_TIMING_C_H
#	define VI_TIMING_VI_TIMING_C_H
#	pragma once

// Ensure minimum language standard: C99 or C++17
#if defined(_MSVC_LANG) // Some MSVC versions (pre-2019) define __cplusplus incorrectly unless /Zc:__cplusplus is enabled. Fallback to _MSVC_LANG if available.
#	if _MSVC_LANG < 201703L
#		error "vi_timing requires at least C++17 or newer."
#	endif
#elif defined(__cplusplus)
#	if __cplusplus < 201703L
#		error "vi_timing requires at least C++17 or newer."
#	endif
#elif defined(__STDC_VERSION__)
#	if __STDC_VERSION__ < 199901L
#		error "vi_timing requires at least ISO C99 or newer."
#	endif
#else
#	error "Unknown language standard. vi_timing requires at least C99 or C++17."
#endif

// Optionally include auto-generated version header if available and not already defined
#if !defined(VI_TM_VERSION_MAJOR) && defined(__has_include) && __has_include("vi_timing_version.h")
#	include "vi_timing_version.h" // Include the generated version data.
#endif

#include <stdint.h> // uint64_t
#include <stddef.h> // size_t

//*******************************************************************************************************************
// Library configuration options:

// Set VI_TM_DEBUG to FALSE to link with the release version of the library.
// Note: This may impact runtime performance.
#if !defined(VI_TM_DEBUG) && !defined(NDEBUG)
#	define VI_TM_DEBUG 1 // Enable debug mode.
#endif

// Set VI_TM_USE_STDCLOCK to TRUE to build the library as a SHARED library.
// Requires library rebuild.
#ifndef VI_TM_SHARED
#	define VI_TM_SHARED 0
#endif

// Used high-performance timing methods (e.g., platform-specific optimizations like ASM).
// Set VI_TM_USE_STDCLOCK to TRUE to switch to the standard C11 function `timespec_get()`.
// Requires library rebuild.
#ifndef VI_TM_USE_STDCLOCK
#	define VI_TM_USE_STDCLOCK 0
#endif

// Set VI_TM_THREADSAFE to FALSE to disable thread safety.
// Improves performance in single-threaded applications.
// Library rebuild required
#ifndef VI_TM_THREADSAFE
#	define VI_TM_THREADSAFE 1
#endif

// Set VI_TM_STAT_USE_RAW macro to FALSE to skip basic statistics collection (cnt, sum).
// Library rebuild required
#ifndef VI_TM_STAT_USE_RAW
#	define VI_TM_STAT_USE_RAW 1
#endif

// Set the VI_TM_STAT_USE_RMSE macro to FALSE to skip correlation coefficient calculation.
// Library rebuild required
#ifndef VI_TM_STAT_USE_RMSE
#	define VI_TM_STAT_USE_RMSE 1
#endif

// Set the VI_TM_STAT_USE_RMSE macro to FALSE to switch off filtering in measurements.
// Library rebuild required
#if !defined(VI_TM_STAT_USE_FILTER) && VI_TM_STAT_USE_RMSE
#	define VI_TM_STAT_USE_FILTER 1
#elif VI_TM_STAT_USE_FILTER && !VI_TM_STAT_USE_RMSE
#	error "The filter is only available when RMSE is enabled."
#endif

// Uncomment the next line to store minimum and maximum measurement values. Library rebuild required
#ifndef VI_TM_STAT_USE_MINMAX
#	define VI_TM_STAT_USE_MINMAX 0
#endif

// If VI_TM_EXPORTS defined, the library is built as a shared and exports its functions.
#ifndef VI_TM_EXPORTS
#	define VI_TM_EXPORTS 0
#endif

//*******************************************************************************************************************

// Configure calling conventions and symbol visibility for different compilers and platforms.
#if defined(_MSC_VER)
#	ifdef _M_IX86 // x86 architecture MSVC
#		define VI_SYS_CALL __cdecl
#		define VI_TM_CALL __cdecl
#	else
#		define VI_SYS_CALL
#		define VI_TM_CALL
#	endif
#
#	if ! VI_TM_SHARED
#		define VI_TM_API
#	elif ! VI_TM_EXPORTS
#		define VI_TM_API __declspec(dllimport)
#	else
#		define VI_TM_API __declspec(dllexport)
#	endif
#elif defined (__GNUC__) || defined(__clang__)
#	ifdef __i386__ // x86 architecture GCC/Clang
#		define VI_SYS_CALL __attribute__((cdecl))
#		define VI_TM_CALL __attribute__((cdecl))
#	else
#		define VI_SYS_CALL
#		define VI_TM_CALL
#	endif
#
#	if VI_TM_EXPORTS
#		define VI_TM_API __attribute__((visibility("default")))
#	else
#		define VI_TM_API
#	endif
#else
#	define VI_TM_DISABLE "Unknown compiler!"
#	define VI_SYS_CALL
#	define VI_TM_CALL
#	define VI_TM_API
#endif

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

// Compiler feature abstraction layer
// Normalizes compiler-specific keywords and pragmas for:
//   - Function name introspection (VI_FUNCNAME)
//   - Pointer aliasing (VI_RESTRICT)
//   - Inline control (VI_NOINLINE)
//   - Temporary optimization disabling/enabling (VI_OPTIMIZE_OFF/ON)
// Supported compilers: MSVC, GCC, Clang
// Usage:
//   VI_NOINLINE void my_function() { ... }
//   VI_OPTIMIZE_OFF
//		... unoptimized code section
//   VI_OPTIMIZE_ON
#if defined(_MSC_VER)
#	define VI_FUNCNAME __FUNCSIG__
#	define VI_RESTRICT __restrict
#	define VI_NOINLINE		__declspec(noinline)
#	define VI_OPTIMIZE_OFF	_Pragma("optimize(\"\", off)")
#	define VI_OPTIMIZE_ON	_Pragma("optimize(\"\", on)")
#elif defined(__clang__)
#	define VI_FUNCNAME __PRETTY_FUNCTION__
#	define VI_RESTRICT __restrict__
#	define VI_NOINLINE		[[gnu::noinline]]
#	define VI_OPTIMIZE_OFF	_Pragma("clang optimize push") \
							_Pragma("clang optimize off")
#	define VI_OPTIMIZE_ON	_Pragma("clang optimize pop")
#elif defined(__GNUC__)
#	define VI_FUNCNAME __PRETTY_FUNCTION__
#	define VI_RESTRICT __restrict__
#	define VI_NOINLINE		[[gnu::noinline]]
#	define VI_OPTIMIZE_OFF	_Pragma("GCC push_options") \
							_Pragma("GCC optimize(\"O0\")")
#	define VI_OPTIMIZE_ON	_Pragma("GCC pop_options")
#else
#	define VI_FUNCNAME __func__
#	define VI_RESTRICT
#	define VI_NOINLINE
#	define VI_OPTIMIZE_OFF
#	define VI_OPTIMIZE_ON
#endif
 
// Language feature abstraction layer
// Unifies C and C++ attributes for consistent API definitions.
// Provides:
//   - VI_NODISCARD : warn if result of function is unused
//   - VI_NOEXCEPT  : declare function as non-throwing
//   - VI_DEFAULT(v): set default argument value (C++ only)
#ifdef __cplusplus
#	define VI_NODISCARD [[nodiscard]]
#	define VI_NOEXCEPT noexcept
#	define VI_DEFAULT(v) =(v)
#elif defined(__GNUC__) || defined(__clang__)
#	define VI_NODISCARD __attribute__((warn_unused_result))
#	define VI_NOEXCEPT __attribute__((nothrow))
#	define VI_DEFAULT(v)
#else
#	define VI_NODISCARD
#	define VI_NOEXCEPT
#	define VI_DEFAULT(v)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t VI_TM_RESULT;
typedef uint32_t VI_TM_FLAGS;
typedef double VI_TM_FP; // Floating-point type used for timing calculations, typically double precision.
typedef size_t VI_TM_SIZE; // Size type used for counting events, typically size_t.
typedef uint64_t VI_TM_TICK; // !!! UNSIGNED !!! Represents a tick count (typically from a high-resolution timer). VI_TM_TICK and VI_TM_TDIFF are always unsigned to handle timer wraparound safely.
typedef VI_TM_TICK VI_TM_TDIFF; // !!! UNSIGNED !!! Represents a difference between two tick counts (duration). Do NOT compare to zero as signed. If a signed value is needed (e.g. for debugging/printing), cast explicitly.
typedef struct vi_tmMeasurement_t *VI_TM_HMEAS; // Opaque handle to a measurement entry.
typedef struct vi_tmMeasurementsJournal_t *VI_TM_HJOUR; // Opaque handle to a measurements journal.
typedef VI_TM_RESULT (VI_TM_CALL *vi_tmMeasEnumCb_t)(VI_TM_HMEAS meas, void* ctx); // Callback type for enumerating measurements; returning non-zero aborts enumeration.
typedef VI_TM_RESULT (VI_SYS_CALL *vi_tmReportCb_t)(const char* str, void* ctx); // Callback type for report function. ABI must be compatible with std::fputs!

// vi_tmStats_t: Structure holding statistics for a timing measurement.
// This structure is used to store the number of calls, total time spent, and other statistical data for a measurement.
// !!!Use the vi_tmStatsReset function to reset the structure to its initial state!!!
typedef struct vi_tmStats_t
{	VI_TM_SIZE calls_;		// The number of times the measurement was invoked.
#if VI_TM_STAT_USE_RAW
	VI_TM_SIZE cnt_;		// The number of all measured events, including discarded ones.
	VI_TM_TDIFF sum_;		// Total time spent measuring all events, in ticks.
#endif
#if VI_TM_STAT_USE_RMSE
	VI_TM_SIZE flt_calls_;	// Filtered! Number of invokes processed.
	VI_TM_FP flt_cnt_;		// Filtered! Number of events counted.
	VI_TM_FP flt_avg_;		// Filtered! Current average time taken per processed events. In ticks.
	VI_TM_FP flt_ss_;		// Filtered! Current sum of squares. In ticks.
#endif
#if VI_TM_STAT_USE_MINMAX
	// The minimum and maximum times are represented with a floating point because they can be initialized with the average of the batch.
	VI_TM_FP min_; //!!!! VI_TM_FP_POSITIVE_INF - initially!!! Minimum time taken for a single event, in ticks.
	VI_TM_FP max_; //!!!! VI_TM_FP_NEGATIVE_INF - initially!!! Maximum time taken for a single event, in ticks.
#endif
} vi_tmStats_t;

// Positive and negative infinity constants used for min/max statistics calculations.
#if VI_TM_STAT_USE_MINMAX
VI_TM_API extern const VI_TM_FP VI_TM_FP_POSITIVE_INF;
VI_TM_API extern const VI_TM_FP VI_TM_FP_NEGATIVE_INF;
#endif

// vi_tmInfo_e: Enumeration for various timing information types used in the vi_timing library.
// Each value corresponds to a specific static information query, such as version, build type, or timing characteristics.
// The return type for each enum value is indicated in the comment.
typedef enum vi_tmInfo_e
{	vi_tmInfoVer,          // const unsigned*: Version number of the library.
	vi_tmInfoBuildNumber,  // const unsigned*: Build number of the library.
	vi_tmInfoVersion,      // const char*: Full version string of the library.
	vi_tmInfoResolution,   // const double*: Clock resolution in ticks.
	vi_tmInfoDuration,     // const double*: Measure duration with cache in ticks.
	vi_tmInfoDurationEx,  // const double*: Measure duration in ticks.
	vi_tmInfoOverhead,     // const double*: Clock overhead in ticks.
	vi_tmInfoUnit,         // const double*: Seconds per tick (time unit).
	vi_tmInfoGitDescribe, // const char*: Git describe string, e.g., "v0.10.0-3-g96b37d4-dirty".
	vi_tmInfoGitCommit,   // const char*: Git commit hash, e.g., "96b37d49d235140e86f6f6c246bc7f166ab773aa".
	vi_tmInfoGitDateTime, // const char*: Git commit date and time, e.g., "2025-07-26 13:56:02 +0300".
	vi_tmInfoFlags,        // const unsigned*: Flags for controlling the library behavior.
	vi_tmInfoCount_,       // Number of information types.
} vi_tmInfo_e;

// vi_tmReportFlags_e: Flags for controlling the formatting and content of timing reports.
// These flags allow customization of sorting, display options, and report details.
typedef enum vi_tmReportFlags_e
{
	vi_tmSortByTime		= 0x00, // sort by time spent on the measurement.
	vi_tmSortByName		= 0x01, // sort by measurement name.
	vi_tmSortBySpeed	= 0x02, // sort by average time per event (speed).
	vi_tmSortByAmount	= 0x03, // sort by the number of events measured.
	vi_tmSortMask		= 0x03,

	vi_tmSortAscending			= 1 << 3, // sort in ascending order.

	vi_tmShowOverhead			= 1 << 4, // show the overhead of the clock.
	vi_tmShowUnit				= 1 << 5, // show the time unit (seconds per tick).
	vi_tmShowDuration			= 1 << 6, // show the duration of the measurement in seconds.
	vi_tmShowDurationEx			= 1 << 7, // show the duration, including overhead costs, in seconds.
	vi_tmShowResolution			= 1 << 8, // show the clock resolution in seconds.
	vi_tmShowAux				= 1 << 9, // show auxiliary information such as overhead.
	vi_tmShowMask				= 0x03F0, // Mask for all show flags.

	vi_tmHideHeader				= 1 << 10, // If set, the report will not show the header with column names.
	vi_tmDoNotSubtractOverhead	= 1 << 11, // If set, the overhead is not subtracted from the measured time in report.
	vi_tmDoNotReport			= 1 << 12, // If set, no report will be generated.

	vi_tmReportFlagsMask		= 0x1FFF,
	vi_tmReportDefault			= vi_tmShowResolution | vi_tmShowDuration | vi_tmSortByTime,
} vi_tmReportFlags_e;

typedef enum vi_tmInitFlags_e
{	vi_tmInitWarmup			= 1 << 0,
	vi_tmInitThreadYield	= 1 << 1,
	vi_tmInitFlagsMask		= 0x03,
} vi_tmInitFlags_e;

typedef enum vi_tmStatus_e
{
	vi_tmDebug			= 1 << 0,
	vi_tmShared			= 1 << 1,
	vi_tmThreadsafe		= 1 << 2,
	vi_tmStatUseBase	= 1 << 3,
	vi_tmStatUseRMSE	= 1 << 4,
	vi_tmStatUseFilter	= 1 << 5,
	vi_tmStatUseMinMax	= 1 << 6,
	vi_tmStatusMask		= 0x7F,
} vi_tmStatus_e;

#define VI_TM_HGLOBAL ((VI_TM_HJOUR)-1) // Global journal handle, used for global measurements.

// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
/// <summary>
/// This function is used to measure time intervals with fine precision.
/// </summary>
/// <returns>A current tick count.</returns>
VI_NODISCARD VI_TM_API VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) VI_NOEXCEPT;

/// <summary>
/// Initializes the timing system and prepares it for use.
/// </summary>
/// <param name="title">
/// The report title that will be displayed when generating statistics.
/// Default: "Timing report:\n".
/// </param>
/// <param name="report_flags">
/// A set of flags that define the contents and format of the report 
/// (for example, showing timer resolution, duration, or sorting by name).
/// Default: vi_tmReportDefault.
/// </param>
/// <param name="flags">
/// Additional flags to configure the library behavior.
/// Default: 0.
/// </param>
/// <returns>
/// Result code of the initialization (VI_TM_RESULT).
/// </returns>
VI_TM_API VI_TM_RESULT VI_TM_CALL vi_tmInit
(	const char* title VI_DEFAULT("Timing report:\n"),
	VI_TM_FLAGS report_flags VI_DEFAULT(vi_tmReportDefault),
	VI_TM_FLAGS flags VI_DEFAULT(0)
);

/// <summary>
/// Shuts down the timing system and releases all associated resources.
/// </summary>
VI_TM_API void VI_TM_CALL vi_tmShutdown();

/// <summary>
/// Creates a new journal object and returns a handle to it.
/// </summary>
/// <returns>A handle to the newly created journal object, or nullptr if memory allocation fails.</returns>
VI_NODISCARD VI_TM_API VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate();

/// <summary>
/// Resets but does not delete all entries in the journal. All entry handles remain valid.
/// </summary>
/// <param name="j">The handle to the journal to reset.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmJournalReset(VI_TM_HJOUR j) VI_NOEXCEPT;

/// <summary>
/// Closes and deletes a journal handle. All descriptors associated with the journal become invalid.
/// </summary>
/// <param name="j">The handle to the journal to be closed and deleted.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmJournalClose(VI_TM_HJOUR j);
	
/// <summary>
/// Retrieves a handle to the measurement associated with the given name, creating it if it does not exist.
/// Handle does not need to be released; it will remain valid as long as the journal exists.
/// </summary>
/// <param name="j">The handle to the journal containing the measurement.</param>
/// <param name="name">The name of the measurement entry to retrieve.</param>
/// <returns>A handle to the specified measurement entry within the journal.</returns>
VI_NODISCARD VI_TM_API VI_TM_HMEAS VI_TM_CALL vi_tmJournalGetMeas(
	VI_TM_HJOUR j,
	const char *name
);

/// <summary>
/// Invokes a callback function for each measurement entry in the journal, allowing early interruption.
/// </summary>
/// <param name="j">The handle to the journal containing the measurements.</param>
/// <param name="fn">A callback function to be called for each measurement. It receives a handle to the measurement and the user-provided data pointer.</param>
/// <param name="ctx">A pointer to user-defined data that is passed to the callback function.</param>
/// <returns>Returns 0 if all measurements were processed. If the callback returns a non-zero value, iteration stops and that value is returned.</returns>
VI_TM_API VI_TM_RESULT VI_TM_CALL vi_tmJournalEnumerateMeas(
	VI_TM_HJOUR j,
	vi_tmMeasEnumCb_t fn,
	void* ctx
);

/// <summary>
/// Performs a measurement replenishment operation by adding the total duration and number of measured events.
/// </summary>
/// <param name="m">A handle to the measurement to be updated.</param>
/// <param name="dur">The duration value to add to the measurement.</param>
/// <param name="cnt">The number of measured events.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmMeasurementAdd(
	VI_TM_HMEAS m,
	VI_TM_TDIFF dur,
	VI_TM_SIZE cnt VI_DEFAULT(1)
) VI_NOEXCEPT;

/// <summary>
/// Merges the statistics from the given source measurement stats into the specified measurement handle.
/// </summary>
/// <param name="m">A handle to the measurement object to be updated.</param>
/// <param name="src">Pointer to the source measurement statistics to merge.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmMeasurementMerge(VI_TM_HMEAS m, const vi_tmStats_t* src) VI_NOEXCEPT;

/// <summary>
/// Retrieves measurement information from a VI_TM_HMEAS object, including its name and statistics.
/// </summary>
/// <param name="m">The measurement handle from which to retrieve information.</param>
/// <param name="name">Pointer to a string pointer that will receive the name of the measurement. Can be nullptr if not needed.</param>
/// <param name="dst">Pointer to a vi_tmStats_t structure that will receive the measurement statistics. Can be nullptr if not needed.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmMeasurementGet(VI_TM_HMEAS m, const char **name, vi_tmStats_t *dst);

/// <summary>
/// Resets the measurement state for the specified measurement handle. The handle remains valid.
/// </summary>
/// <param name="meas">The measurement handle whose state should be reset.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmMeasurementReset(VI_TM_HMEAS m);

/// <summary>
/// Updates the given measurement statistics structure by adding a duration and count.
/// </summary>
/// <param name="dst">Pointer to the destination measurement statistics structure to update.</param>
/// <param name="dur">The duration value to add to the statistics.</param>
/// <param name="cnt">The number of measured events.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmStatsAdd(
	vi_tmStats_t *dst,
	VI_TM_TDIFF dur,
	VI_TM_SIZE cnt VI_DEFAULT(1)
) VI_NOEXCEPT;

/// <summary>
/// Merges the statistics from the source measurement statistics structure into the destination.
/// </summary>
/// <param name="dst">Pointer to the destination measurement statistics structure to update.</param>
/// <param name="src">Pointer to the source measurement statistics structure to merge.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmStatsMerge(
	vi_tmStats_t* VI_RESTRICT dst,
	const vi_tmStats_t* VI_RESTRICT src
) VI_NOEXCEPT;

/// <summary>
/// Resets the given measurement statistics structure to its initial state.
/// </summary>
/// <param name="m">Pointer to the measurement statistics structure to reset.</param>
/// <returns>This function does not return a value.</returns>
VI_TM_API void VI_TM_CALL vi_tmStatsReset(vi_tmStats_t *m) VI_NOEXCEPT;

/// <summary>
/// Checks if the given measurement statistics structure contains valid data.
/// Returns zero if valid.
/// </summary>
VI_NODISCARD VI_TM_API VI_TM_RESULT VI_TM_CALL vi_tmStatsIsValid(const vi_tmStats_t *m) VI_NOEXCEPT;

/// <summary>
/// Retrieves static information about the timing module based on the specified info type.
/// </summary>
/// <param name="info">The type of information to retrieve, specified as a value of the vi_tmInfo_e enumeration.</param>
/// <returns>A pointer to the requested static information. The type of the returned data depends on the info parameter and may point to an unsigned int, a double, or a null-terminated string. Returns nullptr if the info type is not recognized.</returns>
VI_NODISCARD VI_TM_API const void* VI_TM_CALL vi_tmStaticInfo(VI_TM_FLAGS info);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

/// <summary>
/// Report callback function. Generates and prints a timing report for the global journal.
/// </summary>
/// <param name="title">The title to display at the top of the report. Defaults to "Timing report:\n".</param>
/// <param name="flags">Flags controlling report formatting and content. Defaults to showing resolution, duration, and sorting by name.</param>
/// <returns>Returns the total number of characters written, or a negative value if an error occurs.</returns>
VI_TM_API VI_TM_RESULT VI_TM_CALL vi_tmGlobalSetReporter
(   const char *title VI_DEFAULT("Timing report:\n"),
	VI_TM_FLAGS flags VI_DEFAULT(vi_tmReportDefault)
);

/// <summary>
/// Default report callback function. Writes the given string to the standard output stream (or the debugger in Windows).
/// </summary>
/// <param name="str">The string to output.</param>
/// <param name="ignored"> A pointer to user-defined data, which is ignored in this default implementation.</param>
/// <returns>On success, returns a non-negative value.</returns>
VI_TM_API VI_TM_RESULT VI_SYS_CALL vi_tmReportCb(const char *str, void *ignored VI_DEFAULT(NULL));

/// <summary>
/// Generates a report for the specified journal handle, using a callback function to output the report data.
/// </summary>
/// <param name="j">The handle to the journal whose data will be reported.</param>
/// <param name="flags">Flags that control the formatting and content of the report.</param>
/// <param name="cb">A callback function used to output each line of the report. If nullptr, defaults to writing to a FILE* stream.</param>
/// <param name="ctx">A pointer to user data passed to the callback function. If fn is nullptr and ctx is nullptr, defaults to stdout.</param>
/// <returns>The total number of characters written by the report, or a negative value if an error occurs.</returns>
VI_TM_API VI_TM_RESULT VI_TM_CALL vi_tmReport(
	VI_TM_HJOUR j,
	VI_TM_FLAGS flags VI_DEFAULT(vi_tmReportDefault),
	vi_tmReportCb_t cb VI_DEFAULT(vi_tmReportCb),
	void* ctx VI_DEFAULT(NULL)
);
// Auxiliary functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#define VI_STRINGIZE_AUX(x) #x
#define VI_STRINGIZE(x) VI_STRINGIZE_AUX(x)

#ifdef __cplusplus
} // extern "C"
#	if __has_include("vi_timing.hpp")
#		include "vi_timing.hpp"
#	endif
#endif

#if VI_TM_STAT_USE_RAW || VI_TM_STAT_USE_RMSE || VI_TM_STAT_USE_FILTER || VI_TM_STAT_USE_MINMAX || VI_TM_THREADSAFE || VI_TM_SHARED || VI_TM_DEBUG
#	if VI_TM_STAT_USE_RAW
#		define VI_TM_S_STAT_USE_RAW "r"
#	else
#		define VI_TM_S_STAT_USE_RAW
#	endif
#	if VI_TM_STAT_USE_RMSE
#		define VI_TM_S_STAT_USE_RMSE "e"
#	else
#		define VI_TM_S_STAT_USE_RMSE
#	endif
#	if VI_TM_STAT_USE_FILTER
#		define VI_TM_S_STAT_USE_FILTER "f"
#	else
#		define VI_TM_S_STAT_USE_FILTER
#	endif
#	if VI_TM_STAT_USE_MINMAX
#		define VI_TM_S_STAT_USE_MINMAX "m"
#	else
#		define VI_TM_S_STAT_USE_MINMAX
#	endif
#	if VI_TM_THREADSAFE
#		define VI_TM_S_THREADSAFE "t"
#	else
#		define VI_TM_S_THREADSAFE
#	endif
#	if VI_TM_SHARED
#		define VI_TM_S_SHARED "s"
#	else
#		define VI_TM_S_SHARED
#	endif
#	if VI_TM_DEBUG
#		define VI_TM_S_DEBUG "d"
#	else
#		define VI_TM_S_DEBUG
#	endif
#
#	define VI_TM_FLAGS_SUFFIX \
		"_" \
		VI_TM_S_STAT_USE_RAW \
		VI_TM_S_STAT_USE_RMSE \
		VI_TM_S_STAT_USE_FILTER \
		VI_TM_S_STAT_USE_MINMAX \
		VI_TM_S_THREADSAFE \
		VI_TM_S_SHARED \
		VI_TM_S_DEBUG
#else
#	define VI_TM_FLAGS_SUFFIX ""
#endif

#ifdef VI_TM_VERSION_MAJOR
#	ifdef VI_TM_VERSION_MINOR
#		define VI_TM_VERSION_SUFFIX "-" VI_STRINGIZE(VI_TM_VERSION_MAJOR) "." VI_STRINGIZE(VI_TM_VERSION_MINOR)
#	else
#		define VI_TM_VERSION (VI_TM_VERSION_MAJOR * 1000U * 10000U)
#		define VI_TM_VERSION_SUFFIX "-" VI_STRINGIZE(VI_TM_VERSION_MAJOR)
#	endif
#else
#	define VI_TM_VERSION_SUFFIX ""
#endif

#define VI_TM_LIB_SUFFIX VI_TM_FLAGS_SUFFIX VI_TM_VERSION_SUFFIX

#if !defined(VI_TM_DISABLE) && defined(_MSC_VER) && !VI_TM_EXPORTS
#	pragma comment(lib, "vi_timing" VI_TM_LIB_SUFFIX ".lib")
#endif

#endif // #ifndef VI_TIMING_VI_TIMING_C_H
