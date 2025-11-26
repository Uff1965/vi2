// vi_timing_proxy.h - Proxy header for the vi_timing library.
// Copy this file to your project and include it instead of <vi_timing/vi_timing.h>.
//
// Purpose:
// This proxy header provides a safe fallback when the vi_timing library is not available.
// - If <vi_timing/vi_timing.hpp> exists and VI_TM_DISABLE is not defined, the real
//   implementation is included.
// - Otherwise, lightweight stub macros are defined so that code using vi_timing
//   can still compile without modification, with zero runtime overhead.
//
// Key points:
// * VI_UNIC_ID(prefix) generates a unique identifier based on the current line number,
//   used to avoid name collisions in stub definitions.
// * All timing macros (VI_TM_H, VI_TM_SH, VI_TM_FUNC_H, etc.) expand to harmless
//   constants or no-ops, ensuring that instrumentation calls do not affect performance.
// * VI_TM_REPORT and VI_TM_RESET return dummy values or do nothing, so calls remain valid.
// * VI_TM_FULLVERSION and VI_TM_GLOBALINIT also expand to safe defaults.
//
// In short:
// This header guarantees that projects can be built even if the vi_timing library
// is missing, by replacing all timing hooks with no-op stubs. It allows developers
// to keep instrumentation code in place without introducing dependencies or overhead
// when the library is unavailable.

#ifndef VI_TIMING_PROXY_HPP
#	define VI_TIMING_PROXY_HPP
#	pragma once
#
#	if !defined(__has_include)
#		error "Compiler does not support __has_include. This proxy header requires __has_include support."
#	elif !defined(VI_TM_DISABLE) && !__has_include(<vi_timing/vi_timing.hpp>)
#		define VI_TM_DISABLE // Timing functions are disabled because the header 'vi_timing.h' is not available.
#	endif
#
#	ifndef VI_TM_DISABLE
#		include <vi_timing/vi_timing.hpp>
#	else
#		// Auxiliary macros for generating a unique identifier.
#		define VI_STR_CONCAT_AUX( a, b ) a##b
#		define VI_STR_CONCAT( a, b ) VI_STR_CONCAT_AUX( a, b )
#
#		if defined(__COUNTER__) // MSVC, GCC, Clang and some other compilers support __COUNTER__
#			define VI_UNIC_ID( prefix ) VI_STR_CONCAT(prefix, VI_STR_CONCAT(__LINE__, VI_STR_CONCAT(_, __COUNTER__)))
#		else // Fallback to __LINE__ only which may cause collisions if multiple expansions occur on the same line
#			define VI_UNIC_ID( prefix ) VI_STR_CONCAT(prefix, __LINE__)
#		endif
#
#		// Fallback macros for timing functions
#		define VI_TM_H(h, ...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_SH(h, ...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_FUNC_H(h) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_REPORT_H(h, ...) 0
#		define VI_TM_RESET_H(h, ...) (void)0
#		// Fallback macros for global registry timing functions
#		define VI_TM(...) VI_TM_H(0, __VA_ARGS__)
#		define VI_TM_S(...) VI_TM_SH(0, __VA_ARGS__)
#		define VI_TM_FUNC VI_TM_FUNC_H(0)
#		define VI_TM_REPORT(...) VI_TM_REPORT_H(0, __VA_ARGS__)
#		define VI_TM_RESET(...) VI_TM_RESET_H(0, __VA_ARGS__)
#		// Fallback macro for full version string and global init
#		define VI_TM_FULLVERSION ""
#		define VI_TM_GLOBALINIT(...) 0
#	endif
#endif // #ifndef VI_TIMING_PROXY_HPP
