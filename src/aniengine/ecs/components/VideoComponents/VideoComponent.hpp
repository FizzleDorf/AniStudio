/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include "AssetTypes.hpp"
#include <string>

namespace ECS {

	struct VideoComponent : public BaseComponent {
		// Asset system integration
		ResourceID videoAssetId = INVALID_RESOURCE_ID;       // ID of VideoAsset in AssetManager
		ResourceID textureAssetId = INVALID_RESOURCE_ID;     // ID of TextureAsset for current frame

		// Video properties (cached from VideoAsset)
		std::string fileName = "Untitled";
		std::string filePath = Utils::FilePaths::defaultProjectPath;
		int width = 0;
		int height = 0;
		double fps = 30.0;
		int frameCount = 0;

		// Playback state
		int currentFrame = 0;
		bool isPlaying = false;
		float playbackSpeed = 1.0f;
		bool looping = true;

		// Frame timing
		float frameTime = 0.0f;
		float frameDuration = 1.0f / 30.0f;

		// OpenGL texture (cached from TextureAsset)
		GLuint currentTexture = 0;
		bool needsTextureUpdate = false;

		VideoComponent() {
			compName = "Video";
			compCategory = "Video";
		}

		~VideoComponent() {
			// Texture cleanup handled by AssetManager
		}

		// Check if video asset is loaded
		bool IsVideoLoaded() const { return videoAssetId != INVALID_RESOURCE_ID; }
		bool IsTextureReady() const { return textureAssetId != INVALID_RESOURCE_ID && currentTexture != 0; }

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
				{"looping", looping},
				{"videoAssetId", videoAssetId},
				{"textureAssetId", textureAssetId},
				{"currentFrame", currentFrame},
				{"playbackSpeed", playbackSpeed}
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
			if (componentData.contains("videoAssetId")) videoAssetId = componentData["videoAssetId"];
			if (componentData.contains("textureAssetId")) textureAssetId = componentData["textureAssetId"];
			if (componentData.contains("currentFrame")) currentFrame = componentData["currentFrame"];
			if (componentData.contains("playbackSpeed")) playbackSpeed = componentData["playbackSpeed"];
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
				videoAssetId = other.videoAssetId;
				textureAssetId = other.textureAssetId;
				currentTexture = other.currentTexture;
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