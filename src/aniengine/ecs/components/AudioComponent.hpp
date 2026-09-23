#pragma once

#include "BaseComponent.hpp"
#include "FileFormats.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace ECS {

    struct AVFormatContextAudioDeleter {
        void operator()(AVFormatContext* ptr) const {
            if (ptr) avformat_close_input(&ptr);
        }
    };

    struct AVCodecContextAudioDeleter {
        void operator()(AVCodecContext* ptr) const {
            if (ptr) avcodec_free_context(&ptr);
        }
    };

    struct SwrContextDeleter {
        void operator()(SwrContext* ptr) const {
            if (ptr) swr_free(&ptr);
        }
    };

    struct AudioComponent : BaseComponent {
        AudioComponent() : BaseComponent() {}
        ~AudioComponent() { UnloadAudio(); }

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
                    {"hasExifData", {{"type", "boolean"}, {"title", "Has EXIF Metadata"}}},
                    {"hasLSBData", {{"type", "boolean"}, {"title", "Has LSB Data"}}},
                    {"hasAniStudioMetadata", {{"type", "boolean"}, {"title", "Has AniStudio Metadata"}}}
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
            , hasAudioStream(other.hasAudioStream)
            , hasExifData(other.hasExifData)
            , hasLSBData(other.hasLSBData)
            , hasAniStudioMetadata(other.hasAniStudioMetadata)
        {
        }

        AudioComponent& operator=(const AudioComponent& other) {
            if (this != &other) {
                UnloadDecoder();
                filePath = other.filePath;
                fileName = other.fileName;
                duration = other.duration;
                channels = other.channels;
                sampleRate = other.sampleRate;
                totalSamples = other.totalSamples;
                pcmData = other.pcmData;
                hasAudioStream = other.hasAudioStream;
                hasExifData = other.hasExifData;
                hasLSBData = other.hasLSBData;
                hasAniStudioMetadata = other.hasAniStudioMetadata;
            }
            return *this;
        }

        void UnloadAudio() {
            pcmData.clear();
            pcmData.shrink_to_fit();
            duration = 0.0;
            channels = 0;
            sampleRate = 0;
            totalSamples = 0;
            UnloadDecoder();
        }

        void UnloadDecoder() {
            fmtCtx.reset();
            codecCtx.reset();
            swrCtx.reset();
            audioStreamIndex = -1;
        }

        bool IsLoaded() const { return !pcmData.empty(); }
        bool HasDecoder() const { return fmtCtx != nullptr && codecCtx != nullptr; }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"filePath", &filePath},
                {"fileName", &fileName},
                {"duration", &duration},
                {"channels", &channels},
                {"sampleRate", &sampleRate},
                {"hasExifData", &hasExifData},
                {"hasLSBData", &hasLSBData},
                {"hasAniStudioMetadata", &hasAniStudioMetadata}
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
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) componentData = j.at(key);
            else componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("sampleRate")) sampleRate = componentData["sampleRate"];
            if (componentData.contains("hasExifData")) hasExifData = componentData["hasExifData"];
            if (componentData.contains("hasLSBData")) hasLSBData = componentData["hasLSBData"];
            if (componentData.contains("hasAniStudioMetadata")) hasAniStudioMetadata = componentData["hasAniStudioMetadata"];
        }

        std::string filePath;
        std::string fileName;

        double duration = 0.0;
        int channels = 0;
        int sampleRate = 0;
        int64_t totalSamples = 0;

        std::vector<float> pcmData;

        bool hasAudioStream = false;

        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;

        std::unique_ptr<AVFormatContext, AVFormatContextAudioDeleter> fmtCtx;
        std::unique_ptr<AVCodecContext, AVCodecContextAudioDeleter> codecCtx;
        std::unique_ptr<SwrContext, SwrContextDeleter> swrCtx;
        int audioStreamIndex = -1;
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

        InputAudioComponent(const InputAudioComponent& other) : AudioComponent(other) {}
        InputAudioComponent& operator=(const InputAudioComponent& other) {
            if (this != &other) AudioComponent::operator=(other);
            return *this;
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
            if (j.contains(key)) componentData = j.at(key);
            else componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("fileExtension")) fileExtension = componentData["fileExtension"];
        }
    };

} // namespace ECS