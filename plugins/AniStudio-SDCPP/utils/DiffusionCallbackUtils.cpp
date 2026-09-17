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
    int DiffusionCallbackUtils::m_logLevel = 1; // default INFO

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

        // stable-diffusion.cpp exposes the preview via an extended progress
        // callback. Its exact name/signature depends on your build. The CLI
        // uses sd_set_progress_callback_ex() (or sd_set_preview_callback()).
        // Whichever your header declares, register PreviewCallback here.
        //
        // Example (adjust to match your stable-diffusion.h):
        // sd_set_progress_callback_ex(PreviewCallback, nullptr);
        //
        // If your build does NOT expose a preview callback, you can instead
        // poll the library's internal preview buffer, or hook the step
        // callback inside the library's own code path.

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

    void DiffusionCallbackUtils::PreviewCallback(int step, int steps, sd_image_t* image, void* data) {
        if (!image || !image->data || image->width == 0 || image->height == 0 || image->channel == 0) {
            return;
        }

        // Copy pixels ? the library owns `image->data` and may free/reuse it
        // immediately after this callback returns.
        const size_t pixelCount = static_cast<size_t>(image->width) * image->height;
        const size_t byteCount = pixelCount * image->channel;

        auto copy = std::shared_ptr<unsigned char[]>(new unsigned char[byteCount]);
        std::memcpy(copy.get(), image->data, byteCount);

        PreviewFrame frame;
        frame.width = static_cast<int>(image->width);
        frame.height = static_cast<int>(image->height);
        frame.channels = static_cast<int>(image->channel);
        frame.data = std::move(copy);
        frame.sequence = previewSequence.fetch_add(1, std::memory_order_relaxed) + 1;

        {
            std::lock_guard<std::mutex> lock(previewMutex);
            previewFrame = std::move(frame);
        }

        // Optional log ? useful while wiring this up.
        // std::cerr << "[PREVIEW] step " << step << "/" << steps
        //           << " " << image->width << "x" << image->height
        //           << " ch=" << image->channel << "\n";
    }

    PreviewFrame DiffusionCallbackUtils::GetLatestPreview() {
        std::lock_guard<std::mutex> lock(previewMutex);
        // Return a copy ? the caller gets its own shared_ptr reference.
        return previewFrame;
    }

    uint64_t DiffusionCallbackUtils::GetPreviewSequence() {
        return previewSequence.load(std::memory_order_relaxed);
    }

    void DiffusionCallbackUtils::ClearPreview() {
        std::lock_guard<std::mutex> lock(previewMutex);
        previewFrame = PreviewFrame{};
        // Do NOT reset previewSequence to 0 ? a poller comparing last-seen
        // vs. current would then think a new frame arrived. Just bump it.
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