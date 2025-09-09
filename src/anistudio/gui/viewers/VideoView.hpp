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

#ifndef VIDEOVIEW_HPP
#define VIDEOVIEW_HPP

#include "GUI.h"
#include "FilePaths.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include "ImGuiFileDialog.h"
#include "../events/Events.hpp"
#include <pch.h>

namespace GUI {

	class VideoView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Video View",
            "category": "Views",
            "description": "A simple video viewer."
        })";
		}

		VideoView(ECS::EntityManager& entityMgr);
		~VideoView();

		void Init() override;
		void Update(float deltaT) override;
		void Render() override;

	private:
		ECS::EntityID selectedEntityID;
		int videoIndex;
		bool showHistory;
		size_t lastEntityCount; // Track entity count changes

		// Video playback state
		bool isPlaying;
		float playbackSpeed;

		// Values for zoom and panning
		float zoom;
		float offsetX;
		float offsetY;

		// Cached entity list - updated by polling
		std::vector<ECS::EntityID> videoEntities;

		// CRITICAL FIX: Track last generated video for auto-selection
		ECS::EntityID lastGeneratedVideoID;

		// File filters
		const char* filters = "Video files{.mp4,.avi,.mkv,.mov,.webm}"
			".mp4,.avi,.mkv,.mov,.webm"
			"{.mp4},MP4"
			"{.avi},AVI"
			"{.mkv},MKV"
			"{.mov},MOV"
			"{.webm},WEBM";

		// Private method declarations
		void RefreshVideoEntities();
		void RenderVideoInfo();
		void RenderControls();
		void RenderSelector();
		void RenderPlaybackControls();
		void RenderHistory();
		void RenderSelectedVideo();
		void DrawGrid(int videoWidth, int videoHeight);
		void SetZoom(float newZoom);
		void LoadVideos(const std::vector<std::string>& filePaths);
		void RemoveSelectedVideo();
		void PauseAllVideos();
		std::string TruncateFilename(const std::string& filename, float maxTextWidth);
	};

} // namespace GUI

#endif // VIDEOVIEW_HPP