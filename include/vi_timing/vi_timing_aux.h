#ifndef VI_TIMING_VI_TM_MISC_H
#	define VI_TIMING_VI_TM_MISC_H
#	pragma once

#	include <vi_timing/vi_timing.h>

#ifdef __cplusplus
extern "C" {
#endif
	/// <summary>
	/// Performs a CPU warming routine by running computationally intensive tasks across multiple threads for a specified duration.
	/// </summary>
	/// <param name="threads">The number of threads to use for the warming routine. If zero or greater than the number of available hardware threads, the function uses the maximum available.</param>
	/// <param name="ms">The duration of the warming routine in milliseconds. If zero, the function returns immediately.</param>
	/// <returns>Returns zero if successful.</returns>
	VI_TM_API int VI_TM_CALL vi_WarmUp(unsigned threads VI_DEF(0), unsigned ms VI_DEF(500));

	/// <summary>
	/// Fixates the CPU affinity of the current thread.
	/// </summary>
	/// <returns>Returns zero if successful.</returns>
	VI_TM_API int VI_TM_CALL vi_CurrentThreadAffinityFixate(void);

	/// <summary>
	/// Restores the CPU affinity of the current thread to its previous state.
	/// </summary>
	/// <returns>Returns zero if successful.</returns>
	VI_TM_API int VI_TM_CALL vi_CurrentThreadAffinityRestore(void);

	/// <summary>
	/// Yields execution of the current thread, allowing other threads to run.
	/// </summary>
	VI_TM_API void VI_TM_CALL vi_ThreadYield(void) VI_NOEXCEPT;

	/// <summary>
	/// Converts a double value to a formatted string with SI suffix or scientific notation and copies it to a buffer.
	/// </summary>
	/// <param name="buff">Pointer to the destination buffer for the formatted string.</param>
	/// <param name="sz">Size of the destination buffer.</param>
	/// <param name="val">The value to convert.</param>
	/// <param name="sig">Number of significant digits to display.</param>
	/// <param name="dec">Number of decimal places to display.</param>
	/// <returns>The required buffer size (including null-terminator) for the formatted string.</returns>
	VI_TM_API VI_TM_SIZE VI_TM_CALL vi_tmF2A(char *buff, VI_TM_SIZE sz, VI_TM_FP val, unsigned char sig, unsigned char dec);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // #ifndef VI_TIMING_VI_TM_MISC_H
