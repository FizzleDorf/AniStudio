/*
 * ImageView.hpp - Enhanced version with ContextMenu integration
 */

#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "GUI.h"
#include "FilePaths.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "ContextMenuUtils.hpp"
#include <pch.h>

namespace GUI {

	class ImageView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Image View",
            "category": "Viewers",
            "description": "A simple image viewer with copy/paste functionality"
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
		size_t lastEntityCount;

		// Values for zoom and panning
		float zoom;
		float offsetX;
		float offsetY;

		// Cached entity list - updated by callbacks and polling
		std::vector<ECS::EntityID> imageEntities;

		// File filters
		const char* filters;

		// Context menu utilities
		std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

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

		// Context menu rendering
		void RenderImageContextMenus();

		// Image operations
		void SetZoom(float newZoom);
		void LoadImages(const std::vector<std::string>& filePaths);
		void SaveSelectedImage();
		void SaveSelectedImageAs(const std::string& filePath);
		void RemoveSelectedImage();

		// Utility methods
		std::string TruncateFilename(const std::string& filename, float maxTextWidth);

		// Helper to check if entity has image components
		bool HasImageComponents(ECS::EntityID entityId) const;
	};

} // namespace GUI

#endif // IMAGEVIEW_HPP