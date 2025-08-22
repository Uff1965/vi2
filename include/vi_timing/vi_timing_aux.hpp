#ifndef VI_SRC_VI_TIMING_AUX_HPP
#define VI_SRC_VI_TIMING_AUX_HPP
#pragma once

#include "vi_timing_aux.h" // Include the header for vi_tmF2A function.
#include <string> // Include string for to_string function.

namespace vi_tm
{
	inline std::string to_string(double val, unsigned char sig = 2U, unsigned char dec = 1U)
	{	std::string result;
		result.resize(sig + (9 + 1 + 1), '\0'); // "-00S.Se-308"
		const auto len = vi_tmF2A(result.data(), static_cast<unsigned char>(result.size()), val, sig, dec);
		result.resize(len - 1);
		return result;
	}
} // namespace vi_tm

#endif // #ifndef VI_SRC_VI_TIMING_AUX_HPP
