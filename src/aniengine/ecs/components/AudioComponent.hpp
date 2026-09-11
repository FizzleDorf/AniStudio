#pragma once

#include "BaseComponent.hpp"
#include "FileFormats.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <shared_mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace ECS {

    struct AudioComponent : BaseComponent {
        mutable std::shared_mutex dataMutex;

        AudioComponent() : BaseComponent() {}

        const char* GetCompName() const override { return "AudioComponent"; }
        const char* GetCompCategory() const override { return "Media"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Audio"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {{"type", "string"}, {"title", "File Path"}}},
                    {"fileName", {{"type", "string"}, {"title", "File Name"}}},
                    {"duration", {{"type", "number"}, {"title", "Duration (seconds)"}}},
                    {"channels", {{"type", "integer"}, {"title", "Channels"}}},
                    {"sampleRate", {{"type", "integer"}, {"title", "Sample Rate (Hz)"}}},
                    {"volume", {{"type", "number"}, {"title", "Volume"}, {"minimum", 0.0}, {"maximum", 1.0}}},
                    {"playbackSpeed", {{"type", "number"}, {"title", "Playback Speed"}, {"minimum", 0.1}, {"maximum", 4.0}}},
                    {"looping", {{"type", "boolean"}, {"title", "Looping"}}},
                    {"currentTime", {{"type", "number"}, {"title", "Current Time (seconds)"}}},
                    {"hasExifData", {{"type", "boolean"}, {"title", "Has EXIF Metadata"}}},
                    {"hasLSBData", {{"type", "boolean"}, {"title", "Has LSB Data"}}},
                    {"hasAniStudioMetadata", {{"type", "boolean"}, {"title", "Has AniStudio Metadata"}}},
                    {"manualSeek", {{"type", "boolean"}, {"title", "Manual Seek Flag"}}}
                }}
            };
            return j;
        }

        AudioComponent(const AudioComponent& other)
            : BaseComponent(other)
            , filePath(other.filePath)
            , fileName(other.fileName)
            , duration(other.duration)
            , channels(other.channels)
            , sampleRate(other.sampleRate)
            , totalSamples(other.totalSamples)
            , pcmData(other.pcmData)
            , volume(other.volume)
            , playbackSpeed(other.playbackSpeed)
            , looping(other.looping)
            , reachedEnd(other.reachedEnd)
            , currentTime(other.currentTime)
            , hasExifData(other.hasExifData)
            , hasLSBData(other.hasLSBData)
            , hasAniStudioMetadata(other.hasAniStudioMetadata)
            , decodeBuffer(other.decodeBuffer)
            , decodeBufferPosition(other.decodeBufferPosition)
            , manualSeek(other.manualSeek)
            , isLoading(false)
        {
        }

        AudioComponent& operator=(const AudioComponent& other) {
            if (this != &other) {
                filePath = other.filePath;
                fileName = other.fileName;
                duration = other.duration;
                channels = other.channels;
                sampleRate = other.sampleRate;
                totalSamples = other.totalSamples;
                pcmData = other.pcmData;
                volume = other.volume;
                playbackSpeed = other.playbackSpeed;
                looping = other.looping;
                reachedEnd = other.reachedEnd;
                currentTime = other.currentTime;
                hasExifData = other.hasExifData;
                hasLSBData = other.hasLSBData;
                hasAniStudioMetadata = other.hasAniStudioMetadata;
                decodeBuffer = other.decodeBuffer;
                decodeBufferPosition = other.decodeBufferPosition;
                manualSeek = other.manualSeek;
                isLoading = false;
            }
            return *this;
        }

        ~AudioComponent() {
            UnloadAudio();
        }

        void UnloadAudio() {
            std::unique_lock lock(dataMutex);
            if (swrCtx) { swr_free(&swrCtx); swrCtx = nullptr; }
            if (fmtCtx) { avformat_close_input(&fmtCtx); fmtCtx = nullptr; }
            if (codecCtx) { avcodec_free_context(&codecCtx); codecCtx = nullptr; }
            if (frame) { av_frame_free(&frame); frame = nullptr; }
            if (pkt) { av_packet_free(&pkt); pkt = nullptr; }
            pcmData.clear();
            pcmData.shrink_to_fit();
            isLoading = false;
        }

        void UpdatePCMData(std::vector<float>&& data, int ch, int sr, double dur) {
            std::unique_lock lock(dataMutex);
            pcmData = std::move(data);
            channels = ch;
            sampleRate = sr;
            duration = dur;
            totalSamples = pcmData.size();
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"filePath", &filePath},
                {"fileName", &fileName},
                {"duration", &duration},
                {"channels", &channels},
                {"sampleRate", &sampleRate},
                {"volume", &volume},
                {"playbackSpeed", &playbackSpeed},
                {"looping", &looping},
                {"currentTime", &currentTime},
                {"hasExifData", &hasExifData},
                {"hasLSBData", &hasLSBData},
                {"hasAniStudioMetadata", &hasAniStudioMetadata},
                {"manualSeek", &manualSeek}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"duration", duration},
                {"channels", channels},
                {"sampleRate", sampleRate},
                {"volume", volume},
                {"playbackSpeed", playbackSpeed},
                {"looping", looping},
                {"currentTime", currentTime},
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata},
                {"manualSeek", manualSeek}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("sampleRate")) sampleRate = componentData["sampleRate"];
            if (componentData.contains("volume")) volume = componentData["volume"];
            if (componentData.contains("playbackSpeed")) playbackSpeed = componentData["playbackSpeed"];
            if (componentData.contains("looping")) looping = componentData["looping"];
            if (componentData.contains("currentTime")) currentTime = componentData["currentTime"];
            if (componentData.contains("hasExifData")) hasExifData = componentData["hasExifData"];
            if (componentData.contains("hasLSBData")) hasLSBData = componentData["hasLSBData"];
            if (componentData.contains("hasAniStudioMetadata")) hasAniStudioMetadata = componentData["hasAniStudioMetadata"];
            if (componentData.contains("manualSeek")) manualSeek = componentData["manualSeek"];
        }

        std::string filePath;
        std::string fileName;

        double duration = 0.0;
        int channels = 0;
        int sampleRate = 0;
        int64_t totalSamples = 0;

        std::vector<float> pcmData;

        AVFormatContext* fmtCtx = nullptr;
        AVCodecContext* codecCtx = nullptr;
        SwrContext* swrCtx = nullptr;
        AVFrame* frame = nullptr;
        AVPacket* pkt = nullptr;
        int audioStreamIndex = -1;

        float volume = 1.0f;
        float playbackSpeed = 1.0f;
        bool looping = false;
        bool reachedEnd = false;
        double currentTime = 0.0;

        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;

        std::vector<float> decodeBuffer;
        size_t decodeBufferPosition = 0;

        bool manualSeek = false;

        std::atomic<bool> isLoading{ false };
    };

    struct InputAudioComponent : public AudioComponent {
        InputAudioComponent() = default;

        const char* GetCompName() const override { return "InputAudio"; }
        const char* GetCompCategory() const override { return "Media"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Input Audio"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Input Audio File"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".wav,.mp3,.flac,.aac,.ogg,.m4a,.opus"},
                            {"filterName", "Audio Files"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for audio files"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "duration", "channels", "sampleRate"}}
            };
            return j;
        }

        InputAudioComponent(const InputAudioComponent& other)
            : AudioComponent(other) {
        }

        InputAudioComponent& operator=(const InputAudioComponent& other) {
            if (this != &other) {
                AudioComponent::operator=(other);
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"filePath", &filePath},
                {"fileName", &fileName},
                {"duration", &duration},
                {"channels", &channels},
                {"sampleRate", &sampleRate}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"duration", duration},
                {"channels", channels},
                {"sampleRate", sampleRate}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("sampleRate")) sampleRate = componentData["sampleRate"];
        }
    };

    struct OutputAudioComponent : public AudioComponent {
        std::string fileExtension = ".wav";

        OutputAudioComponent() = default;

        const char* GetCompName() const override { return "OutputAudio"; }
        const char* GetCompCategory() const override { return "Media"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json items =
                FileFormats::GetComboItemsJson(FileFormats::GetAudioExtensions());
            static const nlohmann::json j = {
                {"title", "Output Audio"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Output Directory"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "directory"},
                            {"defaultPath", "OutputFolder"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Reset"},
                            {"browseTooltip", "Browse to select output directory for saving audio"}
                        }}
                    }},
                    {"fileName", {
                        {"type", "string"},
                        {"title", "File Name"},
                        {"ui:widget", "input_text"},
                        {"ui:options", {
                            {"dialogDefaultPath", "OutputFolder"},
                            {"defaultPath", "OutputFolder"},
                            {"resetButtonText", "Reset to Default"}
                        }}
                    }},
                    {"fileExtension", {
                        {"type", "string"},
                        {"title", "File Format"},
                        {"ui:widget", "combo"},
                        {"items", items},
                        {"ui:options", {
                            {"resetButtonText", "Reset to WAV"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "fileExtension"}}
            };
            return j;
        }

        OutputAudioComponent(const OutputAudioComponent& other)
            : AudioComponent(other)
            , fileExtension(other.fileExtension) {
        }

        OutputAudioComponent& operator=(const OutputAudioComponent& other) {
            if (this != &other) {
                AudioComponent::operator=(other);
                fileExtension = other.fileExtension;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"filePath", &filePath},
                {"fileName", &fileName},
                {"fileExtension", &fileExtension}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"fileExtension", fileExtension}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("fileExtension")) fileExtension = componentData["fileExtension"];
        }
    };

}