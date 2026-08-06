#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include <string>
#include <vector>
#include <cstdint>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace ECS {

    struct VideoComponent : public BaseComponent {
        AVFormatContext* fmtCtx = nullptr;
        AVCodecContext* codecCtx = nullptr;
        AVFrame* frame = nullptr;
        AVPacket* pkt = nullptr;
        SwsContext* swsCtx = nullptr;
        int videoStreamIndex = -1;

        std::string fileName = "AniStudio";
        std::string filePath = "";
        int width = 0;
        int height = 0;
        double fps = 30.0;
        long long frameCount = 0;
        long long currentFrame = 0;
        bool isPlaying = false;
        float playbackSpeed = 1.0f;
        bool looping = true;

        std::vector<uint8_t> frameDataRGBA;
        GLuint currentTexture = 0;
        bool needsTextureUpdate = false;

        float frameAccumulator = 0.0f;

        VideoComponent() {
            compName = "Video";
            compCategory = "Video";
            setupBaseSchema();
        }

        virtual ~VideoComponent() {
            ReleaseTexture();
            UnloadVideo();
        }

        void ReleaseTexture() {
            if (currentTexture != 0) {
                glDeleteTextures(1, &currentTexture);
                currentTexture = 0;
            }
        }

        void UnloadVideo() {
            if (swsCtx) {
                sws_freeContext(swsCtx);
                swsCtx = nullptr;
            }
            if (frame) {
                av_frame_free(&frame);
                frame = nullptr;
            }
            if (pkt) {
                av_packet_free(&pkt);
                pkt = nullptr;
            }
            if (codecCtx) {
                avcodec_free_context(&codecCtx);
                codecCtx = nullptr;
            }
            if (fmtCtx) {
                avformat_close_input(&fmtCtx);
                fmtCtx = nullptr;
            }
            videoStreamIndex = -1;
            frameDataRGBA.clear();
            frameDataRGBA.shrink_to_fit();
            width = 0;
            height = 0;
            fps = 0.0;
            frameCount = 0;
            currentFrame = 0;
            isPlaying = false;
            frameAccumulator = 0.0f;
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
            properties["isPlaying"] = &isPlaying;
            properties["playbackSpeed"] = &playbackSpeed;
            properties["looping"] = &looping;
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

            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("fps")) fps = componentData["fps"];
            if (componentData.contains("frameCount")) frameCount = componentData["frameCount"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("looping")) looping = componentData["looping"];
        }

        VideoComponent& operator=(const VideoComponent& other) {
            if (this != &other) {
                fileName = other.fileName;
                filePath = other.filePath;
                width = other.width;
                height = other.height;
                fps = other.fps;
                frameCount = other.frameCount;
                currentFrame = other.currentFrame;
                isPlaying = other.isPlaying;
                playbackSpeed = other.playbackSpeed;
                looping = other.looping;
                frameAccumulator = other.frameAccumulator;
            }
            return *this;
        }

        VideoComponent(const VideoComponent& other) : BaseComponent(other) {
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            fps = other.fps;
            frameCount = other.frameCount;
            currentFrame = other.currentFrame;
            isPlaying = other.isPlaying;
            playbackSpeed = other.playbackSpeed;
            looping = other.looping;
            frameAccumulator = other.frameAccumulator;
            fmtCtx = nullptr;
            codecCtx = nullptr;
            frame = nullptr;
            pkt = nullptr;
            swsCtx = nullptr;
            videoStreamIndex = -1;
            currentTexture = 0;
            needsTextureUpdate = false;
            setupBaseSchema();
        }

    protected:
        void setupBaseSchema() {
            schema = {
                {"title", "Video"},
                {"type", "object"},
                {"properties", {
                    {"fileName", {
                        {"type", "string"},
                        {"title", "File Name"}
                    }},
                    {"filePath", {
                        {"type", "string"},
                        {"title", "File Path"}
                    }},
                    {"width", {
                        {"type", "integer"},
                        {"title", "Width"}
                    }},
                    {"height", {
                        {"type", "integer"},
                        {"title", "Height"}
                    }},
                    {"fps", {
                        {"type", "number"},
                        {"title", "FPS"}
                    }},
                    {"frameCount", {
                        {"type", "integer"},
                        {"title", "Frame Count"}
                    }},
                    {"currentFrame", {
                        {"type", "integer"},
                        {"title", "Current Frame"}
                    }},
                    {"isPlaying", {
                        {"type", "boolean"},
                        {"title", "Is Playing"}
                    }},
                    {"playbackSpeed", {
                        {"type", "number"},
                        {"title", "Playback Speed"}
                    }},
                    {"looping", {
                        {"type", "boolean"},
                        {"title", "Looping"}
                    }}
                }}
            };
        }
    };

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
            properties["isPlaying"] = &isPlaying;
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
                {"isPlaying", isPlaying},
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
            if (componentData.contains("isPlaying")) isPlaying = componentData["isPlaying"];
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

    struct OutputVideoComponent : public VideoComponent {
        std::string fileExtension = ".mp4";
        std::vector<std::string> supportedExtensions = { ".mp4", ".webm" };
        int selectedExtensionIndex = 0;

        OutputVideoComponent() {
            compName = "OutputVideo";
            compCategory = "Video";
            setupOutputSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["selectedExtensionIndex"] = &selectedExtensionIndex;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"fileName", fileName},
                {"filePath", filePath},
                {"fileExtension", fileExtension},
                {"selectedExtensionIndex", selectedExtensionIndex}
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
            if (componentData.contains("selectedExtensionIndex")) selectedExtensionIndex = componentData["selectedExtensionIndex"];
        }

        OutputVideoComponent& operator=(const OutputVideoComponent& other) {
            if (this != &other) {
                VideoComponent::operator=(other);
                compName = "OutputVideo";
                fileExtension = other.fileExtension;
                selectedExtensionIndex = other.selectedExtensionIndex;
                supportedExtensions = other.supportedExtensions;
                setupOutputSchema();
            }
            return *this;
        }

        OutputVideoComponent(const OutputVideoComponent& other) : VideoComponent(other) {
            compName = "OutputVideo";
            fileExtension = other.fileExtension;
            selectedExtensionIndex = other.selectedExtensionIndex;
            supportedExtensions = other.supportedExtensions;
            setupOutputSchema();
        }

    private:
        void setupOutputSchema() {
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
                    {"selectedExtensionIndex", {
                        {"type", "integer"},
                        {"title", "File Format"},
                        {"ui:widget", "combo"},
                        {"minimum", 0},
                        {"maximum", 1},
                        {"items", {
                            {{"label", "MP4 (.mp4)"}},
                            {{"label", "WebM (.webm)"}}
                        }},
                        {"ui:options", {
                            {"resetButtonText", "Reset to MP4"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "selectedExtensionIndex"}}
            };
        }
    };

} // namespace ECS