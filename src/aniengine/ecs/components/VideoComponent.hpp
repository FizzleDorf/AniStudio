// VideoComponent.hpp
#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include "FilePathService.hpp"
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

        std::string fileName = "Untitled";
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
            InitializeFilePathFromService();
        }

        ~VideoComponent() {
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

    protected:
        void InitializeFilePathFromService() {
            if (Utils::FilePathService::IsInitialized()) {
                std::string defaultPath = Utils::FilePathService::GetPath("DefaultProject");
                if (!defaultPath.empty())
                    filePath = defaultPath;
            }
        }
    };

    struct InputVideoComponent : public VideoComponent {
        InputVideoComponent() { compName = "InputVideo"; }
    };

    struct OutputVideoComponent : public VideoComponent {
        OutputVideoComponent() { compName = "OutputVideo"; }
    };

} // namespace ECS