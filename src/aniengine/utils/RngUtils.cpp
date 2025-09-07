/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

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