#pragma once

#include "BaseComponent.hpp"
#include "FileFormats.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <sstream>

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

    // One row: "ext|enabled|writeMetadata|bitrate|sampleRate|channels"
    struct AudioExportTarget {
        std::string fileExtension = ".wav";
        bool enabled = true;
        bool writeMetadata = true;
        int bitRate = 192000;
        int sampleRate = 44100;
        int channels = 2;

        nlohmann::json Serialize() const {
            return {
                {"fileExtension", fileExtension},
                {"enabled", enabled},
                {"writeMetadata", writeMetadata},
                {"bitRate", bitRate},
                {"sampleRate", sampleRate},
                {"channels", channels}
            };
        }

        void Deserialize(const nlohmann::json& j) {
            if (j.contains("fileExtension")) fileExtension = j["fileExtension"];
            if (j.contains("enabled")) enabled = j["enabled"];
            if (j.contains("writeMetadata")) writeMetadata = j["writeMetadata"];
            if (j.contains("bitRate")) bitRate = j["bitRate"];
            if (j.contains("sampleRate")) sampleRate = j["sampleRate"];
            if (j.contains("channels")) channels = j["channels"];
        }

        static std::string ToRow(const AudioExportTarget& t) {
            std::ostringstream ss;
            ss << t.fileExtension << "|" << (t.enabled ? 1 : 0)
                << "|" << (t.writeMetadata ? 1 : 0)
                << "|" << t.bitRate
                << "|" << t.sampleRate
                << "|" << t.channels;
            return ss.str();
        }

        static bool FromRow(const std::string& row, AudioExportTarget& out) {
            std::vector<std::string> parts;
            std::string cur;
            for (char c : row) {
                if (c == '|') { parts.push_back(cur); cur.clear(); }
                else cur.push_back(c);
            }
            parts.push_back(cur);
            if (parts.size() < 6) return false;
            try {
                out.fileExtension = parts[0];
                out.enabled = parts[1] == "1";
                out.writeMetadata = parts[2] == "1";
                out.bitRate = std::stoi(parts[3]);
                out.sampleRate = std::stoi(parts[4]);
                out.channels = std::stoi(parts[5]);
            }
            catch (...) { return false; }
            return true;
        }
    };

    struct AudioExportComponent : public BaseComponent {
        std::string fileName = "AniStudio";
        std::string filePath = "OutputFolder";
        bool uniquePerTarget = true;
        std::vector<std::string> targetRows;

        AudioExportComponent() {
            AudioExportTarget t;
            t.fileExtension = ".wav";
            targetRows.push_back(AudioExportTarget::ToRow(t));
        }

        const char* GetCompName() const override { return "AudioExport"; }
        const char* GetCompCategory() const override { return "Media"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json items =
                FileFormats::GetComboItemsJson(FileFormats::GetAudioExtensions());
            static const nlohmann::json j = {
                {"title", "Audio Export"},
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
                            {"browseTooltip", "Directory where every format is saved"}
                        }}
                    }},
                    {"fileName", {
                        {"type", "string"},
                        {"title", "Base Name"},
                        {"ui:widget", "input_text"},
                        {"ui:options", {
                            {"dialogDefaultPath", "OutputFolder"},
                            {"defaultPath", "OutputFolder"},
                            {"resetButtonText", "Reset to Default"}
                        }}
                    }},
                    {"uniquePerTarget", {
                        {"type", "boolean"},
                        {"title", "Unique Filename Per Format"}
                    }},
                    {"targetRows", {
                        {"type", "array"},
                        {"title", "Formats"},
                        {"description", "One row per format: ext|enabled|writeMetadata|bitrate|samplerate|channels"},
                        {"ui:widget", "list"},
                        {"ui:options", {
                            {"formatHint", "ext|on|meta|bitrate|samplerate|channels"},
                            {"availableFormats", items}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "uniquePerTarget", "targetRows"}}
            };
            return j;
        }

        AudioExportComponent(const AudioExportComponent& other)
            : BaseComponent(other)
            , fileName(other.fileName)
            , filePath(other.filePath)
            , uniquePerTarget(other.uniquePerTarget)
            , targetRows(other.targetRows) {
        }

        AudioExportComponent& operator=(const AudioExportComponent& other) {
            if (this != &other) {
                fileName = other.fileName;
                filePath = other.filePath;
                uniquePerTarget = other.uniquePerTarget;
                targetRows = other.targetRows;
            }
            return *this;
        }

        virtual ~AudioExportComponent() {}

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName},
                {"filePath", &filePath},
                {"uniquePerTarget", &uniquePerTarget},
                {"targetRows", &targetRows}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"fileName", fileName},
                {"filePath", filePath},
                {"uniquePerTarget", uniquePerTarget},
                {"targets", nlohmann::json::array()}
            };
            for (const auto& row : targetRows) {
                AudioExportTarget t;
                if (AudioExportTarget::FromRow(row, t))
                    j[GetCompName()]["targets"].push_back(t.Serialize());
            }
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json d = j.contains(key) ? j.at(key) : j;
            if (d.contains("fileName")) fileName = d["fileName"];
            if (d.contains("filePath")) filePath = d["filePath"];
            if (d.contains("uniquePerTarget")) uniquePerTarget = d["uniquePerTarget"];
            targetRows.clear();
            if (d.contains("targets") && d["targets"].is_array()) {
                for (const auto& t : d["targets"]) {
                    AudioExportTarget target;
                    target.Deserialize(t);
                    targetRows.push_back(AudioExportTarget::ToRow(target));
                }
            }
            if (targetRows.empty()) {
                AudioExportTarget def;
                targetRows.push_back(AudioExportTarget::ToRow(def));
            }
        }

        std::vector<AudioExportTarget> GetEnabledTargets() const {
            std::vector<AudioExportTarget> out;
            for (const auto& row : targetRows) {
                AudioExportTarget t;
                if (AudioExportTarget::FromRow(row, t) && t.enabled)
                    out.push_back(t);
            }
            return out;
        }
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
        bool writeMetadata = true;
        int bitRate = 192000;
        int targetSampleRate = 44100;
        int targetChannels = 2;

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
                        {"ui:options", {{"resetButtonText", "Reset to WAV"}}}
                    }},
                    {"writeMetadata", {
                        {"type", "boolean"},
                        {"title", "Write Metadata"}
                    }},
                    {"bitRate", {
                        {"type", "integer"},
                        {"title", "Bit Rate"},
                        {"description", "Encoder bit rate in bps (e.g. 192000)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 8000}, {"max", 1536000}, {"step", 1000}}}
                    }},
                    {"targetSampleRate", {
                        {"type", "integer"},
                        {"title", "Sample Rate"},
                        {"description", "Target sample rate in Hz (e.g. 44100)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 8000}, {"max", 192000}, {"step", 100}}}
                    }},
                    {"targetChannels", {
                        {"type", "integer"},
                        {"title", "Channels"},
                        {"description", "1 = mono, 2 = stereo."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 1}, {"max", 8}, {"step", 1}}}
                    }}
                }},
                {"propertyOrder", {
                    "filePath", "fileName", "fileExtension",
                    "writeMetadata", "bitRate", "targetSampleRate", "targetChannels"
                }}
            };
            return j;
        }

        OutputAudioComponent(const OutputAudioComponent& other)
            : AudioComponent(other)
            , fileExtension(other.fileExtension)
            , writeMetadata(other.writeMetadata)
            , bitRate(other.bitRate)
            , targetSampleRate(other.targetSampleRate)
            , targetChannels(other.targetChannels) {
        }

        OutputAudioComponent& operator=(const OutputAudioComponent& other) {
            if (this != &other) {
                AudioComponent::operator=(other);
                fileExtension = other.fileExtension;
                writeMetadata = other.writeMetadata;
                bitRate = other.bitRate;
                targetSampleRate = other.targetSampleRate;
                targetChannels = other.targetChannels;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"filePath", &filePath},
                {"fileName", &fileName},
                {"fileExtension", &fileExtension},
                {"writeMetadata", &writeMetadata},
                {"bitRate", &bitRate},
                {"targetSampleRate", &targetSampleRate},
                {"targetChannels", &targetChannels}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"filePath", filePath},
                {"fileName", fileName},
                {"fileExtension", fileExtension},
                {"writeMetadata", writeMetadata},
                {"bitRate", bitRate},
                {"targetSampleRate", targetSampleRate},
                {"targetChannels", targetChannels}
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
            if (componentData.contains("writeMetadata")) writeMetadata = componentData["writeMetadata"];
            if (componentData.contains("bitRate")) bitRate = componentData["bitRate"];
            if (componentData.contains("targetSampleRate")) targetSampleRate = componentData["targetSampleRate"];
            if (componentData.contains("targetChannels")) targetChannels = componentData["targetChannels"];
        }
    };

} // namespace ECS