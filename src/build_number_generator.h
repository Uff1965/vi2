#ifndef SRC_BUILD_NUMBER_GENERATOR_H
#	define SRC_BUILD_NUMBER_GENERATOR_H
#	pragma once

#include <cstdint>

namespace misc
{
	// Build Number Generator: Formats the build number using the compilation date and time. Example: 2506170933U.
	// Updates the global variable based on the last compilation time.
	std::uint32_t build_number_updater(const char(&date)[12], const char(&time)[9]);
	// Next line will be called in each translation unit.
	static const std::uint32_t dummy_build_number_updater = build_number_updater(__DATE__, __TIME__);

	// Returns a number based on the date and time of the last compilation.
	std::uint32_t build_number_get();
}

#endif // #ifndef SRC_BUILD_NUMBER_GENERATOR_H
