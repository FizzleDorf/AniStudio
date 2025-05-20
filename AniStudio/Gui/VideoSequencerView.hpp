#ifndef VIDEO_SEQUENCER_VIEW_HPP
#define VIDEO_SEQUENCER_VIEW_HPP

#include "Base/BaseView.hpp"
#include "VideoComponent.hpp"
#include "ImageComponent.hpp"
#include "VideoSystem.hpp"
#include "ImageSystem.hpp"
#include "../Events/Events.hpp"
#include "ImGuizmo.h"
#include "ImSequencer.h"
#include "FilePaths.hpp"
#include <pch.h>
#include <filesystem>

namespace GUI {

	// Custom sequence data for the ImSequencer
	struct VideoSequence : public ImSequencer::SequenceInterface {
		enum ClipType {
			TYPE_VIDEO = 0,
			TYPE_IMAGE = 1,
			TYPE_AUDIO = 2,
			TYPE_COUNT
		};

		struct VideoClip {
			int startFrame;
			int endFrame;
			int type;  // See ClipType enum
			unsigned int color;
			ECS::EntityID entityID;
			std::string name;
			GLuint thumbnailTexture = 0;  // Texture for timeline thumbnail
			int displayDuration = 0;      // For images, how long to show (in frames)
		};

		int frameMin = 0;
		int frameMax = 1000;
		bool expanded = true;
		bool focused = false;
		std::vector<VideoClip> clips;
		int selectedEntry = -1;

		// Implementation of SequenceInterface
		int GetFrameMin() const override { return frameMin; }
		int GetFrameMax() const override { return frameMax; }
		int GetItemCount() const override { return static_cast<int>(clips.size()); }

		const char* GetItemTypeName(int typeIndex) const override {
			switch (typeIndex) {
			case TYPE_VIDEO: return "Video";
			case TYPE_IMAGE: return "Image";
			case TYPE_AUDIO: return "Audio";
			default: return "Unknown";
			}
		}

		const char* GetItemLabel(int index) const override {
			return clips[index].name.c_str();
		}

		void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
			if (index >= 0 && index < clips.size()) {
				static int s, e;
				s = clips[index].startFrame;
				e = clips[index].endFrame;
				if (start) *start = &s;
				if (end) *end = &e;
				if (type) *type = clips[index].type;
				if (color) *color = clips[index].color;
			}
		}

		void Add(int type) override {
			VideoClip newClip;
			newClip.type = type;
			newClip.startFrame = 0;

			// Default duration based on type
			switch (type) {
			case TYPE_VIDEO:
				newClip.endFrame = 100;
				newClip.color = 0xFF66A8FF;  // Blue color for video clips
				break;
			case TYPE_IMAGE:
				newClip.endFrame = 30;  // Default 1 second for images at 30 fps
				newClip.color = 0xFFFFAA66;  // Orange color for image clips
				break;
			case TYPE_AUDIO:
				newClip.endFrame = 100;
				newClip.color = 0xFF66FF66;  // Green color for audio clips
				break;
			default:
				newClip.endFrame = 100;
				newClip.color = 0xFF8C66FF;  // Purple color for unknown types
				break;
			}

			newClip.name = "New Clip " + std::to_string(clips.size());
			newClip.entityID = 0;  // No entity yet
			newClip.thumbnailTexture = 0;
			clips.push_back(newClip);
		}

		void Del(int index) override {
			if (index >= 0 && index < clips.size()) {
				// Free any resources associated with this clip
				if (clips[index].thumbnailTexture != 0 && glIsTexture(clips[index].thumbnailTexture)) {
					glDeleteTextures(1, &clips[index].thumbnailTexture);
				}
				clips.erase(clips.begin() + index);
			}
		}

		void Duplicate(int index) override {
			if (index >= 0 && index < clips.size()) {
				VideoClip newClip = clips[index];
				newClip.name += " (Copy)";
				// Create a new thumbnail if needed
				newClip.thumbnailTexture = 0;
				// Shift the new clip slightly to make it visible
				newClip.startFrame += 30;
				newClip.endFrame += 30;
				// Ensure we don't exceed frame range
				if (newClip.endFrame > frameMax) {
					int shift = newClip.endFrame - frameMax;
					newClip.startFrame -= shift;
					newClip.endFrame -= shift;
				}
				clips.push_back(newClip);
			}
		}

		void Copy() override {
			// Not implemented yet
		}

		void Paste() override {
			// Not implemented yet
		}

		size_t GetCustomHeight(int index) override {
			return 24;  // Slightly taller height for thumbnails
		}

		void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect,
			const ImRect& clippingRect, const ImRect& legendClippingRect) override {
			if (index >= 0 && index < clips.size()) {
				// Draw thumbnail or icon in the custom area if we have one
				if (clips[index].thumbnailTexture != 0 && glIsTexture(clips[index].thumbnailTexture)) {
					ImVec2 thumbnailSize(24, 24);
					ImVec2 pos = legendRect.Min + ImVec2(4, 0);

					// Draw the thumbnail
					draw_list->AddImage(
						reinterpret_cast<void*>(static_cast<intptr_t>(clips[index].thumbnailTexture)),
						pos,
						pos + thumbnailSize,
						ImVec2(0, 1),  // UV coordinates
						ImVec2(1, 0)
					);
				}
				else {
					// Draw an icon based on type if no thumbnail
					ImVec2 pos = legendRect.Min + ImVec2(5, 3);
					const char* icon = "";
					switch (clips[index].type) {
					case TYPE_VIDEO: icon = "V"; break;
					case TYPE_IMAGE: icon = "I"; break;
					case TYPE_AUDIO: icon = "A"; break;
					default: icon = "?"; break;
					}
					draw_list->AddText(pos, 0xFFFFFFFF, icon);
				}
			}
		}

		void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override {
			if (index >= 0 && index < clips.size()) {
				// Draw a small thumbnail or icon in the compact view
				const char* icon = "";
				switch (clips[index].type) {
				case TYPE_VIDEO: icon = "V"; break;
				case TYPE_IMAGE: icon = "I"; break;
				case TYPE_AUDIO: icon = "A"; break;
				default: icon = "?"; break;
				}

				ImVec2 textSize = ImGui::CalcTextSize(icon);
				ImVec2 pos = rc.Min + ImVec2((rc.Max.x - rc.Min.x - textSize.x) * 0.5f, 2);
				draw_list->AddText(pos, 0xFFFFFFFF, icon);

				// Draw the clip name
				std::string shortName = clips[index].name;
				if (shortName.length() > 8) {
					shortName = shortName.substr(0, 6) + "...";
				}

				ImVec2 nameSize = ImGui::CalcTextSize(shortName.c_str());
				ImVec2 namePos = rc.Min + ImVec2((rc.Max.x - rc.Min.x - nameSize.x) * 0.5f,
					rc.Max.y - rc.Min.y - nameSize.y - 2);

				// Draw with a small background for readability
				draw_list->AddRectFilled(
					namePos - ImVec2(2, 2),
					namePos + nameSize + ImVec2(2, 2),
					0x80000000  // Semi-transparent black
				);

				draw_list->AddText(namePos, 0xFFFFFFFF, shortName.c_str());
			}
		}

		void BeginEdit(int index) override {
			selectedEntry = index;
		}

		void EndEdit() override {
			// Save changes if needed
		}

		int GetItemTypeCount() const override {
			return TYPE_COUNT;  // Number of clip types
		}

		void DoubleClick(int index) override {
			// Handle double click - select and focus on the clip
			selectedEntry = index;
		}

		const char* GetCollapseFmt() const override {
			return "%d Frames / %d Elements";
		}
	};

	class VideoSequencerView : public BaseView {
	public:
		VideoSequencerView(ECS::EntityManager& entityMgr)
			: BaseView(entityMgr),
			currentFrame(0),
			sequencePlaybackActive(false),
			currentPlaybackTime(0.0f),
			lastPlaybackTime(0.0f),
			playheadPosition(0.0f),
			lastSelectedClip(-1),
			previewTexture(0),
			previewWidth(640),
			previewHeight(360),
			sequenceModified(false),
			autoUpdatePreview(true),
			sequenceFPS(30.0f),
			showMediaBrowser(false)
		{
			viewName = "VideoSequencerView";
			sequence.frameMin = 0;
			sequence.frameMax = 600;  // 20 seconds @ 30 fps
		}

		~VideoSequencerView() {
			// Clean up preview texture
			if (previewTexture != 0 && glIsTexture(previewTexture)) {
				glDeleteTextures(1, &previewTexture);
				previewTexture = 0;
			}

			// Clean up clip thumbnails
			for (auto& clip : sequence.clips) {
				if (clip.thumbnailTexture != 0 && glIsTexture(clip.thumbnailTexture)) {
					glDeleteTextures(1, &clip.thumbnailTexture);
					clip.thumbnailTexture = 0;
				}
			}
		}

		void Init() override {
			// Ensure system references are valid
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) {
				std::cerr << "VideoSystem not found! The sequencer requires VideoSystem to function properly." << std::endl;
			}
			else {
				// Register callbacks
				videoSystem->RegisterVideoAddedCallback(std::bind(&VideoSequencerView::HandleVideoAdded, this, std::placeholders::_1));
				videoSystem->RegisterVideoRemovedCallback(std::bind(&VideoSequencerView::HandleVideoRemoved, this, std::placeholders::_1));
			}

			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (!imageSystem) {
				std::cerr << "ImageSystem not found! Image support in the sequencer will be limited." << std::endl;
			}

			// Initialize preview texture
			InitializePreviewTexture();

			// Update available media
			UpdateAvailableMedia();
		}

		void Update(float deltaT) override {
			// Handle playback
			if (sequencePlaybackActive) {
				currentPlaybackTime += deltaT;
				float frameTime = 1.0f / sequenceFPS;

				// Calculate how many frames to advance
				int framesAdvanced = static_cast<int>((currentPlaybackTime - lastPlaybackTime) / frameTime);

				if (framesAdvanced > 0) {
					currentFrame += framesAdvanced;
					lastPlaybackTime = currentPlaybackTime;

					// Check if reached end of sequence
					if (currentFrame >= sequence.frameMax) {
						currentFrame = sequence.frameMin;
						currentPlaybackTime = 0.0f;
						lastPlaybackTime = 0.0f;
					}

					// Update preview at current frame
					UpdatePreviewAtFrame(currentFrame);
				}
			}

			// If sequence was modified, update the preview
			if (sequenceModified && autoUpdatePreview) {
				UpdatePreviewAtFrame(currentFrame);
				sequenceModified = false;
			}
		}

		void Render() override {
			ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
			ImGui::Begin("Video Sequencer", nullptr);

			// Top toolbar
			RenderToolbar();

			// Main layout using splitter
			ImVec2 contentSize = ImGui::GetContentRegionAvail();
			static float previewHeightRatio = 0.6f;
			float previewHeight = contentSize.y * previewHeightRatio;
			float timelineHeight = contentSize.y - previewHeight - 8; // 8 for spacing

			// Splitter
			float splitterHeight = 8.0f;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

			// Preview area with horizontal splitter
			if (ImGui::BeginChild("PreviewArea", ImVec2(contentSize.x, previewHeight), true)) {
				RenderPreview();
				ImGui::EndChild();
			}

			// Splitter
			ImGui::Button("##Splitter", ImVec2(contentSize.x, splitterHeight));
			if (ImGui::IsItemActive()) {
				previewHeightRatio += ImGui::GetIO().MouseDelta.y / contentSize.y;
				if (previewHeightRatio < 0.2f) previewHeightRatio = 0.2f;
				if (previewHeightRatio > 0.8f) previewHeightRatio = 0.8f;
			}

			ImGui::PopStyleColor(3);

			// Timeline area - ALWAYS visible
			if (ImGui::BeginChild("TimelineArea", ImVec2(contentSize.x, timelineHeight), true)) {
				RenderTimeline();
				ImGui::EndChild();
			}

			// Media browser popup
			if (showMediaBrowser) {
				RenderMediaBrowser();
			}

			ImGui::End();
		}

	private:
		VideoSequence sequence;
		int currentFrame;
		bool sequencePlaybackActive;
		float currentPlaybackTime;
		float lastPlaybackTime;
		float playheadPosition;
		int lastSelectedClip;

		// Preview texture
		GLuint previewTexture;
		int previewWidth;
		int previewHeight;
		bool sequenceModified;
		bool autoUpdatePreview;
		float sequenceFPS;

		// Media browser
		bool showMediaBrowser;
		std::vector<ECS::EntityID> availableVideos;
		std::vector<ECS::EntityID> availableImages;

		// File filters
		const char* videoFilters = "Video files{.mp4,.avi,.mkv,.mov,.webm};";
		const char* imageFilters = "Image files{.jpg,.jpeg,.png,.bmp,.tga};";

		void RenderToolbar() {
			if (ImGui::Button(sequencePlaybackActive ? "Pause" : "Play")) {
				sequencePlaybackActive = !sequencePlaybackActive;
				if (sequencePlaybackActive) {
					lastPlaybackTime = currentPlaybackTime;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop")) {
				sequencePlaybackActive = false;
				currentFrame = sequence.frameMin;
				currentPlaybackTime = 0.0f;
				lastPlaybackTime = 0.0f;
				UpdatePreviewAtFrame(currentFrame);
			}

			ImGui::SameLine();

			if (ImGui::Button("Previous Frame") && currentFrame > sequence.frameMin) {
				currentFrame--;
				sequencePlaybackActive = false;
				UpdatePreviewAtFrame(currentFrame);
			}

			ImGui::SameLine();

			if (ImGui::Button("Next Frame") && currentFrame < sequence.frameMax) {
				currentFrame++;
				sequencePlaybackActive = false;
				UpdatePreviewAtFrame(currentFrame);
			}

			ImGui::SameLine();
			ImGui::SetNextItemWidth(100);

			if (ImGui::DragInt("Current Frame", &currentFrame, 1.0f, sequence.frameMin, sequence.frameMax)) {
				sequencePlaybackActive = false;
				UpdatePreviewAtFrame(currentFrame);
			}

			ImGui::SameLine();
			ImGui::SetNextItemWidth(80);

			if (ImGui::DragFloat("FPS", &sequenceFPS, 0.1f, 1.0f, 120.0f, "%.1f")) {
				if (sequenceFPS < 1.0f) sequenceFPS = 1.0f;
				if (sequenceFPS > 120.0f) sequenceFPS = 120.0f;
			}

			ImGui::SameLine();

			ImGui::Text("| Seq Length: %.1fs", static_cast<float>(sequence.frameMax) / sequenceFPS);

			ImGui::SameLine();

			if (ImGui::Button("Add Media")) {
				showMediaBrowser = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Video")) {
				LoadVideoFromDisk();
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Image")) {
				LoadImageFromDisk();
			}

			ImGui::SameLine();

			if (ImGui::Button("Clear All")) {
				ImGui::OpenPopup("ClearConfirmPopup");
			}

			// Confirmation popup for clearing
			if (ImGui::BeginPopupModal("ClearConfirmPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Are you sure you want to clear all clips?\nThis operation cannot be undone.");
				ImGui::Separator();

				if (ImGui::Button("Yes", ImVec2(120, 0))) {
					sequence.clips.clear();
					sequenceModified = true;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button("No", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::SameLine();
			ImGui::Checkbox("Auto Update Preview", &autoUpdatePreview);

			ImGui::Separator();
		}

		void RenderPreview() {
			ImVec2 contentSize = ImGui::GetContentRegionAvail();
			ImVec2 previewSize = CalculatePreviewSize(contentSize);

			// Center the preview
			float offsetX = (contentSize.x - previewSize.x) * 0.5f;
			float offsetY = (contentSize.y - previewSize.y) * 0.5f;

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));

			// Black background for the preview area
			ImGui::GetWindowDrawList()->AddRectFilled(
				ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y),
				ImVec2(ImGui::GetCursorScreenPos().x + previewSize.x, ImGui::GetCursorScreenPos().y + previewSize.y),
				IM_COL32(0, 0, 0, 255)
			);

			// If we have a preview texture, draw it
			if (previewTexture != 0 && glIsTexture(previewTexture)) {
				ImGui::Image(
					reinterpret_cast<void*>(static_cast<intptr_t>(previewTexture)),
					previewSize,
					ImVec2(0, 1),  // UV0: top-left with Y flipped
					ImVec2(1, 0)   // UV1: bottom-right with Y flipped
				);
			}
			else {
				// No preview available - draw a placeholder
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (previewSize.x - 200) * 0.5f,
					ImGui::GetCursorPosY() + previewSize.y * 0.5f - 10));
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No preview available");
			}

			// Draw the current time/frame information
			float currentTimeSeconds = static_cast<float>(currentFrame) / sequenceFPS;
			std::string timeInfo = "Time: " + FormatTime(currentTimeSeconds) +
				" | Frame: " + std::to_string(currentFrame) +
				" / " + std::to_string(sequence.frameMax);

			ImVec2 textSize = ImGui::CalcTextSize(timeInfo.c_str());
			float textX = ImGui::GetCursorScreenPos().x + (previewSize.x - textSize.x) * 0.5f;
			float textY = ImGui::GetCursorScreenPos().y + previewSize.y + 5.0f;

			ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), timeInfo.c_str());
		}

		void RenderTimeline() {
			// Always show properties panel for selected clip at the top
			if (sequence.selectedEntry >= 0 && sequence.selectedEntry < sequence.clips.size()) {
				RenderClipProperties();
			}

			// Create a child window for the sequencer that takes remaining space
			ImVec2 contentSize = ImGui::GetContentRegionAvail();
			ImGui::BeginChild("SequencerArea", contentSize, false);

			// The actual sequencer control
			int sequencerFlags = ImSequencer::SEQUENCER_EDIT_STARTEND |
				ImSequencer::SEQUENCER_ADD |
				ImSequencer::SEQUENCER_DEL |
				ImSequencer::SEQUENCER_COPYPASTE |
				ImSequencer::SEQUENCER_CHANGE_FRAME;

			// Get the first visible frame to pass to the sequencer
			int firstFrame = sequence.frameMin;

			// Our selected entry by reference
			int selectedEntry = sequence.selectedEntry;

			// Render the sequencer control
			bool sequencerChanged = ImSequencer::Sequencer(&sequence, &currentFrame, &sequence.expanded, &selectedEntry, &firstFrame, sequencerFlags);

			// Check if the selection changed
			if (selectedEntry != sequence.selectedEntry) {
				sequence.selectedEntry = selectedEntry;
				lastSelectedClip = selectedEntry;
			}

			// If the sequencer changed or we edited something, mark the sequence as modified
			if (sequencerChanged || lastSelectedClip != sequence.selectedEntry) {
				sequenceModified = true;
			}

			ImGui::EndChild();
		}

		void RenderClipProperties() {
			if (sequence.selectedEntry < 0 || sequence.selectedEntry >= sequence.clips.size()) {
				return;
			}

			auto& clip = sequence.clips[sequence.selectedEntry];

			ImGui::BeginChild("ClipProperties", ImVec2(ImGui::GetContentRegionAvail().x, 140), true);

			// Left side - clip details
			ImGui::BeginGroup();
			ImGui::Text("Clip: %s", clip.name.c_str());

			char nameBuffer[256];
			strncpy(nameBuffer, clip.name.c_str(), sizeof(nameBuffer) - 1);
			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
				clip.name = nameBuffer;
			}

			ImGui::Text("Type: %s", sequence.GetItemTypeName(clip.type));

			if (ImGui::DragInt("Start Frame", &clip.startFrame, 1.0f, 0, clip.endFrame - 1)) {
				sequenceModified = true;
				if (clip.startFrame >= clip.endFrame) clip.startFrame = clip.endFrame - 1;
			}

			if (ImGui::DragInt("End Frame", &clip.endFrame, 1.0f, clip.startFrame + 1, sequence.frameMax)) {
				sequenceModified = true;
				if (clip.endFrame <= clip.startFrame) clip.endFrame = clip.startFrame + 1;
			}

			// Calculate clip duration
			float clipDuration = (clip.endFrame - clip.startFrame) / sequenceFPS;
			ImGui::Text("Duration: %.2f seconds", clipDuration);

			// For image clips only
			if (clip.type == VideoSequence::TYPE_IMAGE) {
				ImGui::SameLine();

				if (ImGui::DragInt("Display Duration", &clip.displayDuration, 1.0f, 1, 600)) {
					// Update end frame based on duration
					clip.endFrame = clip.startFrame + clip.displayDuration;
					sequenceModified = true;
				}
			}

			ImGui::EndGroup();

			// Right side - preview and buttons
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 160);
			ImGui::BeginGroup();

			// Preview button
			if (ImGui::Button("Preview Clip", ImVec2(150, 0))) {
				currentFrame = clip.startFrame;
				UpdatePreviewAtFrame(currentFrame);
			}

			// Replace button
			if (ImGui::Button("Replace Media", ImVec2(150, 0))) {
				ReplaceClipMedia(sequence.selectedEntry);
			}

			// Color button
			float color[4] = {
				((clip.color >> 0) & 0xFF) / 255.0f,
				((clip.color >> 8) & 0xFF) / 255.0f,
				((clip.color >> 16) & 0xFF) / 255.0f,
				((clip.color >> 24) & 0xFF) / 255.0f
			};

			if (ImGui::ColorEdit4("Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
				clip.color =
					(static_cast<unsigned int>(color[3] * 255) << 24) |
					(static_cast<unsigned int>(color[2] * 255) << 16) |
					(static_cast<unsigned int>(color[1] * 255) << 8) |
					static_cast<unsigned int>(color[0] * 255);
				sequenceModified = true;
			}

			// Remove clip button
			if (ImGui::Button("Remove Clip", ImVec2(150, 0))) {
				sequence.Del(sequence.selectedEntry);
				sequence.selectedEntry = -1;
				sequenceModified = true;
			}

			ImGui::EndGroup();

			ImGui::EndChild();
		}

		void RenderMediaBrowser() {
			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Media Browser", &showMediaBrowser)) {
				// Tabs for different media types
				if (ImGui::BeginTabBar("MediaTabs")) {
					if (ImGui::BeginTabItem("Videos")) {
						RenderVideoBrowser();
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Images")) {
						RenderImageBrowser();
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}

				ImGui::End();
			}
			else {
				// Window was closed
				showMediaBrowser = false;
			}
		}

		void RenderVideoBrowser() {
			if (ImGui::Button("Refresh")) {
				UpdateAvailableMedia();
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Video File")) {
				LoadVideoFromDisk();
			}

			ImGui::Separator();

			// Display available videos in a grid
			if (availableVideos.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
					"No videos available. Load videos using the Video View or the 'Load Video' button.");
				return;
			}

			// Calculate grid layout
			float thumbnailSize = 120.0f;
			float padding = 10.0f;
			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnsCount = static_cast<int>(panelWidth / (thumbnailSize + padding));
			if (columnsCount < 1) columnsCount = 1;

			// Track the current column for layout
			int currentColumn = 0;

			for (const auto& entityID : availableVideos) {
				if (!mgr.HasComponent<ECS::VideoComponent>(entityID)) {
					continue;
				}

				auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entityID);

				// Start a new row if needed
				if (currentColumn >= columnsCount) {
					ImGui::NewLine();
					currentColumn = 0;
				}

				if (currentColumn > 0) {
					ImGui::SameLine(currentColumn * (thumbnailSize + padding));
				}

				// Create a group for the thumbnail and label
				ImGui::BeginGroup();

				// Video thumbnail (or placeholder)
				if (videoComp.currentTexture != 0 && glIsTexture(videoComp.currentTexture)) {
					// Calculate aspect ratio
					float aspectRatio = static_cast<float>(videoComp.width) / static_cast<float>(videoComp.height);
					ImVec2 imgSize;

					if (aspectRatio > 1.0f) {
						imgSize = ImVec2(thumbnailSize, thumbnailSize / aspectRatio);
					}
					else {
						imgSize = ImVec2(thumbnailSize * aspectRatio, thumbnailSize);
					}

					// Center the image in its cell
					float offsetX = (thumbnailSize - imgSize.x) * 0.5f;
					float offsetY = (thumbnailSize - imgSize.y) * 0.5f;

					// Draw a frame around the thumbnail
					ImGui::GetWindowDrawList()->AddRect(
						ImGui::GetCursorScreenPos() + ImVec2(offsetX - 1, offsetY - 1),
						ImGui::GetCursorScreenPos() + ImVec2(offsetX + imgSize.x + 1, offsetY + imgSize.y + 1),
						IM_COL32(80, 80, 80, 255),
						2.0f
					);

					// Display thumbnail as a button
					if (ImGui::ImageButton(("##video" + std::to_string(entityID)).c_str(),
						reinterpret_cast<void*>(static_cast<intptr_t>(videoComp.currentTexture)),
						imgSize,
						ImVec2(0, 1),  // UV0: top-left with Y flipped
						ImVec2(1, 0),  // UV1: bottom-right with Y flipped
						ImVec4(0, 0, 0, 0)  // Background color (transparent)
					)) {
						// Add to timeline
						AddVideoToSequence(entityID);
					}
				}
				else {
					// Draw a placeholder button
					if (ImGui::Button(("##placeholder" + std::to_string(entityID)).c_str(), ImVec2(thumbnailSize, thumbnailSize))) {
						// Add to timeline
						AddVideoToSequence(entityID);
					}

					// Draw 'VIDEO' text over the placeholder
					ImVec2 textSize = ImGui::CalcTextSize("VIDEO");
					float textX = ImGui::GetItemRectMin().x + (thumbnailSize - textSize.x) * 0.5f;
					float textY = ImGui::GetItemRectMin().y + (thumbnailSize - textSize.y) * 0.5f;

					ImGui::GetWindowDrawList()->AddText(
						ImVec2(textX, textY),
						IM_COL32(180, 180, 180, 255),
						"VIDEO"
					);
				}

				// Display video name - truncate if too long
				std::string displayName = videoComp.fileName;
				if (displayName.length() > 15) {
					displayName = displayName.substr(0, 12) + "...";
				}

				// Center the name under the thumbnail
				ImVec2 nameSize = ImGui::CalcTextSize(displayName.c_str());
				float nameX = (thumbnailSize - nameSize.x) * 0.5f;

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + nameX);
				ImGui::TextWrapped("%s", displayName.c_str());

				// Display duration
				std::string durationText = FormatTime(videoComp.frameCount / videoComp.fps);
				ImVec2 durSize = ImGui::CalcTextSize(durationText.c_str());
				float durX = (thumbnailSize - durSize.x) * 0.5f;

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + durX);
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", durationText.c_str());

				ImGui::EndGroup();

				currentColumn++;
			}
		}

		void RenderImageBrowser() {
			if (ImGui::Button("Refresh")) {
				UpdateAvailableMedia();
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Image File")) {
				LoadImageFromDisk();
			}

			ImGui::Separator();

			// Display available images in a grid
			if (availableImages.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
					"No images available. Load images using the Image View or the 'Load Image' button.");
				return;
			}

			// Calculate grid layout
			float thumbnailSize = 120.0f;
			float padding = 10.0f;
			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnsCount = static_cast<int>(panelWidth / (thumbnailSize + padding));
			if (columnsCount < 1) columnsCount = 1;

			// Track the current column for layout
			int currentColumn = 0;

			for (const auto& entityID : availableImages) {
				if (!mgr.HasComponent<ECS::ImageComponent>(entityID)) {
					continue;
				}

				auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(entityID);

				// Start a new row if needed
				if (currentColumn >= columnsCount) {
					ImGui::NewLine();
					currentColumn = 0;
				}

				if (currentColumn > 0) {
					ImGui::SameLine(currentColumn * (thumbnailSize + padding));
				}

				// Create a group for the thumbnail and label
				ImGui::BeginGroup();

				// Image thumbnail (or placeholder)
				if (imageComp.textureID != 0 && glIsTexture(imageComp.textureID)) {
					// Calculate aspect ratio
					float aspectRatio = static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height);
					ImVec2 imgSize;

					if (aspectRatio > 1.0f) {
						imgSize = ImVec2(thumbnailSize, thumbnailSize / aspectRatio);
					}
					else {
						imgSize = ImVec2(thumbnailSize * aspectRatio, thumbnailSize);
					}

					// Center the image in its cell
					float offsetX = (thumbnailSize - imgSize.x) * 0.5f;
					float offsetY = (thumbnailSize - imgSize.y) * 0.5f;

					// Draw a frame around the thumbnail
					ImGui::GetWindowDrawList()->AddRect(
						ImGui::GetCursorScreenPos() + ImVec2(offsetX - 1, offsetY - 1),
						ImGui::GetCursorScreenPos() + ImVec2(offsetX + imgSize.x + 1, offsetY + imgSize.y + 1),
						IM_COL32(80, 80, 80, 255),
						2.0f
					);

					// Display thumbnail as a button
					if (ImGui::ImageButton(("##image" + std::to_string(entityID)).c_str(),
						reinterpret_cast<void*>(static_cast<intptr_t>(imageComp.textureID)),
						imgSize,
						ImVec2(0, 1),  // UV0: top-left with Y flipped
						ImVec2(1, 0),  // UV1: bottom-right with Y flipped
						ImVec4(0, 0, 0, 0)  // Background color (transparent)
					)) {
						// Add to timeline
						AddImageToSequence(entityID);
					}
				}
				else {
					// Draw a placeholder button
					if (ImGui::Button(("##imgplaceholder" + std::to_string(entityID)).c_str(), ImVec2(thumbnailSize, thumbnailSize))) {
						// Add to timeline
						AddImageToSequence(entityID);
					}

					// Draw 'IMAGE' text over the placeholder
					ImVec2 textSize = ImGui::CalcTextSize("IMAGE");
					float textX = ImGui::GetItemRectMin().x + (thumbnailSize - textSize.x) * 0.5f;
					float textY = ImGui::GetItemRectMin().y + (thumbnailSize - textSize.y) * 0.5f;

					ImGui::GetWindowDrawList()->AddText(
						ImVec2(textX, textY),
						IM_COL32(180, 180, 180, 255),
						"IMAGE"
					);
				}

				// Display image name - truncate if too long
				std::string displayName = imageComp.fileName;
				if (displayName.length() > 15) {
					displayName = displayName.substr(0, 12) + "...";
				}

				// Center the name under the thumbnail
				ImVec2 nameSize = ImGui::CalcTextSize(displayName.c_str());
				float nameX = (thumbnailSize - nameSize.x) * 0.5f;

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + nameX);
				ImGui::TextWrapped("%s", displayName.c_str());

				// Display dimensions
				std::string dimText = std::to_string(imageComp.width) + "x" + std::to_string(imageComp.height);
				ImVec2 dimSize = ImGui::CalcTextSize(dimText.c_str());
				float dimX = (thumbnailSize - dimSize.x) * 0.5f;

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + dimX);
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", dimText.c_str());

				ImGui::EndGroup();

				currentColumn++;
			}
		}

		ImVec2 CalculatePreviewSize(const ImVec2& availableSize) {
			float targetWidth = availableSize.x * 0.9f;  // Use 90% of available width
			float targetHeight = targetWidth * (9.0f / 16.0f);  // Assume 16:9 aspect ratio

			// Ensure the height fits
			if (targetHeight > availableSize.y * 0.9f) {
				targetHeight = availableSize.y * 0.9f;
				targetWidth = targetHeight * (16.0f / 9.0f);
			}

			return ImVec2(targetWidth, targetHeight);
		}

		std::string FormatTime(float seconds) {
			int totalSeconds = static_cast<int>(seconds);
			int minutes = totalSeconds / 60;
			int remainingSeconds = totalSeconds % 60;
			int milliseconds = static_cast<int>((seconds - totalSeconds) * 1000);

			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%02d:%02d.%03d", minutes, remainingSeconds, milliseconds);
			return buffer;
		}

		void InitializePreviewTexture() {
			// Create the preview texture if it doesn't exist
			if (previewTexture == 0) {
				glGenTextures(1, &previewTexture);
				glBindTexture(GL_TEXTURE_2D, previewTexture);

				// Set texture parameters
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				// Allocate empty texture
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, previewWidth, previewHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

				// Check for errors
				GLenum err = glGetError();
				if (err != GL_NO_ERROR) {
					std::cerr << "OpenGL error creating preview texture: " << err << std::endl;
					glDeleteTextures(1, &previewTexture);
					previewTexture = 0;
				}
			}
		}

		void UpdatePreviewAtFrame(int frame) {
			// Find which clip is active at this frame
			for (const auto& clip : sequence.clips) {
				if (frame >= clip.startFrame && frame <= clip.endFrame) {
					// Found an active clip, update the preview with its frame
					if (clip.type == VideoSequence::TYPE_VIDEO) {
						UpdatePreviewFromVideoClip(clip, frame - clip.startFrame);
					}
					else if (clip.type == VideoSequence::TYPE_IMAGE) {
						UpdatePreviewFromImageClip(clip);
					}
					return;
				}
			}

			// No clip found, clear the preview
			ClearPreview();
		}

		void UpdatePreviewFromVideoClip(const VideoSequence::VideoClip& clip, int clipFrame) {
			if (clip.entityID == 0 || !mgr.HasComponent<ECS::VideoComponent>(clip.entityID)) {
				return;
			}

			auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(clip.entityID);

			// Ensure we don't exceed the video's frame count
			if (clipFrame >= videoComp.frameCount) {
				clipFrame = videoComp.frameCount - 1;
			}

			// Get video system
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) return;

			// Seek to the desired frame in the video
			videoSystem->SeekToFrame(videoComp, clipFrame);

			// The video system should have updated the texture in the component
			// Simply copy that texture to our preview or use it directly
			if (videoComp.currentTexture != 0 && glIsTexture(videoComp.currentTexture)) {
				// Copy texture to our preview
				if (previewTexture != 0) {
					CopyTexture(videoComp.currentTexture, previewTexture);
				}
			}
		}

		void UpdatePreviewFromImageClip(const VideoSequence::VideoClip& clip) {
			if (clip.entityID == 0 || !mgr.HasComponent<ECS::ImageComponent>(clip.entityID)) {
				return;
			}

			auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(clip.entityID);

			// Simply copy that texture to our preview
			if (imageComp.textureID != 0 && glIsTexture(imageComp.textureID)) {
				// Copy texture to our preview
				if (previewTexture != 0) {
					CopyTexture(imageComp.textureID, previewTexture);
				}
			}
		}

		void CopyTexture(GLuint sourceTexture, GLuint destTexture) {
			// In a real implementation, you would use an FBO to copy textures properly
			// For now, we'll use the simplest approach - just use the source texture
			previewTexture = sourceTexture;
		}

		void ClearPreview() {
			if (previewTexture != 0) {
				glBindTexture(GL_TEXTURE_2D, previewTexture);

				// Clear with black
				std::vector<unsigned char> blackPixels(previewWidth * previewHeight * 3, 0);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, previewWidth, previewHeight, GL_RGB, GL_UNSIGNED_BYTE, blackPixels.data());
			}
		}

		void UpdateAvailableMedia() {
			// Get videos
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (videoSystem) {
				availableVideos = videoSystem->GetAllVideoEntities();
			}

			// Get images
			availableImages.clear();
			for (auto entityID : mgr.GetAllEntities()) {
				if (mgr.HasComponent<ECS::ImageComponent>(entityID)) {
					availableImages.push_back(entityID);
				}
			}
		}

		void LoadVideoFromDisk() {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			config.countSelectionMax = 10;  // Allow multiple selection

			ImGuiFileDialog::Instance()->OpenDialog(
				"LoadVideoDialog",
				"Choose Video File(s)",
				videoFilters,
				config
			);
		}

		void LoadImageFromDisk() {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			config.countSelectionMax = 10;  // Allow multiple selection

			ImGuiFileDialog::Instance()->OpenDialog(
				"LoadImageDialog",
				"Choose Image File(s)",
				imageFilters,
				config
			);
		}

		// Process file dialog results
		void HandleUpdate() {
			// Handle the ImGuiFileDialog results
			if (ImGuiFileDialog::Instance()->Display("LoadVideoDialog", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();

					for (const auto&[fileName, filePath] : selection) {
						LoadVideoFile(filePath);
					}

					UpdateAvailableMedia();
				}
				ImGuiFileDialog::Instance()->Close();
			}

			if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", ImGuiWindowFlags_NoCollapse, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();

					for (const auto&[fileName, filePath] : selection) {
						LoadImageFile(filePath);
					}

					UpdateAvailableMedia();
				}
				ImGuiFileDialog::Instance()->Close();
			}
		}

		void LoadVideoFile(const std::string& filePath) {
			// Create a new entity with a video component
			auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
			if (!videoSystem) {
				std::cerr << "VideoSystem not found, cannot load video" << std::endl;
				return;
			}

			ECS::EntityID entityID = mgr.AddNewEntity();
			mgr.AddComponent<ECS::VideoComponent>(entityID);

			// Set the video directly using the system
			videoSystem->SetVideo(entityID, filePath);
		}

		void LoadImageFile(const std::string& filePath) {
			// Create a new entity with an image component
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (!imageSystem) {
				std::cerr << "ImageSystem not found, cannot load image" << std::endl;
				return;
			}

			ECS::EntityID entityID = mgr.AddNewEntity();
			mgr.AddComponent<ECS::ImageComponent>(entityID);
			auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(entityID);

			// Load the image
			try {
				cv::Mat img = cv::imread(filePath);
				if (img.empty()) {
					std::cerr << "Failed to load image: " << filePath << std::endl;
					mgr.DestroyEntity(entityID);
					return;
				}

				// Convert to RGB format
				cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

				// Get file info
				std::filesystem::path path(filePath);
				imageComp.fileName = path.filename().string();
				imageComp.filePath = filePath;
				imageComp.width = img.cols;
				imageComp.height = img.rows;

				// Create texture
				glGenTextures(1, &imageComp.textureID);
				glBindTexture(GL_TEXTURE_2D, imageComp.textureID);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				// Upload data
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageComp.width, imageComp.height,
					0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception while loading image: " << e.what() << std::endl;
				mgr.DestroyEntity(entityID);
			}
		}

		void AddVideoToSequence(ECS::EntityID entityID) {
			if (!mgr.HasComponent<ECS::VideoComponent>(entityID)) {
				return;
			}

			auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entityID);

			// Create a new clip
			VideoSequence::VideoClip newClip;
			newClip.type = VideoSequence::TYPE_VIDEO;

			// Find the first free spot in the timeline
			int lastEndFrame = 0;
			for (const auto& clip : sequence.clips) {
				if (clip.endFrame > lastEndFrame) {
					lastEndFrame = clip.endFrame;
				}
			}

			newClip.startFrame = lastEndFrame;

			// Calculate the end frame based on video duration
			int videoDuration = static_cast<int>(videoComp.frameCount / (videoComp.fps / sequenceFPS));
			newClip.endFrame = newClip.startFrame + videoDuration;

			// Ensure it doesn't exceed sequence length
			if (newClip.endFrame > sequence.frameMax) {
				// Either crop or extend the sequence
				if (newClip.endFrame > sequence.frameMax + 300) {  // If it's much longer, crop it
					newClip.endFrame = sequence.frameMax;
				}
				else {
					// Extend the sequence
					sequence.frameMax = newClip.endFrame;
				}
			}

			newClip.color = 0xFF66A8FF;  // Blue color for video clips
			newClip.entityID = entityID;
			newClip.name = videoComp.fileName;

			// Create a thumbnail for the clip
			if (videoComp.currentTexture != 0 && glIsTexture(videoComp.currentTexture)) {
				newClip.thumbnailTexture = videoComp.currentTexture;  // Use the same texture
			}

			// Add to sequence
			sequence.clips.push_back(newClip);
			sequenceModified = true;

			// Update the preview
			UpdatePreviewAtFrame(currentFrame);

			// If in media browser, close it after adding
			if (showMediaBrowser) {
				showMediaBrowser = false;
			}
		}

		void AddImageToSequence(ECS::EntityID entityID) {
			if (!mgr.HasComponent<ECS::ImageComponent>(entityID)) {
				return;
			}

			auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(entityID);

			// Create a new clip
			VideoSequence::VideoClip newClip;
			newClip.type = VideoSequence::TYPE_IMAGE;

			// Find the first free spot in the timeline
			int lastEndFrame = 0;
			for (const auto& clip : sequence.clips) {
				if (clip.endFrame > lastEndFrame) {
					lastEndFrame = clip.endFrame;
				}
			}

			newClip.startFrame = lastEndFrame;

			// Default duration for images: 3 seconds
			int imageDuration = static_cast<int>(3.0f * sequenceFPS);
			newClip.endFrame = newClip.startFrame + imageDuration;
			newClip.displayDuration = imageDuration;

			// Ensure it doesn't exceed sequence length
			if (newClip.endFrame > sequence.frameMax) {
				// Either crop or extend the sequence
				if (newClip.endFrame > sequence.frameMax + 300) {  // If it's much longer, crop it
					newClip.endFrame = sequence.frameMax;
					newClip.displayDuration = newClip.endFrame - newClip.startFrame;
				}
				else {
					// Extend the sequence
					sequence.frameMax = newClip.endFrame;
				}
			}

			newClip.color = 0xFFFFAA66;  // Orange color for image clips
			newClip.entityID = entityID;
			newClip.name = imageComp.fileName;

			// Create a thumbnail for the clip
			if (imageComp.textureID != 0 && glIsTexture(imageComp.textureID)) {
				newClip.thumbnailTexture = imageComp.textureID;  // Use the same texture
			}

			// Add to sequence
			sequence.clips.push_back(newClip);
			sequenceModified = true;

			// Update the preview
			UpdatePreviewAtFrame(currentFrame);

			// If in media browser, close it after adding
			if (showMediaBrowser) {
				showMediaBrowser = false;
			}
		}

		void ReplaceClipMedia(int clipIndex) {
			if (clipIndex < 0 || clipIndex >= sequence.clips.size()) {
				return;
			}

			auto& clip = sequence.clips[clipIndex];

			// Open the media browser to select new media
			showMediaBrowser = true;

			// TODO: Implement the actual replacement logic when a new media is selected
			// This would require keeping track of which clip we're replacing
		}

		// Callback handlers for video and image events
		void HandleVideoAdded(ECS::EntityID entityID) {
			// Update available videos list
			UpdateAvailableMedia();
		}

		void HandleVideoRemoved(ECS::EntityID entityID) {
			// Update available videos list
			UpdateAvailableMedia();

			// Remove any clips that reference this entity
			for (int i = static_cast<int>(sequence.clips.size()) - 1; i >= 0; i--) {
				if (sequence.clips[i].entityID == entityID && sequence.clips[i].type == VideoSequence::TYPE_VIDEO) {
					sequence.clips.erase(sequence.clips.begin() + i);
					sequenceModified = true;

					// If this was the selected clip, clear selection
					if (sequence.selectedEntry == i) {
						sequence.selectedEntry = -1;
					}
					// Adjust selection index if we removed a clip before the selected one
					else if (sequence.selectedEntry > i) {
						sequence.selectedEntry--;
					}
				}
			}
		}
	};

} // namespace GUI

#endif // VIDEO_SEQUENCER_VIEW_HPP