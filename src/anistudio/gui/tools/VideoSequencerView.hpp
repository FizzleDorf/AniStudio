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

#pragma once

#include "GUI.h"
#include "VideoComponent.hpp"
#include "ImageComponent.hpp"
#include "VideoSystem.hpp"
#include "ImageSystem.hpp"
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
		int GetFrameMin() const override;
		int GetFrameMax() const override;
		int GetItemCount() const override;
		const char* GetItemTypeName(int typeIndex) const override;
		const char* GetItemLabel(int index) const override;
		void Get(int index, int** start, int** end, int* type, unsigned int* color) override;
		void Add(int type) override;
		void Del(int index) override;
		void Duplicate(int index) override;
		void Copy() override;
		void Paste() override;
		size_t GetCustomHeight(int index) override;
		void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect,
			const ImRect& clippingRect, const ImRect& legendClippingRect) override;
		void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override;
		void BeginEdit(int index) override;
		void EndEdit() override;
		int GetItemTypeCount() const override;
		void DoubleClick(int index) override;
		const char* GetCollapseFmt() const override;
	};

	class VideoSequencerView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Video Sequencer",
            "category": "Video",
            "description": "Create and edit video sequences with timeline support."
        })";
		}

		VideoSequencerView(ECS::EntityManager& entityMgr);
		~VideoSequencerView();

		void Init() override;
		void Update(float deltaT) override;
		void Render() override;

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
		const char* videoFilters;
		const char* imageFilters;

		// Private methods
		void RenderToolbar();
		void RenderPreview();
		void RenderTimeline();
		void RenderClipProperties();
		void RenderMediaBrowser();
		void RenderVideoBrowser();
		void RenderImageBrowser();

		ImVec2 CalculatePreviewSize(const ImVec2& availableSize);
		std::string FormatTime(float seconds);

		void InitializePreviewTexture();
		void UpdatePreviewAtFrame(int frame);
		void UpdatePreviewFromVideoClip(const VideoSequence::VideoClip& clip, int clipFrame);
		void UpdatePreviewFromImageClip(const VideoSequence::VideoClip& clip);
		void CopyTexture(GLuint sourceTexture, GLuint destTexture);
		void ClearPreview();

		void UpdateAvailableMedia();
		void LoadVideoFromDisk();
		void LoadImageFromDisk();
		void HandleUpdate();
		void LoadVideoFile(const std::string& filePath);
		void LoadImageFile(const std::string& filePath);

		void AddVideoToSequence(ECS::EntityID entityID);
		void AddImageToSequence(ECS::EntityID entityID);
		void ReplaceClipMedia(int clipIndex);

		// Callback handlers for video and image events
		void HandleVideoAdded(ECS::EntityID entityID);
		void HandleVideoRemoved(ECS::EntityID entityID);
	};

} // namespace GUI