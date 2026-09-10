// WaveformUtils.hpp
#pragma once

#include <vector>
#include <imgui.h>
#include <mutex>
#include <string>
#include <functional>

namespace GUI {

    struct WaveformData {
        std::vector<float> samples;
        mutable std::mutex mutex;
        bool dirty{ true };
        size_t targetSize{ 4096 };

        void Update(const float* data, size_t numSamples, int channels);
        void Clear();
        bool IsEmpty() const;
    };

    class WaveformRenderer {
    public:
        struct Config {
            float height{ 64.0f };
            float timeLabelWidth{ 115.0f };
            ImU32 colorBeforePlayhead{ IM_COL32(100, 255, 100, 200) };
            ImU32 colorAfterPlayhead{ IM_COL32(100, 150, 255, 150) };
            ImU32 backgroundColor{ IM_COL32(20, 20, 30, 200) };
            ImU32 borderColor{ IM_COL32(100, 100, 120, 255) };
            ImU32 playheadColor{ IM_COL32(255, 255, 255, 200) };
        };

        WaveformRenderer() = default;
        explicit WaveformRenderer(const Config& config) : m_config(config) {}

        void Render(const WaveformData& data, float progress,
            const std::string& currentTimecode,
            const std::string& totalTimecode,
            std::function<void(double)> onSeek = nullptr);

        void SetConfig(const Config& config) { m_config = config; }
        const Config& GetConfig() const { return m_config; }

    private:
        Config m_config;
        ImVec2 m_lastRenderSize{ 0, 0 };
    };

    // Utility function for timecode formatting
    std::string FormatTimecode(double seconds);

} // namespace GUI