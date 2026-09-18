// DiffusionCallbackUtils.hpp
#pragma once
#include "stable-diffusion.h"
#include <iostream>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <cstring>

namespace GUI {
    struct ProgressData {
        std::atomic<int> currentStep{ 0 };
        std::atomic<int> totalSteps{ 0 };
        std::atomic<float> currentTime{ 0.0f };
        std::atomic<bool> isProcessing{ false };
    };

    struct PreviewFrame {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::shared_ptr<unsigned char[]> data;
        uint64_t sequence = 0;
        bool valid() const { return data && width > 0 && height > 0 && channels > 0; }
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

        static void SetPreviewMode(int previewMode, int previewInterval);

        static void PreviewCallback(int step, int frame_count, sd_image_t* frames,
            bool is_noisy, void* data);

        static PreviewFrame GetLatestPreview();
        static uint64_t GetPreviewSequence();
        static void ClearPreview();

    private:
        static ProgressData progressData;
        static std::mutex mutex;
        static int m_logLevel;

        static PreviewFrame previewFrame;
        static std::mutex previewMutex;
        static std::atomic<uint64_t> previewSequence;
    };
}