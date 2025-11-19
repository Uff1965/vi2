// vi_timing_proxy.h - Proxy header for the vi_timing library.
// This file provides a fallback implementation if the vi_timing library is not available.
// Copy this file to your project and include it instead of <vi_timing/vi_timing.h>.

#ifndef VI_TIMING_PROXY_HPP
#	define VI_TIMING_PROXY_HPP
#	pragma once
#
#	if !defined(VI_TM_DISABLE) && !__has_include(<vi_timing/vi_timing.hpp>)
#		define VI_TM_DISABLE // Timing functions are disabled because the header 'vi_timing.h' is not available.
#	endif
#
#	ifndef VI_TM_DISABLE
#		include <vi_timing/vi_timing.hpp>
#	else
#		// Auxiliary macros for generating a unique identifier.
#		define VI_ID __LINE__
#		define VI_STR_CONCAT_AUX( a, b ) a##b // Concatenate two tokens without expansion
#		define VI_STR_CONCAT( a, b ) VI_STR_CONCAT_AUX( a, b ) // Concatenate two tokens
#		define VI_UNIC_ID( prefix ) VI_STR_CONCAT( prefix, VI_ID ) // Unique identifier macro
#
#		// Fallback macros for timing functions
#		define VI_TM_H(h, ...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_SH(h, ...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_FUNC_H(h) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_REPORT_H(h, ...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_RESET_H(h, ...) const int VI_UNIC_ID(vi_tm__) = 0
#		// Fallback macros for global registry timing functions
#		define VI_TM(...) VI_TM_H(0, __VA_ARGS__)
#		define VI_TM_S(...) VI_TM_SH(0, __VA_ARGS__)
#		define VI_TM_FUNC VI_TM_FUNC_H(V0)
#		define VI_TM_REPORT(...) VI_TM_REPORT_H(0, __VA_ARGS__)
#		define VI_TM_RESET(...) VI_TM_RESET_h(0, __VA_ARGS__)
#		// Fallback macro for full version string and global init
#		define VI_TM_FULLVERSION ""
#		define VI_TM_GLOBALINIT(...) 0
#	endif
#endif // #ifndef VI_TIMING_PROXY_HPP
