#pragma once

#include "BaseComponent.hpp"
#include "FileFormats.hpp"
#include <string>
#include <vector>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace ECS {

    struct AudioComponent : BaseComponent {
        AudioComponent() : BaseComponent() {
            compName = "AudioComponent";
            compCategory = "Media";
            setupBaseSchema();
        }

        ~AudioComponent() {
            UnloadAudio();
        }

        void UnloadAudio() {
            if (swrCtx) {
                swr_free(&swrCtx);
                swrCtx = nullptr;
            }
            if (fmtCtx) {
                avformat_close_input(&fmtCtx);
                fmtCtx = nullptr;
            }
            if (codecCtx) {
                avcodec_free_context(&codecCtx);
                codecCtx = nullptr;
            }
            if (frame) {
                av_frame_free(&frame);
                frame = nullptr;
            }
            if (pkt) {
                av_packet_free(&pkt);
                pkt = nullptr;
            }
            pcmData.clear();
            pcmData.shrink_to_fit();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["filePath"] = &filePath;
            properties["fileName"] = &fileName;
            properties["duration"] = &duration;
            properties["channels"] = &channels;
            properties["sampleRate"] = &sampleRate;
            properties["isPlaying"] = &isPlaying;
            properties["isPaused"] = &isPaused;
            properties["looping"] = &looping;
            properties["volume"] = &volume;
            properties["currentTime"] = &currentTime;
            properties["hasExifData"] = &hasExifData;
            properties["hasLSBData"] = &hasLSBData;
            properties["hasAniStudioMetadata"] = &hasAniStudioMetadata;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"duration", duration},
                {"channels", channels},
                {"sampleRate", sampleRate},
                {"isPlaying", isPlaying},
                {"isPaused", isPaused},
                {"looping", looping},
                {"volume", volume},
                {"currentTime", currentTime},
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            else
                componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("sampleRate")) sampleRate = componentData["sampleRate"];
            if (componentData.contains("isPlaying")) isPlaying = componentData["isPlaying"];
            if (componentData.contains("isPaused")) isPaused = componentData["isPaused"];
            if (componentData.contains("looping")) looping = componentData["looping"];
            if (componentData.contains("volume")) volume = componentData["volume"];
            if (componentData.contains("currentTime")) currentTime = componentData["currentTime"];
            if (componentData.contains("hasExifData")) hasExifData = componentData["hasExifData"];
            if (componentData.contains("hasLSBData")) hasLSBData = componentData["hasLSBData"];
            if (componentData.contains("hasAniStudioMetadata")) hasAniStudioMetadata = componentData["hasAniStudioMetadata"];
        }

        // File info
        std::string filePath;
        std::string fileName;

        // Audio metadata
        double duration = 0.0;
        int channels = 0;
        int sampleRate = 0;
        int64_t totalSamples = 0;

        // Decoded PCM data (interleaved float format, range -1.0 to 1.0)
        std::vector<float> pcmData;

        // FFmpeg context
        AVFormatContext* fmtCtx = nullptr;
        AVCodecContext* codecCtx = nullptr;
        SwrContext* swrCtx = nullptr;
        AVFrame* frame = nullptr;
        AVPacket* pkt = nullptr;
        int audioStreamIndex = -1;

        // Playback state
        bool isPlaying = false;
        bool isPaused = false;
        bool looping = false;
        float volume = 1.0f;
        double currentTime = 0.0;
        size_t currentSampleIndex = 0;

        // Metadata flags
        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;

        // For streaming/playback
        std::vector<float> decodeBuffer;
        size_t decodeBufferPosition = 0;

    protected:
        void setupBaseSchema() {
            schema = {
                {"title", "Audio"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "File Path"}
                    }},
                    {"fileName", {
                        {"type", "string"},
                        {"title", "File Name"}
                    }},
                    {"duration", {
                        {"type", "number"},
                        {"title", "Duration (seconds)"}
                    }},
                    {"channels", {
                        {"type", "integer"},
                        {"title", "Channels"}
                    }},
                    {"sampleRate", {
                        {"type", "integer"},
                        {"title", "Sample Rate (Hz)"}
                    }},
                    {"isPlaying", {
                        {"type", "boolean"},
                        {"title", "Is Playing"}
                    }},
                    {"isPaused", {
                        {"type", "boolean"},
                        {"title", "Is Paused"}
                    }},
                    {"looping", {
                        {"type", "boolean"},
                        {"title", "Looping"}
                    }},
                    {"volume", {
                        {"type", "number"},
                        {"title", "Volume"},
                        {"minimum", 0.0},
                        {"maximum", 1.0}
                    }},
                    {"currentTime", {
                        {"type", "number"},
                        {"title", "Current Time (seconds)"}
                    }},
                    {"hasExifData", {
                        {"type", "boolean"},
                        {"title", "Has EXIF Metadata"}
                    }},
                    {"hasLSBData", {
                        {"type", "boolean"},
                        {"title", "Has LSB Data"}
                    }},
                    {"hasAniStudioMetadata", {
                        {"type", "boolean"},
                        {"title", "Has AniStudio Metadata"}
                    }}
                }}
            };
        }
    };

    struct InputAudioComponent : public AudioComponent {
        InputAudioComponent() {
            compName = "InputAudio";
            compCategory = "Media";
            setupInputSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["filePath"] = &filePath;
            properties["fileName"] = &fileName;
            properties["duration"] = &duration;
            properties["channels"] = &channels;
            properties["sampleRate"] = &sampleRate;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"duration", duration},
                {"channels", channels},
                {"sampleRate", sampleRate}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            else
                componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("sampleRate")) sampleRate = componentData["sampleRate"];
        }

        InputAudioComponent& operator=(const InputAudioComponent& other) {
            if (this != &other) {
                AudioComponent::operator=(other);
                compName = "InputAudio";
                setupInputSchema();
            }
            return *this;
        }

        InputAudioComponent(const InputAudioComponent& other) : AudioComponent(other) {
            compName = "InputAudio";
            setupInputSchema();
        }

    private:
        void setupInputSchema() {
            auto items = FileFormats::GetComboItemsJson(FileFormats::GetAudioExtensions());
            schema = {
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
        }
    };

    struct OutputAudioComponent : public AudioComponent {
        std::string fileExtension = ".wav";

        OutputAudioComponent() {
            compName = "OutputAudio";
            compCategory = "Media";
            setupOutputSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["filePath"] = &filePath;
            properties["fileName"] = &fileName;
            properties["fileExtension"] = &fileExtension;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"fileExtension", fileExtension}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            else
                componentData = j;

            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("fileExtension")) fileExtension = componentData["fileExtension"];
        }

        OutputAudioComponent& operator=(const OutputAudioComponent& other) {
            if (this != &other) {
                AudioComponent::operator=(other);
                compName = "OutputAudio";
                fileExtension = other.fileExtension;
                setupOutputSchema();
            }
            return *this;
        }

        OutputAudioComponent(const OutputAudioComponent& other) : AudioComponent(other) {
            compName = "OutputAudio";
            fileExtension = other.fileExtension;
            setupOutputSchema();
        }

    private:
        void setupOutputSchema() {
            auto items = FileFormats::GetComboItemsJson(FileFormats::GetAudioExtensions());
            schema = {
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
        }
    };

} // namespace ECS