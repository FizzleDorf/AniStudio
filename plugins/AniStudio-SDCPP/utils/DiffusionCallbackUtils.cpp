// DiffusionCallbackUtils.cpp
#include "DiffusionCallbackUtils.hpp"
#include "ECS.h"
#include "SettingsSystem.hpp"
#include "SDCPPSettingsComponent.hpp"
#include "ErrorBus.hpp"
#include <iostream>
#include <cstring>

namespace GUI {
    ProgressData DiffusionCallbackUtils::progressData;
    std::mutex DiffusionCallbackUtils::mutex;
    int DiffusionCallbackUtils::m_logLevel = 1;

    PreviewFrame DiffusionCallbackUtils::previewFrame;
    std::mutex DiffusionCallbackUtils::previewMutex;
    std::atomic<uint64_t> DiffusionCallbackUtils::previewSequence{ 0 };

    PreviewBatch DiffusionCallbackUtils::previewBatch;
    std::mutex DiffusionCallbackUtils::batchMutex;
    std::atomic<uint64_t> DiffusionCallbackUtils::batchSequence{ 0 };

    std::atomic<uint64_t> DiffusionCallbackUtils::activeGenerationToken{ 0 };

    ProgressData& DiffusionCallbackUtils::GetProgressData() {
        std::lock_guard<std::mutex> lock(mutex);
        return progressData;
    }

    ProgressSnapshot DiffusionCallbackUtils::GetProgressSnapshot() {
        ProgressSnapshot s;
        s.currentStep = progressData.currentStep.load(std::memory_order_relaxed);
        s.totalSteps = progressData.totalSteps.load(std::memory_order_relaxed);
        s.currentTime = progressData.currentTime.load(std::memory_order_relaxed);
        s.isProcessing = progressData.isProcessing.load(std::memory_order_relaxed);
        return s;
    }

    void DiffusionCallbackUtils::InitializeCallbacks() {
        std::cout << "[DEBUG] Setting SD callbacks..." << "\n";
        sd_set_log_callback(LogCallback, nullptr);
        sd_set_progress_callback(ProgressCallback, nullptr);
        sd_set_preview_callback(PreviewCallback, PREVIEW_NONE, 0, true, false, nullptr);
        std::cout << "[DEBUG] SD callbacks set successfully" << "\n";
    }

    void DiffusionCallbackUtils::LogCallback(sd_log_level_t level,
        const char* text, void* /*data*/) {
        if (static_cast<int>(level) >= m_logLevel) {
            const char* levelStr = "UNKNOWN";
            switch (level) {
            case SD_LOG_DEBUG: levelStr = "DEBUG"; break;
            case SD_LOG_INFO:  levelStr = "INFO";  break;
            case SD_LOG_WARN:  levelStr = "WARN";  break;
            case SD_LOG_ERROR: levelStr = "ERROR"; break;
            }
            std::cout << "[SD_LOG][" << levelStr << "] " << (text ? text : "")
                << std::flush;
        }
        if (level == SD_LOG_ERROR) {
            ANI::ErrorBus::Push("sd.cpp", text ? text : "(no message)");
        }
    }

    void DiffusionCallbackUtils::ProgressCallback(int step, int steps, float time, void* /*data*/) {
        progressData.currentStep.store(step, std::memory_order_relaxed);
        progressData.totalSteps.store(steps, std::memory_order_relaxed);
        progressData.currentTime.store(time, std::memory_order_relaxed);
        progressData.isProcessing.store(steps > 0, std::memory_order_relaxed);
        std::cout << "[PROGRESS] Step " << step << "/" << steps
            << " | Time: " << time << "s" << "\n";
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
            << " interval=" << previewInterval << "\n";
    }

    void DiffusionCallbackUtils::PreviewCallback(int step, int frame_count,
        sd_image_t* frames, bool is_noisy,
        void* /*data*/) {
        const uint64_t token = activeGenerationToken.load(std::memory_order_acquire);
        if (token == 0) return;
        if (!frames || frame_count <= 0) return;

        PreviewBatch batch;
        batch.step = step;
        batch.frames.reserve(static_cast<size_t>(frame_count));

        for (int i = 0; i < frame_count; ++i) {
            sd_image_t& image = frames[i];
            if (!image.data || image.width == 0 || image.height == 0) continue;
            if (image.channel != 1 && image.channel != 3 && image.channel != 4) continue;

            const size_t byteCount =
                static_cast<size_t>(image.width) *
                static_cast<size_t>(image.height) *
                static_cast<size_t>(image.channel);

            auto copy = std::shared_ptr<unsigned char[]>(
                new (std::nothrow) unsigned char[byteCount]);
            if (!copy) continue;
            std::memcpy(copy.get(), image.data, byteCount);

            PreviewFrame f;
            f.width = static_cast<int>(image.width);
            f.height = static_cast<int>(image.height);
            f.channels = static_cast<int>(image.channel);
            f.data = std::move(copy);
            f.sequence = previewSequence.fetch_add(1, std::memory_order_relaxed) + 1;
            batch.frames.push_back(std::move(f));
        }

        if (batch.frames.empty()) return;

        // Always publish the first frame as the "latest preview" so
        // image tasks (frame_count == 1) and video tasks (whose first
        // frame is representative) both work.
        {
            std::lock_guard<std::mutex> lock(previewMutex);
            previewFrame = batch.frames[0];
        }

        // Publish the whole batch. For image tasks this is a single frame;
        // for video tasks this is every frame of the current step.
        batch.sequence = batchSequence.fetch_add(1, std::memory_order_relaxed) + 1;
        {
            std::lock_guard<std::mutex> lock(batchMutex);
            previewBatch = std::move(batch);
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

    PreviewBatch DiffusionCallbackUtils::GetLatestBatch() {
        std::lock_guard<std::mutex> lock(batchMutex);
        return previewBatch;
    }

    uint64_t DiffusionCallbackUtils::GetBatchSequence() {
        return batchSequence.load(std::memory_order_relaxed);
    }

    void DiffusionCallbackUtils::ClearBatch() {
        std::lock_guard<std::mutex> lock(batchMutex);
        previewBatch = PreviewBatch{};
        batchSequence.fetch_add(1, std::memory_order_relaxed);
    }

    uint64_t DiffusionCallbackUtils::BeginGeneration() {
        uint64_t token = previewSequence.fetch_add(1, std::memory_order_relaxed) + 1;
        if (token == 0) token = previewSequence.fetch_add(1, std::memory_order_relaxed) + 1;
        activeGenerationToken.store(token, std::memory_order_release);
        ClearPreview();
        ClearBatch();
        return token;
    }

    void DiffusionCallbackUtils::EndGeneration(uint64_t token) {
        uint64_t expected = token;
        activeGenerationToken.compare_exchange_strong(
            expected, 0,
            std::memory_order_acq_rel,
            std::memory_order_acquire);
        // NOTE: do NOT clear here. The last batch must stay visible until
        // the next generation begins (or the user presses Clear). Clearing
        // here was what killed the final preview frame right when the
        // generation finished.
    }

    void DiffusionCallbackUtils::TestCallbacks() {
        std::cout << "[TEST] Testing callback system..." << "\n";
        LogCallback(SD_LOG_INFO, "Test log message\n", nullptr);
        ProgressCallback(5, 10, 2.5f, nullptr);
        const auto s = GetProgressSnapshot();
        std::cout << "[TEST] Progress data: " << s.currentStep
            << "/" << s.totalSteps << "\n";
    }

    void DiffusionCallbackUtils::SetLogLevel(int level) {
        if (level < static_cast<int>(SD_LOG_DEBUG)) level = static_cast<int>(SD_LOG_DEBUG);
        if (level > static_cast<int>(SD_LOG_ERROR)) level = static_cast<int>(SD_LOG_ERROR);
        m_logLevel = level;
        std::cout << "[SD_LOG] Log level set to " << level << "\n";
    }

    int DiffusionCallbackUtils::GetLogLevel() {
        return m_logLevel;
    }
}