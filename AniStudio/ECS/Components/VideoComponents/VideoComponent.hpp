#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include <GL/glew.h>
#include <opencv2/opencv.hpp>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace ECS {

	struct VideoComponent : public BaseComponent {
		// FFmpeg video context
		AVFormatContext* formatCtx = nullptr;
		AVCodecContext* codecCtx = nullptr;
		SwsContext* swsCtx = nullptr;
		AVFrame* rgbFrame = nullptr;
		uint8_t* frameBuffer = nullptr;
		int videoStreamIdx = -1;

		// Video properties
		std::string fileName = "Untitled";
		std::string filePath = Utils::FilePaths::defaultProjectPath;
		int width = 0;
		int height = 0;
		double fps = 30.0;
		int frameCount = 0;
		int currentFrame = 0;
		bool isPlaying = false;
		float playbackSpeed = 1.0f;
		bool looping = true;

		// Frame timing
		float frameTime = 0.0f;
		float frameDuration = 1.0f / 30.0f;

		// OpenGL texture
		GLuint currentTexture = 0;
		cv::Mat currentFrameData;
		bool needsTextureUpdate = false;

		VideoComponent() {
			compName = "Video";
		}

		~VideoComponent() {
			ReleaseTexture();
		}

		void ReleaseTexture() {
			if (currentTexture != 0) {
				glDeleteTextures(1, &currentTexture);
				currentTexture = 0;
			}
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
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				componentData = j;
			}

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
			}
			return *this;
		}
	};

	struct InputVideoComponent : public VideoComponent {
		InputVideoComponent() {
			compName = "InputVideo";
			fileName = "";
			filePath = "";
		}
	};

	struct OutputVideoComponent : public VideoComponent {
		OutputVideoComponent() {
			compName = "OutputVideo";
		}
	};

} // namespace ECS