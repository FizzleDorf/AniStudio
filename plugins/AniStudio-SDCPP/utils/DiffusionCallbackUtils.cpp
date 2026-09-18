// DiffusionCallbackUtils.cpp
#include "DiffusionCallbackUtils.hpp"
#include "ECS.h"
#include "SettingsSystem.hpp"
#include "SDCPPSettingsComponent.hpp"
#include <iostream>
#include <cstring>

namespace GUI {
    ProgressData DiffusionCallbackUtils::progressData;
    std::mutex DiffusionCallbackUtils::mutex;
    int DiffusionCallbackUtils::m_logLevel = 1;

    PreviewFrame DiffusionCallbackUtils::previewFrame;
    std::mutex DiffusionCallbackUtils::previewMutex;
    std::atomic<uint64_t> DiffusionCallbackUtils::previewSequence{ 0 };

    ProgressData& DiffusionCallbackUtils::GetProgressData() {
        std::lock_guard<std::mutex> lock(mutex);
        return progressData;
    }

    void DiffusionCallbackUtils::InitializeCallbacks() {
        std::cout << "[DEBUG] Setting SD callbacks..." << std::endl;

        sd_set_log_callback(LogCallback, nullptr);
        sd_set_progress_callback(ProgressCallback, nullptr);
        sd_set_preview_callback(PreviewCallback, PREVIEW_NONE, 0, true, false, nullptr);

        std::cout << "[DEBUG] SD callbacks set successfully" << std::endl;
    }

    void DiffusionCallbackUtils::LogCallback(sd_log_level_t level, const char* text, void* data) {
        if (level < m_logLevel) return;
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

    void DiffusionCallbackUtils::SetPreviewMode(int previewMode, int previewInterval) {
        preview_t mode = PREVIEW_NONE;
        switch (previewMode) {
        case 1: mode = PREVIEW_PROJ; break;
        case 2: mode = PREVIEW_TAE;  break;
        case 3: mode = PREVIEW_VAE;  break;
        default: mode = PREVIEW_NONE; break;
        }

        if (mode == PREVIEW_NONE) {
            sd_set_preview_callback(PreviewCallback, PREVIEW_NONE, 0,
                true, false, nullptr);
            return;
        }

        sd_set_preview_callback(PreviewCallback, mode, previewInterval,
            true, false, nullptr);

        std::cout << "[PREVIEW] mode=" << (int)mode
            << " interval=" << previewInterval << std::endl;
    }

    void DiffusionCallbackUtils::PreviewCallback(int step, int frame_count,
        sd_image_t* frames, bool is_noisy,
        void* data) {
        if (!frames || frame_count <= 0) return;
        sd_image_t& image = frames[0];
        if (!image.data || image.width == 0 || image.height == 0) return;
        if (image.channel != 1 && image.channel != 3 && image.channel != 4) return;

        const size_t byteCount =
            static_cast<size_t>(image.width) *
            static_cast<size_t>(image.height) *
            static_cast<size_t>(image.channel);

        auto copy = std::shared_ptr<unsigned char[]>(new unsigned char[byteCount]);
        std::memcpy(copy.get(), image.data, byteCount);

        PreviewFrame frame;
        frame.width = static_cast<int>(image.width);
        frame.height = static_cast<int>(image.height);
        frame.channels = static_cast<int>(image.channel);
        frame.data = std::move(copy);
        frame.sequence = previewSequence.fetch_add(1, std::memory_order_relaxed) + 1;

        {
            std::lock_guard<std::mutex> lock(previewMutex);
            previewFrame = std::move(frame);
        }
    }

    PreviewFrame DiffusionCallbackUtils::GetLatestPreview() {
        std::lock_guard<std::mutex> lock(previewMutex);
        return previewFrame;
    }

    uint64_t DiffusionCallbackUtils::GetPreviewSequence() {
        return previewSequence.load(std::memory_order_relaxed);
    }

    void DiffusionCallbackUtils::ClearPreview() {
        std::lock_guard<std::mutex> lock(previewMutex);
        previewFrame = PreviewFrame{};
        previewSequence.fetch_add(1, std::memory_order_relaxed);
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