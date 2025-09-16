#include "DiffusionCallbackUtils.hpp"

namespace GUI {
    ProgressData DiffusionCallbackUtils::progressData;
    std::mutex DiffusionCallbackUtils::mutex;

    ProgressData& DiffusionCallbackUtils::GetProgressData() {
        std::lock_guard<std::mutex> lock(mutex);
        return progressData;
    }

    void DiffusionCallbackUtils::InitializeCallbacks() {
        std::cout << "[DEBUG] Setting SD callbacks..." << std::endl;
        
        // Check if functions are available
        if (!sd_set_log_callback || !sd_set_progress_callback) {
            std::cerr << "[ERROR] SD callback functions not available!" << std::endl;
            return;
        }
        
        sd_set_log_callback(LogCallback, nullptr);
        sd_set_progress_callback(ProgressCallback, nullptr);
        std::cout << "[DEBUG] SD callbacks set successfully" << std::endl;
    }

    void DiffusionCallbackUtils::LogCallback(sd_log_level_t level, const char* text, void* data) {
        std::cout << "[SD_LOG] " << text << std::flush;
    }

    void DiffusionCallbackUtils::ProgressCallback(int step, int steps, float time, void* data) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            progressData.currentStep = step;
            progressData.totalSteps = steps;
            progressData.currentTime = time;
            progressData.isProcessing = (steps > 0);
        }
        
        // Force console output
        std::cout << "[PROGRESS] Step " << step << "/" << steps 
                  << " | Time: " << time << "s" << std::endl;
        std::cout.flush();
    }

    void DiffusionCallbackUtils::TestCallbacks() {
        std::cout << "[TEST] Testing callback system..." << std::endl;
        LogCallback(SD_LOG_INFO, "Test log message\n", nullptr);
        ProgressCallback(5, 10, 2.5f, nullptr);
        
        const auto& data = GetProgressData();
        std::cout << "[TEST] Progress data: " << data.currentStep.load() 
                  << "/" << data.totalSteps.load() << std::endl;
    }
}