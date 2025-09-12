#include "DiffusionCallbackUtils.hpp"

namespace GUI {

	// Static member definition
	ProgressData DiffusionCallbackUtils::progressData;

	ProgressData& DiffusionCallbackUtils::GetProgressData() {
		return progressData;
	}

	void DiffusionCallbackUtils::InitializeCallbacks() {
		sd_set_log_callback(LogCallback, nullptr);
		sd_set_progress_callback(ProgressCallback, nullptr);
	}

	void DiffusionCallbackUtils::LogCallback(sd_log_level_t level, const char* text, void* data) {
		switch (level) {
		case SD_LOG_DEBUG:
			std::cout << "[DEBUG]: " << text;
			break;
		case SD_LOG_INFO:
			std::cout << "[INFO]: " << text;
			break;
		case SD_LOG_WARN:
			std::cout << "[WARNING]: " << text;
			break;
		case SD_LOG_ERROR:
			std::cerr << "[ERROR]: " << text;
			break;
		default:
			std::cerr << "[UNKNOWN LOG LEVEL]: " << text;
			break;
		}
	}

	void DiffusionCallbackUtils::ProgressCallback(int step, int steps, float time, void* data) {
		progressData.currentStep = step;
		progressData.totalSteps = steps;
		progressData.currentTime = time;
		progressData.isProcessing = (steps > 0);
		std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
	}
}