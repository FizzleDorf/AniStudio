#include "RngUtils.hpp"

namespace Utils {

	// Global RNG state definitions - moved from header
	std::random_device rd;
	STDDefaultRNG rng;
	bool initialized = false;

	uint64_t generateRandomSeed()
	{
		// Seed the RNG with a random device if not already seeded
		if (!initialized)
		{
			rng.manual_seed(rd());
			initialized = true;
		}

		// Get random numbers
		std::vector<float> random_values = rng.randn(1);

		// Convert to a positive integer seed
		uint64_t seed = static_cast<uint64_t>(std::abs(random_values[0] * UINT32_MAX)) % INT32_MAX;
		return seed > 0 ? seed : 1; // Ensure seed is positive
	}

} // namespace Utils