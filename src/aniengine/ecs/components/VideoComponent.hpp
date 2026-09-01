#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include "FileFormats.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <atomic>
#include <shared_mutex>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace ECS {

    // Custom deleters for FFmpeg types
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
            if (ptr) sws_freeContext(ptr);
        }
    };

    struct VideoComponent : public BaseComponent {
        mutable std::shared_mutex dataMutex;

        // ---- Smart pointers for FFmpeg contexts ----
        std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fmtCtx;
        std::unique_ptr<AVCodecContext, AVCodecContextDeleter> codecCtx;
        std::unique_ptr<AVFrame, AVFrameDeleter> frame;
        std::unique_ptr<AVPacket, AVPacketDeleter> pkt;
        std::unique_ptr<SwsContext, SwsContextDeleter> swsCtx;
        int videoStreamIndex = -1;

        // ---- Video metadata ----
        std::string fileName = "AniStudio";
        std::string filePath = "";
        int width = 0;
        int height = 0;
        double fps = 30.0;
        long long frameCount = 0;
        long long currentFrame = 0;
        float playbackSpeed = 1.0f;
        bool looping = true;
        bool isPaused = false;
        float frameAccumulator = 0.0f;
        double currentTime = 0.0;

        // ---- Frame data ----
        std::vector<uint8_t> frameDataRGBA;
        GLuint currentTexture = 0;
        bool needsTextureUpdate = false;

        // ---- Metadata ----
        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;
        uint64_t fileSize = 0;
        std::string fileDate;
        std::string fileTime;

        // ---- Construction ----
        VideoComponent() {
            compName = "Video";
            compCategory = "Video";
            setupBaseSchema();
        }

        virtual ~VideoComponent() {
            ReleaseTexture();
        }

        void ReleaseTexture() {
            std::unique_lock lock(dataMutex);
            if (currentTexture != 0) {
                glDeleteTextures(1, &currentTexture);
                currentTexture = 0;
            }
            needsTextureUpdate = false;
        }

        void UpdateFrameData(std::vector<uint8_t>&& data, int w, int h, long long frame, double time = -1.0) {
            std::unique_lock lock(dataMutex);
            frameDataRGBA = std::move(data);
            width = w;
            height = h;
            currentFrame = frame;
            if (time >= 0.0) {
                currentTime = time;
            }
            else {
                currentTime = static_cast<double>(frame) / (fps > 0.0 ? fps : 30.0);
            }
            needsTextureUpdate = true;
        }

        // ---- Serialization ----
        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["width"] = &width;
            properties["height"] = &height;
            properties["fps"] = &fps;
            properties["frameCount"] = &frameCount;
            properties["currentFrame"] = &currentFrame;
            properties["playbackSpeed"] = &playbackSpeed;
            properties["looping"] = &looping;
            properties["currentTime"] = &currentTime;
            properties["fileSize"] = &fileSize;
            properties["fileDate"] = &fileDate;
            properties["fileTime"] = &fileTime;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"width", width},
                {"height", height},
                {"fps", fps},
                {"frameCount", frameCount},
                {"fileName", fileName},
                {"filePath", filePath},
                {"playbackSpeed", playbackSpeed},
                {"looping", looping},
                {"currentTime", currentTime},
                {"fileSize", fileSize},
                {"fileDate", fileDate},
                {"fileTime", fileTime},
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

            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("fps")) fps = componentData["fps"];
            if (componentData.contains("frameCount")) frameCount = componentData["frameCount"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("playbackSpeed")) playbackSpeed = componentData["playbackSpeed"];
            if (componentData.contains("looping")) looping = componentData["looping"];
            if (componentData.contains("currentTime")) currentTime = componentData["currentTime"];
            if (componentData.contains("fileSize")) fileSize = componentData["fileSize"];
            if (componentData.contains("fileDate")) fileDate = componentData["fileDate"];
            if (componentData.contains("fileTime")) fileTime = componentData["fileTime"];
            if (componentData.contains("hasExifData")) hasExifData = componentData["hasExifData"];
            if (componentData.contains("hasLSBData")) hasLSBData = componentData["hasLSBData"];
            if (componentData.contains("hasAniStudioMetadata")) hasAniStudioMetadata = componentData["hasAniStudioMetadata"];
        }

        VideoComponent& operator=(const VideoComponent& other) {
            if (this != &other) {
                std::unique_lock lock(dataMutex);
                std::shared_lock otherLock(other.dataMutex);
                fileName = other.fileName;
                filePath = other.filePath;
                width = other.width;
                height = other.height;
                fps = other.fps;
                frameCount = other.frameCount;
                currentFrame = other.currentFrame;
                playbackSpeed = other.playbackSpeed;
                looping = other.looping;
                isPaused = other.isPaused;
                frameAccumulator = other.frameAccumulator;
                currentTime = other.currentTime;
                fileSize = other.fileSize;
                fileDate = other.fileDate;
                fileTime = other.fileTime;
                hasExifData = other.hasExifData;
                hasLSBData = other.hasLSBData;
                hasAniStudioMetadata = other.hasAniStudioMetadata;
                // Do not copy FFmpeg contexts or texture
            }
            return *this;
        }

        VideoComponent(const VideoComponent& other) : BaseComponent(other) {
            std::shared_lock otherLock(other.dataMutex);
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            fps = other.fps;
            frameCount = other.frameCount;
            currentFrame = other.currentFrame;
            playbackSpeed = other.playbackSpeed;
            looping = other.looping;
            isPaused = other.isPaused;
            frameAccumulator = other.frameAccumulator;
            currentTime = other.currentTime;
            fileSize = other.fileSize;
            fileDate = other.fileDate;
            fileTime = other.fileTime;
            hasExifData = other.hasExifData;
            hasLSBData = other.hasLSBData;
            hasAniStudioMetadata = other.hasAniStudioMetadata;
            // FFmpeg contexts are not copied - they start empty
            setupBaseSchema();
        }

    protected:
        void setupBaseSchema() {
            schema = {
                {"title", "Video"},
                {"type", "object"},
                {"properties", {
                    {"fileName", {{"type", "string"}, {"title", "File Name"}}},
                    {"filePath", {{"type", "string"}, {"title", "File Path"}}},
                    {"width", {{"type", "integer"}, {"title", "Width"}}},
                    {"height", {{"type", "integer"}, {"title", "Height"}}},
                    {"fps", {{"type", "number"}, {"title", "FPS"}}},
                    {"frameCount", {{"type", "integer"}, {"title", "Frame Count"}}},
                    {"currentFrame", {{"type", "integer"}, {"title", "Current Frame"}}},
                    {"playbackSpeed", {{"type", "number"}, {"title", "Playback Speed"}}},
                    {"looping", {{"type", "boolean"}, {"title", "Looping"}}},
                    {"currentTime", {{"type", "number"}, {"title", "Current Time (seconds)"}}},
                    {"fileSize", {{"type", "integer"}, {"title", "File Size (bytes)"}}},
                    {"fileDate", {{"type", "string"}, {"title", "Date Modified"}}},
                    {"fileTime", {{"type", "string"}, {"title", "Time Modified"}}}
                }}
            };
        }
    };

    // ---- Input Video Component ----
    struct InputVideoComponent : public VideoComponent {
        InputVideoComponent() {
            compName = "InputVideo";
            compCategory = "Video";
            setupInputSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["width"] = &width;
            properties["height"] = &height;
            properties["fps"] = &fps;
            properties["frameCount"] = &frameCount;
            properties["currentFrame"] = &currentFrame;
            properties["playbackSpeed"] = &playbackSpeed;
            properties["looping"] = &looping;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"fileName", fileName},
                {"filePath", filePath},
                {"width", width},
                {"height", height},
                {"fps", fps},
                {"frameCount", frameCount},
                {"currentFrame", currentFrame},
                {"playbackSpeed", playbackSpeed},
                {"looping", looping}
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

            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("fps")) fps = componentData["fps"];
            if (componentData.contains("frameCount")) frameCount = componentData["frameCount"];
            if (componentData.contains("currentFrame")) currentFrame = componentData["currentFrame"];
            if (componentData.contains("playbackSpeed")) playbackSpeed = componentData["playbackSpeed"];
            if (componentData.contains("looping")) looping = componentData["looping"];
        }

        InputVideoComponent& operator=(const InputVideoComponent& other) {
            if (this != &other) {
                VideoComponent::operator=(other);
                compName = "InputVideo";
                setupInputSchema();
            }
            return *this;
        }

        InputVideoComponent(const InputVideoComponent& other) : VideoComponent(other) {
            compName = "InputVideo";
            setupInputSchema();
        }

    private:
        void setupInputSchema() {
            schema = {
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
        }
    };

    // ---- Output Video Component ----
    struct OutputVideoComponent : public VideoComponent {
        std::string fileExtension = ".mp4";

        OutputVideoComponent() {
            compName = "OutputVideo";
            compCategory = "Video";
            setupOutputSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["fileExtension"] = &fileExtension;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"fileName", fileName},
                {"filePath", filePath},
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

            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileExtension")) fileExtension = componentData["fileExtension"];
        }

        OutputVideoComponent& operator=(const OutputVideoComponent& other) {
            if (this != &other) {
                VideoComponent::operator=(other);
                compName = "OutputVideo";
                fileExtension = other.fileExtension;
                setupOutputSchema();
            }
            return *this;
        }

        OutputVideoComponent(const OutputVideoComponent& other) : VideoComponent(other) {
            compName = "OutputVideo";
            fileExtension = other.fileExtension;
            setupOutputSchema();
        }

    private:
        void setupOutputSchema() {
            auto items = FileFormats::GetComboItemsJson(FileFormats::GetVideoExtensions());
            schema = {
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
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "fileExtension"}}
            };
        }
    };

}