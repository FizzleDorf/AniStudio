#pragma once
#include "stable-diffusion.h"
#include <iostream>
#include <mutex>
#include <atomic>

namespace GUI {
    struct ProgressData {
        std::atomic<int> currentStep{ 0 };
        std::atomic<int> totalSteps{ 0 };
        std::atomic<float> currentTime{ 0.0f };
        std::atomic<bool> isProcessing{ false };
    };

    class DiffusionCallbackUtils {
    public:
        static ProgressData& GetProgressData();
        static void InitializeCallbacks();
        static void LogCallback(sd_log_level_t level, const char* text, void* data);
        static void ProgressCallback(int step, int steps, float time, void* data);
        static void TestCallbacks();

        static void SetLogLevel(int level);
        static int GetLogLevel();

    private:
        static ProgressData progressData;
        static std::mutex mutex;
        static int m_logLevel;
    };
}