#ifndef VIDEOVIEW_HPP
#define VIDEOVIEW_HPP

#include "Base/BaseView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include "../Events/Events.hpp"
#include <pch.h>
#include "ImSequencer.h"

namespace GUI {

	class VideoView : public BaseView {
	public:
		VideoView(ECS::EntityManager& entityMgr)
			: BaseView(entityMgr),
			selectedEntityID(0),
			showControls(true),
			zoom(1.0f),
			offsetX(0.0f),
			offsetY(0.0f),
			currentFrame(0),
			firstFrame(0),
			expanded(true),
			selectedEntry(0)
		{
			viewName = "VideoView";
		}

		void Init() override {
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) {
				mgr.RegisterSystem<ECS::VideoSystem>();
				videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			}

			if (videoSystem) {
				videoSystem->RegisterVideoAddedCallback([this](ECS::EntityID entityID) {
					HandleVideoAdded(entityID);
				});

				videoSystem->RegisterVideoRemovedCallback([this](ECS::EntityID entityID) {
					HandleVideoRemoved(entityID);
				});
			}

			// Initialize with the first video if any exist
			if (selectedEntityID == 0) {
				auto videoEntities = GetVideoEntities();
				if (!videoEntities.empty()) {
					selectedEntityID = videoEntities[0];
				}
			}
		}

		void Update(const float deltaT) override {
			// Update current frame from video component if playing
			if (selectedEntityID != 0 && mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
				auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
				currentFrame = videoComp.currentFrame;
			}
		}

		void Render() override {
			ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);
			ImGui::Begin("Video Viewer", nullptr);

			// Show video information if one is selected
			if (selectedEntityID != 0 && mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
				const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
				ImGui::Text("File: %s", videoComp.fileName.c_str());
				ImGui::Text("Dimensions: %dx%d", videoComp.width, videoComp.height);
				ImGui::Text("FPS: %.2f, Frames: %d", videoComp.fps, videoComp.frameCount);
				ImGui::Text("Current Frame: %d/%d", videoComp.currentFrame, videoComp.frameCount - 1);
				ImGui::Separator();
			}

			RenderSelector();

			if (ImGui::Button("Load Video")) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				ImGuiFileDialog::Instance()->OpenDialog("LoadVideoDialog", "Choose Video",
					"Video files{.mp4,.avi,.mov,.mkv,.webm}{.mp4,.avi,.mov,.mkv,.webm}", config);
			}

			if (ImGuiFileDialog::Instance()->Display("LoadVideoDialog")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					LoadVideo(filePath);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();
			if (selectedEntityID != 0 && ImGui::Button("Remove Video")) {
				RemoveSelectedVideo();
			}

			ImGui::SameLine();
			ImGui::Checkbox("Show Controls", &showControls);

			// Video controls
			if (showControls && selectedEntityID != 0) {
				RenderVideoControls();
			}

			ImGui::Separator();

			// Main video display
			if (ImGui::BeginChild("VideoViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
				RenderSelectedVideo();
				ImGui::EndChild();
			}

			ImGui::End();
		}

		~VideoView() {
			// Unregister callbacks if needed
		}

	private:
		ECS::EntityID selectedEntityID;
		bool showControls;
		float zoom;
		float offsetX;
		float offsetY;

		// Sequencer state
		int currentFrame;
		int firstFrame;
		int sequenceOptions;
		bool expanded;
		int selectedEntry;

		std::vector<ECS::EntityID> GetVideoEntities() const {
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (videoSystem) {
				return videoSystem->GetAllVideoEntities();
			}
			return {};
		}

		void HandleVideoAdded(ECS::EntityID entityID) {
			selectedEntityID = entityID;
			std::cout << "New video added and selected: EntityID=" << entityID << std::endl;
		}

		void HandleVideoRemoved(ECS::EntityID entityID) {
			if (selectedEntityID == entityID) {
				auto videoEntities = GetVideoEntities();
				if (!videoEntities.empty()) {
					selectedEntityID = videoEntities[0];
				}
				else {
					selectedEntityID = 0;
				}
			}
		}

		void RenderSelector() {
			auto videoEntities = GetVideoEntities();
			if (videoEntities.empty()) {
				ImGui::Text("No videos loaded.");
				return;
			}

			// Create a dropdown selector for videos
			if (ImGui::BeginCombo("##VideoSelector",
				selectedEntityID != 0 ?
				mgr.GetComponent<ECS::VideoComponent>(selectedEntityID).fileName.c_str() : "Select Video")) {

				for (auto entity : videoEntities) {
					if (!mgr.HasComponent<ECS::VideoComponent>(entity)) continue;

					const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entity);
					bool isSelected = (entity == selectedEntityID);

					if (ImGui::Selectable(videoComp.fileName.c_str(), isSelected)) {
						selectedEntityID = entity;
						currentFrame = videoComp.currentFrame;
					}

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		void RenderVideoControls() {
			if (!mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) return;

			auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) return;

			// Play/Pause button
			if (ImGui::Button(videoComp.isPlaying ? "Pause" : "Play")) {
				videoComp.isPlaying = !videoComp.isPlaying;
			}

			ImGui::SameLine();

			// Stop button
			if (ImGui::Button("Stop")) {
				videoComp.isPlaying = false;
				videoSystem->SeekToFrame(videoComp, 0);
			}

			ImGui::SameLine();

			// Frame navigation
			if (ImGui::Button("<<")) {
				int newFrame = std::max(0, videoComp.currentFrame - 1);
				videoSystem->SeekToFrame(videoComp, newFrame);
			}

			ImGui::SameLine();
			if (ImGui::Button(">>")) {
				int newFrame = std::min(videoComp.frameCount - 1, videoComp.currentFrame + 1);
				videoSystem->SeekToFrame(videoComp, newFrame);
			}

			ImGui::SameLine();

			// Frame slider
			int frame = videoComp.currentFrame;
			if (ImGui::SliderInt("Frame", &frame, 0, videoComp.frameCount - 1, "%d")) {
				videoSystem->SeekToFrame(videoComp, frame);
			}

			// Playback speed
			ImGui::SliderFloat("Speed", &videoComp.playbackSpeed, 0.1f, 5.0f, "%.1f");
		}

		void RenderSelectedVideo() {
			if (selectedEntityID == 0 || !mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
				ImGui::Text("No video selected.");
				return;
			}

			auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);

			if (videoComp.currentTexture == 0) {
				ImGui::Text("Loading video...");
				return;
			}

			// Calculate display size while maintaining aspect ratio
			float aspect = static_cast<float>(videoComp.width) / videoComp.height;
			float availWidth = ImGui::GetContentRegionAvail().x;
			float displayWidth = availWidth;
			float displayHeight = displayWidth / aspect;

			// Adjust if height exceeds available space
			float availHeight = ImGui::GetContentRegionAvail().y;
			if (displayHeight > availHeight) {
				displayHeight = availHeight;
				displayWidth = displayHeight * aspect;
			}

			// Center the video in available space
			float offsetX = (availWidth - displayWidth) * 0.5f;
			if (offsetX > 0) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
			}

			// Render the video frame with proper orientation
			ImGui::Image(
				reinterpret_cast<void*>(static_cast<intptr_t>(videoComp.currentTexture)),
				ImVec2(displayWidth, displayHeight),
				ImVec2(0, 1),  // Bottom-left UV
				ImVec2(1, 0)   // Top-right UV (flipped vertically)
			);
		}

		void LoadVideo(const std::string& filePath) {
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) {
				std::cerr << "Error: VideoSystem not found!" << std::endl;
				return;
			}

			ECS::EntityID entity = mgr.AddNewEntity();
			mgr.AddComponent<ECS::VideoComponent>(entity);
			videoSystem->SetVideo(entity, filePath);
		}

		void RemoveSelectedVideo() {
			if (selectedEntityID == 0) return;

			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (videoSystem) {
				videoSystem->RemoveVideo(selectedEntityID);
			}
		}
	};

} // namespace GUI

#endif // VIDEO_VIEW_HPP