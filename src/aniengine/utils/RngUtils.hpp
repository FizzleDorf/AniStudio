#pragma once

#pragma once

#include "rng.hpp"
#include <random>
#include <iostream>

namespace Utils {

	// Forward declarations for global RNG state
	extern std::random_device rd;
	extern STDDefaultRNG rng;
	extern bool initialized;

	// Generate a random seed using STDDefaultRNG from sdcpp
	uint64_t generateRandomSeed();

} // namespace Utils