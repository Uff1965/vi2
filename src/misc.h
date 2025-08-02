#ifndef VI_TIMING_SOURCE_INTERNAL_H
#	define VI_TIMING_SOURCE_INTERNAL_H
#	pragma once

#include <cassert>
#include <chrono>
#include <locale> // for std::numpunct
#include <string>
#include <string_view>
#ifdef __cpp_lib_source_location
#	include <source_location>
#endif

#define verify(v) [](bool b) constexpr noexcept { assert(b); return b; }(v) // Define for displaying the __FILE__ and __LINE__ during debugging.

namespace misc
{
#ifdef __cpp_lib_source_location
#	define VI_EXIT_SUCCESS (0) // Use zero as success code.
#	define VI_EXIT_FAILURE []() noexcept ->int { return(0 - std::source_location::current().line()); } () // Use negative line number as error code.
#else
#	define VI_EXIT_SUCCESS (0) // Use zero as success code.
#	define VI_EXIT_FAILURE (0 - __LINE__) // Use negative line number as error code.
#endif

	struct space_out final: std::numpunct<char>
	{	char do_thousands_sep() const override { return '\''; }  // separate with '
		std::string do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	struct properties_t
	{	std::chrono::duration<double> seconds_per_tick_; // [nanoseconds]
		double clock_overhead_ticks_; // Duration of one clock call [ticks]
		std::chrono::duration<double> duration_ex_threadsafe_;
		std::chrono::duration<double> duration_threadsafe_; // Duration of one measurement with preservation. [nanoseconds]
		double clock_resolution_ticks_; // [ticks]
		static const properties_t& props();
	private:
		properties_t();
		static const properties_t self_;
	};

	[[nodiscard]] std::string to_string(double d, unsigned char precision, unsigned char dec);
}

#endif // #ifndef VI_TIMING_SOURCE_INTERNAL_H
