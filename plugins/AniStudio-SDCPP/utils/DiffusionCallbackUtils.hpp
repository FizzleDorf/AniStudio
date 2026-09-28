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

    // POD snapshot: safe to copy, no atomics, no torn reads.
    struct ProgressSnapshot {
        int currentStep = 0;
        int totalSteps = 0;
        float currentTime = 0.0f;
        bool isProcessing = false;
    };

    struct PreviewFrame {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::shared_ptr<unsigned char[]> data;
        uint64_t sequence = 0;
        bool valid() const { return data && width > 0 && height > 0 && channels > 0; }
    };

    // A whole batch of frames from one denoiser step. For image models this is
    // a single frame. For video models it's the video frame count.
    struct PreviewBatch {
        std::vector<PreviewFrame> frames;
        uint64_t sequence = 0;   // bumped every callback
        int step = 0;
        bool valid() const { return !frames.empty() && frames[0].valid(); }
    };

    class DiffusionCallbackUtils {
    public:
        static ProgressData& GetProgressData();
        // Consistent snapshot for UI reads. Use this instead of GetProgressData().
        static ProgressSnapshot GetProgressSnapshot();

        static void InitializeCallbacks();
        static void LogCallback(sd_log_level_t level, const char* text, void* data);
        static void ProgressCallback(int step, int steps, float time, void* data);
        static void TestCallbacks();

        static void SetLogLevel(int level);
        static int GetLogLevel();

        static void SetPreviewMode(int previewMode, int previewInterval);

        static void PreviewCallback(int step, int frame_count, sd_image_t* frames,
            bool is_noisy, void* data);

        // ---- single-frame path (image previews) ----
        static PreviewFrame GetLatestPreview();
        static uint64_t GetPreviewSequence();
        static void ClearPreview();

        // ---- whole-batch path (live video previews) ----
        static PreviewBatch GetLatestBatch();
        static uint64_t GetBatchSequence();
        static void ClearBatch();

        // --- generation token -------------------------------------------------
        // BeginGeneration() must be called on the worker thread immediately
        // before generate_image()/generate_video(). EndGeneration() must be
        // called on the same thread immediately after it returns AND after
        // free_sd_images()/free_sd_audio(). PreviewCallback drops any frame
        // whose token does not match the currently-active token.
        static uint64_t BeginGeneration();
        static void EndGeneration(uint64_t token);

    private:
        static ProgressData progressData;
        static std::mutex mutex;
        static int m_logLevel;

        // Single-frame state (used by image previews).
        static PreviewFrame previewFrame;
        static std::mutex previewMutex;
        static std::atomic<uint64_t> previewSequence;

        // Whole-batch state (used by video previews).
        static PreviewBatch previewBatch;
        static std::mutex batchMutex;
        static std::atomic<uint64_t> batchSequence;

        // 0 == no generation active. Non-zero == token of the active generation.
        static std::atomic<uint64_t> activeGenerationToken;
    };
}