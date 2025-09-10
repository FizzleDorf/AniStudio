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