#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include "FileFormats.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <shared_mutex>
#include <memory>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace ECS {

    struct AVFormatContextDeleter {
        void operator()(AVFormatContext* ptr) const {
            if (ptr) avformat_close_input(&ptr);
        }
    };

    struct AVCodecContextDeleter {
        void operator()(AVCodecContext* ptr) const {
            if (ptr) avcodec_free_context(&ptr);
        }
    };

    struct AVFrameDeleter {
        void operator()(AVFrame* ptr) const {
            if (ptr) av_frame_free(&ptr);
        }
    };

    struct AVPacketDeleter {
        void operator()(AVPacket* ptr) const {
            if (ptr) av_packet_free(&ptr);
        }
    };

    struct SwsContextDeleter {
        void operator()(SwsContext* ptr) const {
            if (ptr) sws_free_context(&ptr);
        }
    };

    // Static asset info + decoder resources only.
    // All runtime playback state lives in PlaybackStateComponent.
    struct VideoComponent : public BaseComponent {
        std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fmtCtx;
        std::unique_ptr<AVCodecContext, AVCodecContextDeleter> codecCtx;
        std::unique_ptr<AVFrame, AVFrameDeleter> frame;
        std::unique_ptr<AVPacket, AVPacketDeleter> pkt;
        std::unique_ptr<SwsContext, SwsContextDeleter> swsCtx;
        int videoStreamIndex = -1;

        std::string fileName;
        std::string filePath;
        int width = 0;
        int height = 0;
        double fps = 30.0;
        long long frameCount = 0;

        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;
        uint64_t fileSize = 0;
        std::string fileDate;
        std::string fileTime;

        VideoComponent() = default;

        const char* GetCompName() const override { return "Video"; }
        const char* GetCompCategory() const override { return "Video"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Video"},
                {"type", "object"},
                {"properties", {
                    {"fileName", {{"type", "string"}, {"title", "File Name"}}},
                    {"filePath", {{"type", "string"}, {"title", "File Path"}}},
                    {"width", {{"type", "integer"}, {"title", "Width"}}},
                    {"height", {{"type", "integer"}, {"title", "Height"}}},
                    {"fps", {{"type", "number"}, {"title", "FPS"}}},
                    {"frameCount", {{"type", "integer"}, {"title", "Frame Count"}}},
                    {"fileSize", {{"type", "integer"}, {"title", "File Size (bytes)"}}},
                    {"fileDate", {{"type", "string"}, {"title", "Date Modified"}}},
                    {"fileTime", {{"type", "string"}, {"title", "Time Modified"}}}
                }}
            };
            return j;
        }

        VideoComponent(const VideoComponent& other) : BaseComponent(other) {
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            fps = other.fps;
            frameCount = other.frameCount;
            fileSize = other.fileSize;
            fileDate = other.fileDate;
            fileTime = other.fileTime;
            hasExifData = other.hasExifData;
            hasLSBData = other.hasLSBData;
            hasAniStudioMetadata = other.hasAniStudioMetadata;
        }

        VideoComponent& operator=(const VideoComponent& other) {
            if (this != &other) {
                fileName = other.fileName;
                filePath = other.filePath;
                width = other.width;
                height = other.height;
                fps = other.fps;
                frameCount = other.frameCount;
                fileSize = other.fileSize;
                fileDate = other.fileDate;
                fileTime = other.fileTime;
                hasExifData = other.hasExifData;
                hasLSBData = other.hasLSBData;
                hasAniStudioMetadata = other.hasAniStudioMetadata;
            }
            return *this;
        }

        virtual ~VideoComponent() = default;

        bool IsLoaded() const { return fmtCtx != nullptr && frameCount > 0; }

        void Unload() {
            fmtCtx.reset();
            codecCtx.reset();
            frame.reset();
            pkt.reset();
            swsCtx.reset();
            videoStreamIndex = -1;
            width = 0;
            height = 0;
            fps = 30.0;
            frameCount = 0;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName},
                {"filePath", &filePath},
                {"width", &width},
                {"height", &height},
                {"fps", &fps},
                {"frameCount", &frameCount},
                {"fileSize", &fileSize},
                {"fileDate", &fileDate},
                {"fileTime", &fileTime}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"width", width},
                {"height", height},
                {"fps", fps},
                {"frameCount", frameCount},
                {"fileName", fileName},
                {"filePath", filePath},
                {"fileSize", fileSize},
                {"fileDate", fileDate},
                {"fileTime", fileTime},
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

            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("fps")) fps = componentData["fps"];
            if (componentData.contains("frameCount")) frameCount = componentData["frameCount"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileSize")) fileSize = componentData["fileSize"];
            if (componentData.contains("fileDate")) fileDate = componentData["fileDate"];
            if (componentData.contains("fileTime")) fileTime = componentData["fileTime"];
            if (componentData.contains("hasExifData")) hasExifData = componentData["hasExifData"];
            if (componentData.contains("hasLSBData")) hasLSBData = componentData["hasLSBData"];
            if (componentData.contains("hasAniStudioMetadata")) hasAniStudioMetadata = componentData["hasAniStudioMetadata"];
        }
    };

    struct InputVideoComponent : public VideoComponent {
        InputVideoComponent() = default;

        const char* GetCompName() const override { return "InputVideo"; }
        const char* GetCompCategory() const override { return "Video"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Input Video"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Input Video File"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".mp4,.webm,.avi,.mov,.mkv"},
                            {"filterName", "Video Files"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for video files (.mp4, .webm, .avi, .mov, .mkv)"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "width", "height", "fps", "frameCount"}}
            };
            return j;
        }

        InputVideoComponent(const InputVideoComponent& other) : VideoComponent(other) {}
        InputVideoComponent& operator=(const InputVideoComponent& other) {
            if (this != &other) VideoComponent::operator=(other);
            return *this;
        }
    };

    struct OutputVideoComponent : public VideoComponent {
        std::string fileExtension = ".mp4";
        int video_frames = 25;
        int output_fps = 24;

        OutputVideoComponent() = default;

        const char* GetCompName() const override { return "OutputVideo"; }
        const char* GetCompCategory() const override { return "Video"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json items =
                FileFormats::GetComboItemsJson(FileFormats::GetVideoExtensions());
            static const nlohmann::json j = {
                {"title", "Output Video"},
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
                            {"browseTooltip", "Browse to select output directory for saving videos"}
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
                            {"resetButtonText", "Reset to MP4"}
                        }}
                    }},
                    {"video_frames", {
                        {"type", "integer"},
                        {"title", "Video Frames"},
                        {"description", "Number of frames to generate. More frames = longer video but slower generation."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"step_fast", 8}, {"min", 1}, {"max", 99999}}}
                    }},
                    {"output_fps", {
                        {"type", "integer"},
                        {"title", "Output FPS"},
                        {"description", "Frame rate for the generated video. Standard values: 6, 12, 16, 24, 30."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"step_fast", 6}, {"min", 1}, {"max", 120}}}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "fileExtension", "video_frames", "output_fps"}}
            };
            return j;
        }

        OutputVideoComponent(const OutputVideoComponent& other)
            : VideoComponent(other)
            , fileExtension(other.fileExtension)
            , video_frames(other.video_frames)
            , output_fps(other.output_fps) {
        }

        OutputVideoComponent& operator=(const OutputVideoComponent& other) {
            if (this != &other) {
                VideoComponent::operator=(other);
                fileExtension = other.fileExtension;
                video_frames = other.video_frames;
                output_fps = other.output_fps;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName},
                {"filePath", &filePath},
                {"fileExtension", &fileExtension},
                {"video_frames", &video_frames},
                {"output_fps", &output_fps}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"fileName", fileName},
                {"filePath", filePath},
                {"fileExtension", fileExtension},
                {"video_frames", video_frames},
                {"output_fps", output_fps}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) componentData = j.at(key);
            else componentData = j;

            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileExtension")) fileExtension = componentData["fileExtension"];
            if (componentData.contains("video_frames")) video_frames = componentData["video_frames"];
            if (componentData.contains("output_fps")) output_fps = componentData["output_fps"];
        }
    };

    struct PreviewVideoComponent : public VideoComponent {
        // Free-form label shown in debug tooltips: "sdcpp gen", "input ref", ...
        std::string sourceName = "preview";

        PreviewVideoComponent() {
            fileName = "";
            filePath = "";
        }

        const char* GetCompName() const override { return "PreviewVideo"; }
        const char* GetCompCategory() const override { return "Video"; }

        // No editable UI; the preview pane owns the controls.
        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = nlohmann::json::object();
            return j;
        }

        PreviewVideoComponent(const PreviewVideoComponent& other)
            : VideoComponent(other)
            , sourceName(other.sourceName) {
        }

        PreviewVideoComponent& operator=(const PreviewVideoComponent& other) {
            if (this != &other) {
                VideoComponent::operator=(other);
                sourceName = other.sourceName;
            }
            return *this;
        }

        virtual ~PreviewVideoComponent() {}

        // Preview entities are transient UI. Hide from any property grid.
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {};
        }

        // Never persisted; never restored.
        nlohmann::json Serialize() const override {
            return nlohmann::json::object();
        }

        void Deserialize(const nlohmann::json&) override {}
    };
} // namespace ECS