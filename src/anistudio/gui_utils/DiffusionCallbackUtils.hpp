#pragma once

#include "stable-diffusion.h"
#include <iostream>

namespace GUI {
	struct ProgressData {
		int currentStep = 0;
		int totalSteps = 0;
		float currentTime = 0.0f;
		bool isProcessing = false;
	};

	class DiffusionCallbackUtils {
	public:
		// Get the global progress data instance
		static ProgressData& GetProgressData();

		// Initialize logging and progress callbacks
		static void InitializeCallbacks();

		// Logging callback for stable-diffusion
		static void LogCallback(sd_log_level_t level, const char* text, void* data);

		// Progress callback for stable-diffusion
		static void ProgressCallback(int step, int steps, float time, void* data);

	private:
		static ProgressData progressData;
	};
}