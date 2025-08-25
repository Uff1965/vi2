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

#	include <stdint.h> // uint64_t
#	include <stddef.h> // size_t

//*******************************************************************************************************************
// Library configuration options:

// Set VI_TM_DEBUG to TRUE to enable debug-mode assertions and diagnostics.
// Note: This may impact runtime performance.
// Library rebuild required
#if !defined(VI_TM_DEBUG) && !defined(NDEBUG)
#	define VI_TM_DEBUG 1 // Enable debug mode.
#endif

// Used high-performance timing methods (e.g., platform-specific optimizations like ASM).
// Set the VI_TM_USE_STDCLOCK macro to TRUE to switch to standard C11 `timespec_get()`.
// Requires library rebuild.
#ifndef VI_TM_USE_STDCLOCK
#	define VI_TM_USE_STDCLOCK 0
#endif

// If VI_TM_SHARED macro is TRUE, the library is a shared library.
#ifndef VI_TM_SHARED
#	define VI_TM_SHARED 0
#endif

// Set VI_TM_THREADSAFE to FALSE to disable thread safety.
// Improves performance in single-threaded applications.
// Library rebuild required
#ifndef VI_TM_THREADSAFE
#	define VI_TM_THREADSAFE 1
#endif

// Set VI_TM_STAT_USE_BASE macro to FALSE to skip basic statistics collection (cnt, sum).
// Library rebuild required
#ifndef VI_TM_STAT_USE_BASE
#	define VI_TM_STAT_USE_BASE 1
#endif

// Set the VI_TM_STAT_USE_FILTER macro to FALSE to skip correlation coefficient calculation
// and bounce filtering in measurements.
// Library rebuild required
#ifndef VI_TM_STAT_USE_FILTER
#	define VI_TM_STAT_USE_FILTER 1
#endif

// Set the VI_TM_STAT_USE_MINMAX macro to TRUE to store minimum and maximum measurement values.
// Library rebuild required
#ifndef VI_TM_STAT_USE_MINMAX
#	define VI_TM_STAT_USE_MINMAX 0
#endif

// If VI_TM_EXPORTS defined, the library is built as a shared and exports its functions.

//*******************************************************************************************************************

// Define: VI_SYS_CALL, VI_TM_CALL and VI_TM_API vvvvvvvvvvvvvv
#if defined(_MSC_VER)
#	ifdef _M_IX86 // x86 architecture
#		define VI_SYS_CALL __cdecl
#		define VI_TM_CALL __fastcall
#	else
#		define VI_SYS_CALL
#		define VI_TM_CALL
#	endif

#	if ! VI_TM_SHARED
#		define VI_TM_API

#		if VI_TM_DEBUG
#			pragma comment(lib, "vi_timing_d.lib")
#		else
#			pragma comment(lib, "vi_timing.lib")
#		endif
#	elif ! defined(VI_TM_EXPORTS)
#		define VI_TM_API __declspec(dllimport)

#		if VI_TM_DEBUG
#			pragma comment(lib, "vi_timing_sd.lib")
#		else
#			pragma comment(lib, "vi_timing_s.lib")
#		endif
#	else
#		define VI_TM_API __declspec(dllexport)
#	endif
#elif defined (__GNUC__) || defined(__clang__)
#	ifdef __i386__
#		define VI_SYS_CALL __attribute__((cdecl))
#		define VI_TM_CALL __attribute__((fastcall))
#	else
#		define VI_SYS_CALL
#		define VI_TM_CALL
#	endif

#	ifdef VI_TM_EXPORTS
#		define VI_TM_API __attribute__((visibility("default")))
#	else
#		define VI_TM_API
#	endif
#else
#	define VI_SYS_CALL
#	define VI_TM_DISABLE "Unknown compiler!"
#	define VI_TM_CALL
#	define VI_TM_API
#endif
// Define: VI_FUNCNAME, VI_SYS_CALL, VI_TM_CALL and VI_TM_API ^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary macros: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// VI_NOINLINE macro: This macro is used to prevent the compiler from inlining a function.
// The VI_OPTIMIZE_ON/OFF macros allow or disable compiler optimizations
// for specific code sections. They must appear outside of a function and takes 
// effect at the first function defined after the macros is seen.
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
 
#ifdef __cplusplus
extern "C" {
#	define VI_NODISCARD [[nodiscard]]
#	define VI_NOEXCEPT noexcept
#	define VI_DEF(v) =(v)
#else
#	define VI_NODISCARD
#	define VI_NOEXCEPT
#	define VI_DEF(v)
#endif

typedef double VI_TM_FP; // Floating-point type used for timing calculations, typically double precision.
typedef size_t VI_TM_SIZE; // Size type used for counting events, typically size_t.
typedef uint64_t VI_TM_TICK; // !!! UNSIGNED !!! Represents a tick count (typically from a high-resolution timer).
typedef VI_TM_TICK VI_TM_TDIFF; // !!! UNSIGNED !!! Represents a difference between two tick counts (duration).
typedef struct vi_tmMeasurement_t *VI_TM_HMEAS; // Opaque handle to a measurement entry.
typedef struct vi_tmMeasurementsJournal_t *VI_TM_HJOUR; // Opaque handle to a measurements journal.
typedef int (VI_TM_CALL *vi_tmMeasEnumCb_t)(VI_TM_HMEAS meas, void* ctx); // Callback type for enumerating measurements; returning non-zero aborts enumeration.
typedef int (VI_SYS_CALL *vi_tmReportCb_t)(const char* str, void* ctx); // Callback type for report function. ABI must be compatible with std::fputs!
typedef /*VI_NODISCARD*/ VI_TM_TICK (VI_TM_CALL vi_tmGetTicks_t)(void) VI_NOEXCEPT;

// vi_tmMeasurementStats_t: Structure holding statistics for a timing measurement.
// This structure is used to store the number of calls, total time spent, and other statistical data for a measurement.
// !!!Use the vi_tmMeasurementStatsReset function to reset the structure to its initial state!!!
typedef struct vi_tmMeasurementStats_t
{	VI_TM_SIZE calls_;		// The number of times the measurement was invoked.
#if VI_TM_STAT_USE_BASE
	VI_TM_SIZE cnt_;		// The number of all measured events, including discarded ones.
	VI_TM_TDIFF sum_;		// Total time spent measuring all events, in ticks.
#endif
#if VI_TM_STAT_USE_FILTER
	VI_TM_SIZE flt_calls_;	// Filtered! Number of invokes processed.
	VI_TM_FP flt_cnt_;		// Filtered! Number of events counted.
	VI_TM_FP flt_avg_;		// Filtered! Current average time taken per processed events. In ticks.
	VI_TM_FP flt_ss_;		// Filtered! Current sum of squares. In ticks.
#endif
#if VI_TM_STAT_USE_MINMAX
	// The minimum and maximum times are represented with a floating point because they can be initialized with the average of the batch.
	VI_TM_FP min_; //!!!! INFINITY - initially!!! Minimum time taken for a single event, in ticks.
	VI_TM_FP max_; //!!!! -INFINITY - initially!!! Maximum time taken for a single event, in ticks.
#endif
} vi_tmMeasurementStats_t;

// vi_tmInfo_e: Enumeration for various timing information types used in the vi_timing library.
// Each value corresponds to a specific static information query, such as version, build type, or timing characteristics.
// The return type for each enum value is indicated in the comment.
typedef enum vi_tmInfo_e
{	VI_TM_INFO_VER,          // const unsigned*: Version number of the library.
	VI_TM_INFO_BUILDNUMBER,  // const unsigned*: Build number of the library.
	VI_TM_INFO_VERSION,      // const char*: Full version string of the library.
	VI_TM_INFO_RESOLUTION,   // const double*: Clock resolution in ticks.
	VI_TM_INFO_DURATION,     // const double*: Measure duration with cache in ticks.
	VI_TM_INFO_DURATION_EX,  // const double*: Measure duration in ticks.
	VI_TM_INFO_OVERHEAD,     // const double*: Clock overhead in ticks.
	VI_TM_INFO_UNIT,         // const double*: Seconds per tick (time unit).
	VI_TM_INFO_GIT_DESCRIBE, // const char*: Git describe string, e.g., "v0.10.0-3-g96b37d4-dirty".
	VI_TM_INFO_GIT_COMMIT,   // const char*: Git commit hash, e.g., "96b37d49d235140e86f6f6c246bc7f166ab773aa".
	VI_TM_INFO_GIT_DATETIME, // const char*: Git commit date and time, e.g., "2025-07-26 13:56:02 +0300".
	VI_TM_INFO_FLAGS,        // const unsigned*: Flags for controlling the library behavior.
	VI_TM_INFO_COUNT_,       // Number of information types.
} vi_tmInfo_e;

// vi_tmReportFlags_e: Flags for controlling the formatting and content of timing reports.
// These flags allow customization of sorting, display options, and report details.
typedef enum vi_tmReportFlags_e
{
	vi_tmSortByTime = 0x00, // If set, the report will be sorted by time spent in the measurement.
	vi_tmSortByName = 0x01, // If set, the report will be sorted by measurement name.
	vi_tmSortBySpeed = 0x02, // If set, the report will be sorted by average time per event (speed).
	vi_tmSortByAmount = 0x03, // If set, the report will be sorted by the number of events measured.
	vi_tmSortMask = 0x07, // Mask for all sorting flags.

	vi_tmSortDescending = 0x00, // If set, the report will be sorted in descending order.
	vi_tmSortAscending = 0x08, // If set, the report will be sorted in ascending order.

	vi_tmShowOverhead = 0x0010, // If set, the report will show the overhead of the clock.
	vi_tmShowUnit = 0x0020, // If set, the report will show the time unit (seconds per tick).
	vi_tmShowDuration = 0x0040, // If set, the report will show the duration of the measurement in seconds.
	vi_tmShowDurationEx = 0x0080, // If set, the report will show the duration, including overhead costs, in seconds.
	vi_tmShowDurationNonThreadsafe = 0x0100, // If this parameter is set, the report will indicate the duration of the threadunsafe measurement.
	vi_tmShowResolution = 0x0200, // If set, the report will show the clock resolution in seconds.
	vi_tmShowAux = 0x0400, // If set, the report will show auxiliary information such as overhead.
	vi_tmShowMask = 0x7F0, // Mask for all show flags.

	vi_tmHideHeader = 0x0800, // If set, the report will not show the header with column names.
	vi_tmDoNotSubtractOverhead = 0x1000, // If set, the overhead is not subtracted from the measured time in report.

	vi_tmDebug = 0x01,
	vi_tmShared = 0x02,
	vi_tmThreadsafe = 0x04,
	vi_tmStatUseBase = 0x08,
	vi_tmStatUseFilter = 0x10,
	vi_tmStatUseMinMax = 0x20,

} vi_tmReportFlags_e;

#define VI_TM_HGLOBAL ((VI_TM_HJOUR)-1) // Global journal handle, used for global measurements.

// Main functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	extern VI_TM_API vi_tmGetTicks_t *vi_tmGetTicksPtr;

	/// <summary>
	/// This function is used to measure time intervals with fine precision.
	/// </summary>
	/// <returns>A current tick count.</returns>
	VI_NODISCARD inline VI_TM_TICK VI_TM_CALL vi_tmGetTicks(void) VI_NOEXCEPT
	{	return vi_tmGetTicksPtr();
	}

	/// <summary>
	/// Initializes the global journal.
	/// </summary>
	/// <returns>If successful, returns 0.</returns>
	VI_TM_API int VI_TM_CALL vi_tmInit(vi_tmGetTicks_t *fn VI_DEF(NULL));

	/// <summary>
	/// Deinitializes the global journal.
	/// </summary>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmFinit(void);

	/// <summary>
	/// Creates a new journal object and returns a handle to it.
	/// </summary>
	/// <returns>A handle to the newly created journal object, or nullptr if memory allocation fails.</returns>
	VI_TM_API VI_NODISCARD VI_TM_HJOUR VI_TM_CALL vi_tmJournalCreate(
		unsigned flags VI_DEF(0U),
		void* reserved VI_DEF(NULL)
	);

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
	VI_TM_API VI_NODISCARD VI_TM_HMEAS VI_TM_CALL vi_tmMeasurement(
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
	VI_TM_API int VI_TM_CALL vi_tmMeasurementEnumerate(
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
		VI_TM_SIZE cnt VI_DEF(1)
	) VI_NOEXCEPT;

    /// <summary>
    /// Merges the statistics from the given source measurement stats into the specified measurement handle.
    /// </summary>
    /// <param name="m">A handle to the measurement object to be updated.</param>
    /// <param name="src">Pointer to the source measurement statistics to merge.</param>
    /// <returns>This function does not return a value.</returns>
    VI_TM_API void VI_TM_CALL vi_tmMeasurementMerge(
		VI_TM_HMEAS VI_RESTRICT m,
		const vi_tmMeasurementStats_t* VI_RESTRICT src
	) VI_NOEXCEPT;

    /// <summary>
    /// Retrieves measurement information from a VI_TM_HMEAS object, including its name and statistics.
    /// </summary>
    /// <param name="m">The measurement handle from which to retrieve information.</param>
    /// <param name="name">Pointer to a string pointer that will receive the name of the measurement. Can be nullptr if not needed.</param>
    /// <param name="dst">Pointer to a vi_tmMeasurementStats_t structure that will receive the measurement statistics. Can be nullptr if not needed.</param>
    /// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasurementGet(
		VI_TM_HMEAS VI_RESTRICT m,
		const char **name,
		vi_tmMeasurementStats_t* VI_RESTRICT dst
	);
	
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
	VI_TM_API void VI_TM_CALL vi_tmMeasurementStatsAdd(
		vi_tmMeasurementStats_t *dst,
		VI_TM_TDIFF dur,
		VI_TM_SIZE cnt VI_DEF(1)
	) VI_NOEXCEPT;

    /// <summary>
    /// Merges the statistics from the source measurement statistics structure into the destination.
    /// </summary>
    /// <param name="dst">Pointer to the destination measurement statistics structure to update.</param>
    /// <param name="src">Pointer to the source measurement statistics structure to merge.</param>
    /// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasurementStatsMerge(
		vi_tmMeasurementStats_t* VI_RESTRICT dst,
		const vi_tmMeasurementStats_t* VI_RESTRICT src
	) VI_NOEXCEPT;

	/// <summary>
	/// Resets the given measurement statistics structure to its initial state.
	/// </summary>
	/// <param name="m">Pointer to the measurement statistics structure to reset.</param>
	/// <returns>This function does not return a value.</returns>
	VI_TM_API void VI_TM_CALL vi_tmMeasurementStatsReset(vi_tmMeasurementStats_t *m) VI_NOEXCEPT;

	/// <summary>
	/// Retrieves static information about the timing module based on the specified info type.
	/// </summary>
	/// <param name="info">The type of information to retrieve, specified as a value of the vi_tmInfo_e enumeration.</param>
	/// <returns>A pointer to the requested static information. The type of the returned data depends on the info parameter and may point to an unsigned int, a double, or a null-terminated string. Returns nullptr if the info type is not recognized.</returns>
	VI_TM_API VI_NODISCARD const void* VI_TM_CALL vi_tmStaticInfo(vi_tmInfo_e info);
// Main functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Auxiliary functions: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

	/// <summary>
	/// Checks if the given measurement statistics structure contains valid data.
	/// Returns zero if valid.
	/// </summary>
	VI_TM_API VI_NODISCARD int VI_TM_CALL vi_tmMeasurementStatsIsValid(const vi_tmMeasurementStats_t *m) VI_NOEXCEPT;

    /// <summary>
    /// Default report callback function. Writes the given string to the specified output stream.
    /// </summary>
    /// <param name="str">The string to output.</param>
	/// <param name="ignored"> A pointer to user-defined data, which is ignored in this default implementation.</param>
    /// <returns>On success, returns a non-negative value.</returns>
    VI_TM_API int VI_SYS_CALL vi_tmReportCb(const char *str, void *ignored VI_DEF(NULL));

	/// <summary>
	/// Generates a report for the specified journal handle, using a callback function to output the report data.
	/// </summary>
	/// <param name="j">The handle to the journal whose data will be reported.</param>
	/// <param name="flags">Flags that control the formatting and content of the report.</param>
	/// <param name="cb">A callback function used to output each line of the report. If nullptr, defaults to writing to a FILE* stream.</param>
	/// <param name="ctx">A pointer to user data passed to the callback function. If fn is nullptr and ctx is nullptr, defaults to stdout.</param>
	/// <returns>The total number of characters written by the report, or a negative value if an error occurs.</returns>
	VI_TM_API int VI_TM_CALL vi_tmReport(
		VI_TM_HJOUR j,
		unsigned flags VI_DEF(0),
		vi_tmReportCb_t cb VI_DEF(vi_tmReportCb),
		void* ctx VI_DEF(NULL)
	);
// Auxiliary functions: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#	ifdef __cplusplus
} // extern "C"

#		include "vi_timing.hpp"
#	endif

#endif // #ifndef VI_TIMING_VI_TIMING_C_H
