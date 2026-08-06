#include "DiffusionCallbackUtils.hpp"
#include "ECS.h"
#include "SettingsSystem.hpp"
#include "SDCPPSettingsComponent.hpp"
#include <iostream>

namespace GUI {
    ProgressData DiffusionCallbackUtils::progressData;
    std::mutex DiffusionCallbackUtils::mutex;
    int DiffusionCallbackUtils::m_logLevel = 1; // default INFO

    ProgressData& DiffusionCallbackUtils::GetProgressData() {
        std::lock_guard<std::mutex> lock(mutex);
        return progressData;
    }

    void DiffusionCallbackUtils::InitializeCallbacks() {
        std::cout << "[DEBUG] Setting SD callbacks..." << std::endl;

        if (!sd_set_log_callback || !sd_set_progress_callback) {
            std::cerr << "[ERROR] SD callback functions not available!" << std::endl;
            return;
        }

        sd_set_log_callback(LogCallback, nullptr);
        sd_set_progress_callback(ProgressCallback, nullptr);
        std::cout << "[DEBUG] SD callbacks set successfully" << std::endl;
    }

    void DiffusionCallbackUtils::LogCallback(sd_log_level_t level, const char* text, void* data) {
        if (level < m_logLevel) return; // filter based on current log level
        const char* levelStr = "UNKNOWN";
        switch (level) {
        case SD_LOG_DEBUG: levelStr = "DEBUG"; break;
        case SD_LOG_INFO:  levelStr = "INFO";  break;
        case SD_LOG_WARN:  levelStr = "WARN";  break;
        case SD_LOG_ERROR: levelStr = "ERROR"; break;
        }
        std::cout << "[SD_LOG][" << levelStr << "] " << text << std::flush;
    }

    void DiffusionCallbackUtils::ProgressCallback(int step, int steps, float time, void* data) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            progressData.currentStep = step;
            progressData.totalSteps = steps;
            progressData.currentTime = time;
            progressData.isProcessing = (steps > 0);
        }

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

    void DiffusionCallbackUtils::SetLogLevel(int level) {
        if (level < 0) level = 0;
        if (level > 3) level = 3;
        m_logLevel = level;
        std::cout << "[SD_LOG] Log level set to " << level << std::endl;
    }

    int DiffusionCallbackUtils::GetLogLevel() {
        return m_logLevel;
    }
}