#ifndef VI_TIMING_SOURCE_VERSION_H
#	define VI_TIMING_SOURCE_VERSION_H
#	pragma once

#include <cstdint>
#include <string_view>

namespace misc
{
#if __has_include("version.out")
#	include "version.out" // Include the generated version data.
#else
	constexpr unsigned VI_TM_VERSION_MAJOR = 0U;
	constexpr unsigned VI_TM_VERSION_MINOR = 0U;
	constexpr unsigned VI_TM_VERSION_PATCH = 0U;

	constexpr std::string_view VI_TM_GIT_DESCRIBE = "";
	constexpr std::string_view VI_TM_GIT_COMMIT = "unknown";
	constexpr std::string_view VI_TM_GIT_DATETIME = "unknown";
#endif
	// Build Number Generator: Formats the build number using the compilation date and time. Example: 2506170933U.
	// Updates the global variable based on the last compilation time.
	std::uint32_t build_number_updater(const char(&date)[12], const char(&time)[9]);
	// Next line will be called in each translation unit.
	static const std::uint32_t dummy_build_number_updater = build_number_updater(__DATE__, __TIME__);

	// Returns a number based on the date and time of the last compilation.
	std::uint32_t build_number_get();
}

#endif // #ifndef VI_TIMING_SOURCE_VERSION_H
