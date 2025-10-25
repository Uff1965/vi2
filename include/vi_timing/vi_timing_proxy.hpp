// vi_timing_proxy.h - Proxy header for the vi_timing library.
// This file provides a fallback implementation if the vi_timing library is not available.
// Copy this file to your project and include it instead of <vi_timing/vi_timing.h>.

#ifndef VI_TIMING_PROXY_HPP
#	define VI_TIMING_PROXY_HPP
#	pragma once

#	if !defined(VI_TM_DISABLE) && !__has_include(<vi_timing/vi_timing.hpp>)
#		define VI_TM_DISABLE // Timing functions are disabled because the header 'vi_timing.h' is not available.
#	endif

#	ifndef VI_TM_DISABLE
#		include <vi_timing/vi_timing.hpp>
#	else
		// Auxiliary macros for generating a unique identifier.
#		define VI_ID __LINE__
#		define VI_STR_CONCAT_AUX( a, b ) a##b // Concatenate two tokens without expansion
#		define VI_STR_CONCAT( a, b ) VI_STR_CONCAT_AUX( a, b ) // Concatenate two tokens
#		define VI_UNIC_ID( prefix ) VI_STR_CONCAT( prefix, VI_ID ) // Unique identifier macro

		// Fallback macros for global registry timing functions.
#		define VI_TM_INIT(...) static const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM(...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_S(...) const int VI_UNIC_ID(vi_tm__) = 0
#		define VI_TM_FUNC ((void)0)
#		define VI_TM_REPORT(...) ((void)0)
#		define VI_TM_RESET(...) ((void)0)
#		define VI_TM_FULLVERSION ""
#	endif
#endif // #ifndef VI_TIMING_PROXY_HPP
