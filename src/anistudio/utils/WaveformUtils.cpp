// WaveformUtils.cpp
#include "WaveformUtils.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace GUI {

    void WaveformData::Update(const float* data, size_t numSamples, int channels) {
        std::lock_guard<std::mutex> lock(mutex);

        if (!data || numSamples == 0 || channels == 0) {
            Clear();
            return;
        }

        size_t totalSamples = numSamples / channels;
        if (totalSamples == 0) {
            Clear();
            return;
        }

        samples.resize(targetSize);

        float step = static_cast<float>(totalSamples) / static_cast<float>(targetSize);

        for (size_t i = 0; i < targetSize; ++i) {
            size_t startIdx = static_cast<size_t>(i * step);
            size_t endIdx = static_cast<size_t>((i + 1) * step);
            if (endIdx > totalSamples) endIdx = totalSamples;

            float peak = 0.0f;
            for (size_t j = startIdx; j < endIdx; ++j) {
                float sample = 0.0f;
                size_t baseIdx = j * channels;
                for (int c = 0; c < channels && c < 8; ++c) {
                    sample += std::abs(data[baseIdx + c]);
                }
                sample /= channels;
                if (sample > peak) peak = sample;
            }
            samples[i] = peak;
        }

        dirty = false;
    }

    void WaveformData::Clear() {
        std::lock_guard<std::mutex> lock(mutex);
        samples.clear();
        dirty = false;
    }

    bool WaveformData::IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return samples.empty();
    }

    void WaveformRenderer::Render(const WaveformData& data, float progress,
        const std::string& currentTimecode,
        const std::string& totalTimecode,
        std::function<void(double)> onSeek) {
        std::lock_guard<std::mutex> lock(data.mutex);

        if (data.samples.empty()) {
            ImGui::TextDisabled("No audio data available for waveform.");
            return;
        }

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float availWidth = ImGui::GetContentRegionAvail().x;

        float waveformWidth = availWidth - (m_config.timeLabelWidth * 2) - (spacing * 2);
        if (waveformWidth < 50.0f) waveformWidth = 50.0f;

        float totalNeeded = (m_config.timeLabelWidth * 2) + waveformWidth + (spacing * 2);
        float startX = (availWidth - totalNeeded) * 0.5f;
        if (startX < 0.0f) startX = 0.0f;
        ImGui::SetCursorPosX(startX);

        // Current time label
        ImGui::PushID("WaveformCurrentTime");
        ImGui::BeginChild("##WaveformCurrentTime",
            ImVec2(m_config.timeLabelWidth, m_config.height),
            true, ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPos(ImVec2(
            (m_config.timeLabelWidth - ImGui::CalcTextSize(currentTimecode.c_str()).x) * 0.5f,
            (m_config.height - ImGui::GetTextLineHeight()) * 0.5f
        ));
        ImGui::Text("%s", currentTimecode.c_str());
        ImGui::EndChild();
        ImGui::PopID();

        ImGui::SameLine();

        // Waveform area
        ImGui::PushID("WaveformArea");
        ImGui::BeginChild("##WaveformArea",
            ImVec2(waveformWidth, m_config.height),
            true, ImGuiWindowFlags_NoScrollbar);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();

        if (size.x > 0 && size.y > 0) {
            drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                m_config.backgroundColor);

            float centerY = pos.y + size.y * 0.5f;
            float halfHeight = size.y * 0.4f;

            size_t totalSamples = data.samples.size();
            size_t samplesToShow = static_cast<size_t>(size.x);
            if (samplesToShow > totalSamples) samplesToShow = totalSamples;

            if (samplesToShow > 0) {
                float step = static_cast<float>(totalSamples) / static_cast<float>(samplesToShow);
                size_t currentSample = static_cast<size_t>(progress * totalSamples);

                for (size_t i = 0; i < samplesToShow; ++i) {
                    size_t sampleIndex = static_cast<size_t>(i * step);
                    if (sampleIndex >= totalSamples) break;

                    float x = pos.x + (static_cast<float>(i) / static_cast<float>(samplesToShow)) * size.x;
                    float sample = data.samples[sampleIndex];
                    float heightPos = sample * halfHeight;

                    bool isBeforePlayhead = (sampleIndex <= currentSample);
                    ImU32 color = isBeforePlayhead ?
                        m_config.colorBeforePlayhead : m_config.colorAfterPlayhead;

                    drawList->AddLine(
                        ImVec2(x, centerY - heightPos),
                        ImVec2(x, centerY + heightPos),
                        color, 1.0f
                    );
                }

                // Draw playhead
                if (progress > 0.0f && progress < 1.0f) {
                    float playheadX = pos.x + progress * size.x;
                    drawList->AddLine(
                        ImVec2(playheadX, pos.y),
                        ImVec2(playheadX, pos.y + size.y),
                        m_config.playheadColor, 2.0f
                    );
                }

                drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                    m_config.borderColor);
            }

            // Click to seek
            ImGui::InvisibleButton("WaveformSeek", size);
            if (ImGui::IsItemHovered()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                float relativeX = (mousePos.x - pos.x) / size.x;
                relativeX = std::clamp(relativeX, 0.0f, 1.0f);

                if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && onSeek) {
                    onSeek(relativeX);
                }

                // Show tooltip with time
                double hoverTime = relativeX * 0.0; // Will be replaced with actual duration
                ImGui::SetTooltip("%s", FormatTimecode(hoverTime).c_str());
            }
        }

        ImGui::EndChild();
        ImGui::PopID();

        ImGui::SameLine();

        // Total time label
        ImGui::PushID("WaveformTotalTime");
        ImGui::BeginChild("##WaveformTotalTime",
            ImVec2(m_config.timeLabelWidth, m_config.height),
            true, ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPos(ImVec2(
            (m_config.timeLabelWidth - ImGui::CalcTextSize(totalTimecode.c_str()).x) * 0.5f,
            (m_config.height - ImGui::GetTextLineHeight()) * 0.5f
        ));
        ImGui::Text("%s", totalTimecode.c_str());
        ImGui::EndChild();
        ImGui::PopID();

        ImGui::Dummy(ImVec2(0, 5));
        m_lastRenderSize = ImVec2(waveformWidth, m_config.height);
    }

    std::string FormatTimecode(double seconds) {
        if (seconds < 0) seconds = 0;
        int totalSeconds = static_cast<int>(seconds);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int secs = totalSeconds % 60;
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << secs;
        return oss.str();
    }

} // namespace GUI