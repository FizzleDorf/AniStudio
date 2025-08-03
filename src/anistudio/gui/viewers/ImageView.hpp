/*
 * ImageView.hpp - Enhanced version with RenderContent override
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

		// Override RenderContent instead of Render to use BaseView's window close handling
		void RenderContent() override;

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