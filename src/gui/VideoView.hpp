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
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#ifndef VIDEOVIEW_HPP
#define VIDEOVIEW_HPP

#include "Base/BaseView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include "../Events/Events.hpp"
#include <pch.h>

namespace GUI {

	class VideoView : public BaseView {
	public:
		VideoView(ECS::EntityManager& entityMgr)
			: BaseView(entityMgr),
			selectedEntityID(0),
			videoIndex(0),
			showHistory(true),
			zoom(1.0f),
			offsetX(0.0f),
			offsetY(0.0f),
			isPlaying(false),
			playbackSpeed(1.0f)
		{
			viewName = "VideoView";
		}

		void Init() override {
			// Ensure VideoSystem is registered
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) {
				mgr.RegisterSystem<ECS::VideoSystem>();
				videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			}

			// Register callbacks to update the view when videos change
			if (videoSystem) {
				videoSystem->RegisterVideoAddedCallback(std::bind(&VideoView::HandleVideoAdded, this, std::placeholders::_1));
				videoSystem->RegisterVideoRemovedCallback(std::bind(&VideoView::HandleVideoRemoved, this, std::placeholders::_1));
			}

			// Try to select first video if nothing is selected
			if (selectedEntityID == 0) {
				videoEntities = GetVideoEntities();
				if (!videoEntities.empty()) {
					selectedEntityID = videoEntities[0];
					videoIndex = 0;
				}
			}
		}

		void Update(float deltaT) override {
			// Update the video entities list to ensure we have the latest data
			videoEntities = GetVideoEntities();
		}

		void Render() override {
			ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);
			ImGui::Begin("Video Viewer", nullptr);

			// Show video information if one is selected
			if (selectedEntityID != 0 && mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
				const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
				ImGui::Text("File: %s", videoComp.fileName.c_str());
				ImGui::Text("Dimensions: %dx%d, FPS: %.2f, Frames: %d",
					videoComp.width, videoComp.height, videoComp.fps, videoComp.frameCount);
				ImGui::Text("Current Frame: %d / %d", videoComp.currentFrame, videoComp.frameCount);
				ImGui::Separator();
			}

			RenderSelector();

			if (ImGui::Button("Load Video(s)")) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				// Enable multiple selection
				config.countSelectionMax = 0; // 0 means infinite selections
				ImGuiFileDialog::Instance()->OpenDialog("LoadVideoDialog", "Choose Video(s)",
					filters, config);
			}

			if (ImGuiFileDialog::Instance()->Display("LoadVideoDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					// Get multiple selections
					std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();

					std::vector<std::string> filePaths;
					for (const auto&[fileName, filePath] : selection) {
						filePaths.push_back(filePath);
					}

					LoadVideos(filePaths);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();

			if (selectedEntityID != 0 && ImGui::Button("Remove Video")) {
				RemoveSelectedVideo();
			}

			// Option to show/hide history panel
			ImGui::SameLine();
			ImGui::Checkbox("Show History", &showHistory);

			// Video playback controls
			RenderPlaybackControls();

			if (showHistory) {
				RenderHistory();
			}

			ImGui::Separator();

			if (ImGui::BeginChild("VideoViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
				RenderSelectedVideo();
				ImGui::EndChild();
			}

			ImGui::End();
		}

		~VideoView() {
			// Unregister callbacks if possible to prevent accessing deleted object
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (videoSystem) {
				// We would need a way to unregister callbacks here
				// This would require the VideoSystem to have a method to remove callbacks
				// For now, we'll rely on the system not calling callbacks on deleted views
			}
		}

	private:
		ECS::EntityID selectedEntityID;
		int videoIndex;
		bool showHistory;
		std::vector<ECS::EntityID> videoEntities;

		// Video playback state
		bool isPlaying;
		float playbackSpeed;

		// Values for zoom and panning
		float zoom;
		float offsetX;
		float offsetY;

		// File filters
		const char* filters = "Video files{.mp4,.avi,.mkv,.mov,.webm}"
			".mp4,.avi,.mkv,.mov,.webm"
			"{.mp4},MP4"
			"{.avi},AVI"
			"{.mkv},MKV"
			"{.mov},MOV"
			"{.webm},WEBM";

		// Get current list of video entities directly from the VideoSystem
		std::vector<ECS::EntityID> GetVideoEntities() const {
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (videoSystem) {
				return videoSystem->GetAllVideoEntities();
			}
			return {}; // Empty vector if no system or no entities
		}

		// Get the index of the currently selected video
		int GetCurrentVideoIndex() const {
			if (selectedEntityID == 0) return -1;

			auto videoEntities = GetVideoEntities();
			auto it = std::find(videoEntities.begin(), videoEntities.end(), selectedEntityID);
			if (it != videoEntities.end()) {
				return static_cast<int>(std::distance(videoEntities.begin(), it));
			}
			return -1; // Not found
		}

		// Callback handlers
		void HandleVideoAdded(ECS::EntityID entityID) {
			// Set the newly added video as the selected one
			videoEntities = GetVideoEntities();
			selectedEntityID = entityID;
			videoIndex = GetCurrentVideoIndex();

			// Log the operation
			std::cout << "New video added and selected: EntityID=" << entityID << ", Index=" << videoIndex << std::endl;
		}

		void HandleVideoRemoved(ECS::EntityID entityID) {
			videoEntities = GetVideoEntities();
			// Check if the removed entity was the selected one
			if (selectedEntityID == entityID) {
				// Get current video list
				if (videoEntities.empty()) {
					// No videos left
					selectedEntityID = 0;
					videoIndex = 0;
					std::cout << "Selected video was removed. No videos remaining." << std::endl;
				}
				else {
					// Try to keep the same index if possible
					if (videoIndex >= static_cast<int>(videoEntities.size())) {
						videoIndex = static_cast<int>(videoEntities.size() - 1);
					}

					// Set the new selected entity
					selectedEntityID = videoEntities[videoIndex];
					// Pause the previously playing video if any
					PauseAllVideos();
					std::cout << "Selected video was removed. New selection: EntityID="
						<< selectedEntityID << ", Index=" << videoIndex << std::endl;
				}
			}
			else {
				// If a different video was removed, we need to update the videoIndex
				// since the overall collection has changed
				videoIndex = GetCurrentVideoIndex();
				std::cout << "Video removed: EntityID=" << entityID << ". Current selection updated to index: " << videoIndex << std::endl;
			}
		}

		void RenderSelector() {
			if (videoEntities.empty()) {
				ImGui::Text("No videos loaded.");
				return;
			}

			// Navigation buttons
			if (ImGui::Button("First")) {
				if (!videoEntities.empty()) {
					videoIndex = 0;
					selectedEntityID = videoEntities[videoIndex];
					// Pause the previously playing video if any
					PauseAllVideos();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Previous")) {
				if (!videoEntities.empty()) {
					videoIndex = (videoIndex - 1 + static_cast<int>(videoEntities.size())) % static_cast<int>(videoEntities.size());
					selectedEntityID = videoEntities[videoIndex];
					// Pause the previously playing video if any
					PauseAllVideos();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Next")) {
				if (!videoEntities.empty()) {
					videoIndex = (videoIndex + 1) % static_cast<int>(videoEntities.size());
					selectedEntityID = videoEntities[videoIndex];
					// Pause the previously playing video if any
					PauseAllVideos();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Last")) {
				if (!videoEntities.empty()) {
					// Fix potential size_t to int conversion warning
					videoIndex = static_cast<int>(videoEntities.size() - 1);
					selectedEntityID = videoEntities[videoIndex];
					// Pause the previously playing video if any
					PauseAllVideos();
				}
			}

			ImGui::SameLine();

			// Video index selection
			if (ImGui::InputInt("Current Video", &videoIndex)) {
				if (videoEntities.empty()) {
					videoIndex = 0;
					return;
				}

				// Fix potential size_t to int conversion warning
				const int size = static_cast<int>(videoEntities.size());
				if (size == 1) {
					videoIndex = 0;
				}
				else {
					if (videoIndex < 0) {
						videoIndex = size - 1;
					}
					videoIndex = (videoIndex % size + size) % size;
				}

				selectedEntityID = videoEntities[videoIndex];
				// Pause the previously playing video if any
				PauseAllVideos();
			}
		}

		void RenderPlaybackControls() {
			ImGui::Separator();

			if (selectedEntityID == 0 || !mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
				ImGui::Text("No video selected.");
				return;
			}

			auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);

			// Current frame slider
			int currentFrame = videoComp.currentFrame;
			if (ImGui::SliderInt("Frame", &currentFrame, 0, videoComp.frameCount - 1)) {
				auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
				if (videoSystem) {
					videoSystem->SeekToFrame(videoComp, currentFrame);
				}
			}

			// Play/Pause button
			if (ImGui::Button(videoComp.isPlaying ? "Pause" : "Play")) {
				videoComp.isPlaying = !videoComp.isPlaying;
			}

			ImGui::SameLine();

			// Stop button
			if (ImGui::Button("Stop")) {
				videoComp.isPlaying = false;
				auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
				if (videoSystem) {
					videoSystem->SeekToFrame(videoComp, 0);
				}
			}

			ImGui::SameLine();

			// Loop checkbox
			ImGui::Checkbox("Loop", &videoComp.looping);

			ImGui::SameLine();

			// Playback speed slider
			if (ImGui::SliderFloat("Speed", &videoComp.playbackSpeed, 0.1f, 2.0f, "%.1fx")) {
				// The VideoSystem will handle the speed change
			}

			ImGui::Separator();
		}

		void RenderHistory() {
			ImGui::Begin("Video History", &showHistory);

			if (ImGui::Button("Refresh")) {
				videoEntities = GetVideoEntities();
			}

			if (videoEntities.empty()) {
				ImGui::Text("No videos available.");
				ImGui::End();
				return;
			}

			// Track the total width of buttons in the current row
			float currentRowWidth = 0.0f;

			// Create scrollable history panel
			for (size_t i = 0; i < videoEntities.size(); ++i) {
				ECS::EntityID entityID = videoEntities[i];

				if (!mgr.HasComponent<ECS::VideoComponent>(entityID)) {
					continue;
				}

				const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entityID);
				ImGui::BeginGroup();

				// Highlight the selected video
				if (static_cast<int>(i) == videoIndex) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
				}

				// Calculate video dimensions for the thumbnail
				float aspectRatio = static_cast<float>(videoComp.width) / static_cast<float>(videoComp.height);
				ImVec2 maxSize(128.0f, 128.0f);
				ImVec2 imageSize;

				if (aspectRatio > 1.0f) {
					imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
				}
				else {
					imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
				}

				// Display the filename and details
				ImGui::Text("%zu: %s", i, TruncateFilename(videoComp.fileName, imageSize.x).c_str());
				ImGui::Text("%.0f fps, %d frames", videoComp.fps, videoComp.frameCount);

				if (static_cast<int>(i) == videoIndex) {
					ImGui::PopStyleColor();
				}

				// Make the video thumbnail clickable
				if (videoComp.currentTexture != 0) {
					if (ImGui::ImageButton(("##vid" + std::to_string(i)).c_str(),
						reinterpret_cast<void*>(static_cast<intptr_t>(videoComp.currentTexture)),
						imageSize,
						ImVec2(0, 1),  // Top-left with Y flipped
						ImVec2(1, 0)   // Bottom-right with Y flipped
					)) {
						videoIndex = static_cast<int>(i);
						selectedEntityID = entityID;
						// Pause all videos when switching
						PauseAllVideos();
					}
				}
				else {
					// Fallback if no texture
					if (ImGui::Button(("Select##" + std::to_string(i)).c_str(), imageSize)) {
						videoIndex = static_cast<int>(i);
						selectedEntityID = entityID;
						// Pause all videos when switching
						PauseAllVideos();
					}
				}

				ImGui::EndGroup();

				float buttonWidth = imageSize.x + ImGui::GetStyle().ItemSpacing.x;
				currentRowWidth += buttonWidth;

				if (i < videoEntities.size() - 1) {
					float nextButtonWidth = std::min(maxSize.x, maxSize.y * aspectRatio) + ImGui::GetStyle().ItemSpacing.x;

					if (currentRowWidth + nextButtonWidth > ImGui::GetContentRegionAvail().x) {
						ImGui::NewLine();
						currentRowWidth = 0.0f;
					}
					else {
						ImGui::SameLine();
					}
				}
			}

			ImGui::End();
		}

		void RenderSelectedVideo() {
			if (selectedEntityID == 0 || !mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
				ImGui::Text("No video selected.");
				return;
			}

			const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);

			// Handle zooming with mouse wheel when child window is hovered
			if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
				float newZoom = zoom + ImGui::GetIO().MouseWheel * 0.1f;
				SetZoom(newZoom);
			}

			// Calculate the window padding
			const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImVec2 contentPos = ImVec2(windowPos.x + windowPadding.x, windowPos.y + windowPadding.y);

			// Calculate video size and position
			ImVec2 imageSize = ImVec2(videoComp.width * zoom, videoComp.height * zoom);

			// Center the image when zoomed out
			if (zoom <= 1.0f) {
				offsetX = (windowSize.x - imageSize.x) * 0.5f;
				offsetY = (windowSize.y - imageSize.y) * 0.5f;
			}

			ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

			// Draw grid before the video
			DrawGrid(videoComp.width, videoComp.height);

			// Set cursor position and render video frame
			ImGui::SetCursorPos(imagePos);
			if (videoComp.currentTexture != 0) {
				// Display the frame directly as it is in the texture
				ImGui::Image((void*)(intptr_t)videoComp.currentTexture, 
					imageSize,
					ImVec2(0, 1),  // Top-left with Y flipped
					ImVec2(1, 0)   // Bottom-right with Y flipped
				);

				// Handle panning only when the video is hovered and zoomed in
				if (zoom > 1.0f && ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					offsetX += ImGui::GetIO().MouseDelta.x;
					offsetY += ImGui::GetIO().MouseDelta.y;
				}
			}
			else {
				ImGui::Text("No video frame available.");
			}

			// Ensure the content size is set to accommodate the video
			ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
		}

		void DrawGrid(int videoWidth, int videoHeight) {
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			const float gridStep = 100.0f * zoom;

			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImVec2 contentMin = windowPos;
			const ImVec2 contentMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);

			// Calculate grid start positions
			float startX = contentMin.x - fmodf(ImGui::GetScrollX(), gridStep);
			float startY = contentMin.y - fmodf(ImGui::GetScrollY(), gridStep);

			// Draw vertical grid lines
			for (float x = startX; x < contentMax.x; x += gridStep) {
				draw_list->AddLine(ImVec2(x, contentMin.y), ImVec2(x, contentMax.y), IM_COL32(255, 255, 255, 50));
			}

			// Draw horizontal grid lines
			for (float y = startY; y < contentMax.y; y += gridStep) {
				draw_list->AddLine(ImVec2(contentMin.x, y), ImVec2(contentMax.x, y), IM_COL32(255, 255, 255, 50));
			}
		}

		void SetZoom(float newZoom) {
			// Limit zoom level to a reasonable range
			zoom = std::clamp(newZoom, 0.1f, 5.0f);
		}

		void LoadVideos(const std::vector<std::string>& filePaths) {
			// Debug output first
			std::cout << "Selected files to load:" << std::endl;
			for (const auto& filePath : filePaths) {
				std::cout << "  - " << filePath << std::endl;
			}

			// Get VideoSystem
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) {
				std::cerr << "Error: VideoSystem not found!" << std::endl;
				return;
			}

			// Create entities and load videos directly
			ECS::EntityID lastEntity = 0;
			for (const auto& filePath : filePaths) {
				if (filePath.empty()) {
					continue; // Skip empty paths
				}

				// Create entity with component
				ECS::EntityID entity = mgr.AddNewEntity();
				lastEntity = entity; // Keep track of the last entity loaded

				mgr.AddComponent<ECS::VideoComponent>(entity);

				// Load the video directly - this will also trigger the callback
				videoSystem->SetVideo(entity, filePath);
			}
		}

		void RemoveSelectedVideo() {
			if (selectedEntityID == 0) return;

			// Get the video system
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (videoSystem) {
				// Remove via the system to trigger callbacks properly
				videoSystem->RemoveVideo(selectedEntityID);
			}
		}

		// Helper function to pause all videos
		void PauseAllVideos() {
			for (auto entityID : videoEntities) {
				if (mgr.HasComponent<ECS::VideoComponent>(entityID)) {
					auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entityID);
					videoComp.isPlaying = false;
				}
			}
		}

		// Truncate the filename to fit the width of the video
		std::string TruncateFilename(const std::string& filename, float maxTextWidth) {
			float textWidth = ImGui::CalcTextSize(filename.c_str()).x;

			if (textWidth <= maxTextWidth) {
				return filename;
			}

			std::string truncated = "...";
			float ellipsisWidth = ImGui::CalcTextSize(truncated.c_str()).x;

			for (int i = static_cast<int>(filename.length()) - 1; i >= 0; --i) {
				truncated.insert(3, 1, filename[i]);
				float newWidth = ImGui::CalcTextSize(truncated.c_str()).x;

				if (newWidth > maxTextWidth) {
					truncated.erase(3, 1);
					return truncated;
				}
			}

			return truncated;
		}
	};

} // namespace GUI

#endif // VIDEOVIEW_HPP