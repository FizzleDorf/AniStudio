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

#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "GUI.h"
#include "FilePaths.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "../events/Events.hpp"
#include <pch.h>

namespace GUI {

	class ImageView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Image View",
            "category": "Views",
            "description": "A simple image viewer"
        })";
		}

		ImageView(ECS::EntityManager& entityMgr);
		~ImageView();

		void Init() override;
		void Update(const float deltaT) override;
		void Render() override;

	private:
		ECS::EntityID selectedEntityID;
		int imgIndex;
		bool showHistory;
		size_t lastEntityCount; // Track entity count changes

		// Values for zoom and panning
		float zoom;
		float offsetX;
		float offsetY;

		// Cached entity list - updated by callbacks and polling
		std::vector<ECS::EntityID> imageEntities;

		// File filters
		const char* filters;

		// CALLBACK HANDLERS - Called by ImageSystem when images are loaded/removed
		void OnImageLoaded(ECS::EntityID entityID);
		void OnImageRemoved(ECS::EntityID entityID);
		void RefreshImageEntities();

		// Rendering methods
		void RenderImageInfo();
		void RenderControls();
		void RenderSelector();
		void RenderHistory();
		void RenderSelectedImage();
		void DrawGrid(int imageWidth, int imageHeight);

		// Image operations
		void SetZoom(float newZoom);
		void LoadImages(const std::vector<std::string>& filePaths);
		void SaveSelectedImage();
		void SaveSelectedImageAs(const std::string& filePath);
		void RemoveSelectedImage();

		// Utility methods
		std::string TruncateFilename(const std::string& filename, float maxTextWidth);
	};

} // namespace GUI

#endif // IMAGEVIEW_HPP