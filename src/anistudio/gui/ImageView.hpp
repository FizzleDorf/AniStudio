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

		ImageView(ECS::EntityManager& entityMgr)
			: BaseView(entityMgr),
			selectedEntityID(0),
			imgIndex(0),
			showHistory(true),
			zoom(1.0f),
			offsetX(0.0f),
			offsetY(0.0f),
			lastEntityCount(0)
		{
			viewName = "ImageView";
			std::cout << "[ImageView] Constructor called" << std::endl;
		}

		void Init() override {
			std::cout << "[ImageView] Initializing..." << std::endl;

			// Ensure ImageSystem exists
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (!imageSystem) {
				std::cout << "[ImageView] Registering ImageSystem..." << std::endl;
				mgr.RegisterSystem<ECS::ImageSystem>();
				imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			}

			if (imageSystem) {
				// Register callback for when images are added successfully
				imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entityID) {
					OnImageLoaded(entityID);
				});

				// Register callback for when images are removed
				imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entityID) {
					OnImageRemoved(entityID);
				});

				std::cout << "[ImageView] Registered callbacks with ImageSystem" << std::endl;
			}

			RefreshImageEntities();

			std::cout << "[ImageView] Initialization complete - CALLBACKS REGISTERED" << std::endl;
		}

		void Update(const float deltaT) override {
			// NOTE: We no longer need to poll for changes in Update since we use callbacks
			// The callbacks will handle immediate updates when images are loaded/removed
		}

		void Render() override {
			if (!ImGui::GetCurrentContext()) {
				std::cerr << "[ImageView] ERROR: No ImGui context!" << std::endl;
				return;
			}

			ImGui::SetNextWindowSize(ImVec2(1024, 1024), ImGuiCond_FirstUseEver);

			std::string windowName = "Image Viewer##" + std::to_string(GetID());
			bool windowOpen = true;

			if (!ImGui::Begin(windowName.c_str(), &windowOpen)) {
				ImGui::End();
				return;
			}

			try {
				RenderImageInfo();
				RenderControls();
				RenderSelector();

				ImGui::SameLine();
				ImGui::Checkbox("Show History", &showHistory);

				if (showHistory) {
					RenderHistory();
				}

				ImGui::Separator();

				if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
					RenderSelectedImage();
				}
				ImGui::EndChild();
			}
			catch (const std::exception& e) {
				std::cerr << "[ImageView] Exception in Render: " << e.what() << std::endl;
				ImGui::Text("Error rendering ImageView: %s", e.what());
			}

			ImGui::End();
		}

		~ImageView() {
			std::cout << "[ImageView] Destructor called - Note: ImageSystem callbacks are stored by value so no cleanup needed" << std::endl;
			// NOTE: The ImageSystem stores callbacks in a vector by value (lambdas)
			// When this view is destroyed, the lambdas become invalid but that's handled
			// by the capture-by-value nature of the lambdas
		}

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
		const char* filters = "Image files{.png,.jpg,.jpeg,.bmp,.tga}"
			".png,.jpg,.jpeg,.bmp,.tga"
			"{.png},PNG"
			"{.jpg,.jpeg},JPEG"
			"{.bmp},BMP"
			"{.tga},TGA";

		// CALLBACK HANDLERS - Called by ImageSystem when images are loaded/removed
		void OnImageLoaded(ECS::EntityID entityID) {
			std::cout << "[ImageView] CALLBACK: Image added for entity " << entityID << std::endl;

			// Refresh the entity list
			RefreshImageEntities();

			// Set the selection to the newly loaded image (last in the list)
			if (!imageEntities.empty()) {
				// Find the index of the loaded entity
				auto it = std::find(imageEntities.begin(), imageEntities.end(), entityID);
				if (it != imageEntities.end()) {
					imgIndex = static_cast<int>(std::distance(imageEntities.begin(), it));
					selectedEntityID = entityID;
					std::cout << "[ImageView] Selected newly loaded image: Entity " << entityID << " at index " << imgIndex << std::endl;
				}
			}
		}

		void OnImageRemoved(ECS::EntityID entityID) {
			std::cout << "[ImageView] CALLBACK: Image removed for entity " << entityID << std::endl;

			// Store the current index before refresh
			int previousIndex = imgIndex;

			// Refresh the entity list
			RefreshImageEntities();

			// Handle selection changes after removal
			if (selectedEntityID == entityID) {
				// The selected image was removed, select previous image or adjust selection
				if (!imageEntities.empty()) {
					// If we were at the last image, go to the new last image
					if (previousIndex >= static_cast<int>(imageEntities.size())) {
						imgIndex = static_cast<int>(imageEntities.size()) - 1;
					}
					// Otherwise try to stay at the same index (which now points to the next image)
					else {
						imgIndex = std::max(0, std::min(previousIndex, static_cast<int>(imageEntities.size()) - 1));
					}

					if (imgIndex >= 0 && imgIndex < static_cast<int>(imageEntities.size())) {
						selectedEntityID = imageEntities[imgIndex];
						std::cout << "[ImageView] Selected previous image: Entity " << selectedEntityID << " at index " << imgIndex << std::endl;
					}
					else {
						selectedEntityID = 0;
						imgIndex = 0;
					}
				}
				else {
					// No images left
					selectedEntityID = 0;
					imgIndex = 0;
					std::cout << "[ImageView] No images remaining after removal" << std::endl;
				}
			}
		}

		void RefreshImageEntities() {
			try {
				auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
				if (imageSystem) {
					imageEntities = imageSystem->GetAllImageEntities();
					lastEntityCount = imageEntities.size();
					std::cout << "[ImageView] Refreshed entities, found " << imageEntities.size() << " images" << std::endl;
				}
				else {
					imageEntities.clear();
					lastEntityCount = 0;
					std::cout << "[ImageView] No ImageSystem found, cleared entity list" << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ImageView] Exception refreshing entities: " << e.what() << std::endl;
				imageEntities.clear();
				lastEntityCount = 0;
			}
		}

		void RenderImageInfo() {
			if (selectedEntityID != 0 && mgr.IsEntityValid(selectedEntityID) &&
				mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
				try {
					const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);
					ImGui::Text("File: %s", imageComp.fileName.c_str());
					ImGui::Text("Dimensions: %dx%d", imageComp.width, imageComp.height);
					ImGui::Text("Channels: %d", imageComp.channels);
					ImGui::Text("Entity ID: %zu", selectedEntityID);
					ImGui::Separator();
				}
				catch (const std::exception& e) {
					ImGui::Text("Error reading image info: %s", e.what());
				}
			}
		}

		void RenderControls() {
			if (ImGui::Button("Load Image(s)")) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.countSelectionMax = 0;
				ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image(s)",
					filters, config);
			}

			if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();
					std::vector<std::string> filePaths;
					for (const auto&[fileName, filePath] : selection) {
						filePaths.push_back(filePath);
					}
					LoadImages(filePaths);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();

			if (selectedEntityID != 0 && ImGui::Button("Save Image")) {
				SaveSelectedImage();
			}

			ImGui::SameLine();

			if (selectedEntityID != 0 && ImGui::Button("Save Image As")) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				ImGuiFileDialog::Instance()->OpenDialog("SaveImageAsDialog", "Save Image As",
					filters, config);
			}

			if (ImGuiFileDialog::Instance()->Display("SaveImageAsDialog")) {
				if (ImGuiFileDialog::Instance()->IsOk() && selectedEntityID != 0) {
					std::string savePath = ImGuiFileDialog::Instance()->GetFilePathName();
					SaveSelectedImageAs(savePath);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();

			if (selectedEntityID != 0 && ImGui::Button("Remove Image")) {
				RemoveSelectedImage();
			}

			ImGui::SameLine();

			if (ImGui::Button("Refresh")) {
				RefreshImageEntities();
			}
		}

		void RenderSelector() {
			if (imageEntities.empty()) {
				ImGui::Text("No images loaded.");
				return;
			}

			if (ImGui::Button("First")) {
				if (!imageEntities.empty()) {
					imgIndex = 0;
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Previous")) {
				if (!imageEntities.empty()) {
					imgIndex = (imgIndex - 1 + static_cast<int>(imageEntities.size())) % static_cast<int>(imageEntities.size());
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Next")) {
				if (!imageEntities.empty()) {
					imgIndex = (imgIndex + 1) % static_cast<int>(imageEntities.size());
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Last")) {
				if (!imageEntities.empty()) {
					imgIndex = static_cast<int>(imageEntities.size() - 1);
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::SameLine();

			if (ImGui::InputInt("Current Image", &imgIndex)) {
				if (!imageEntities.empty()) {
					const int size = static_cast<int>(imageEntities.size());
					if (size == 1) {
						imgIndex = 0;
					}
					else {
						imgIndex = ((imgIndex % size) + size) % size;
					}
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::Text("Image %d of %zu", imgIndex + 1, imageEntities.size());
		}

		void RenderHistory() {
			ImGui::Begin("History", &showHistory);

			if (imageEntities.empty()) {
				ImGui::Text("No images available.");
				ImGui::End();
				return;
			}

			float currentRowWidth = 0.0f;

			for (size_t i = 0; i < imageEntities.size(); ++i) {
				ECS::EntityID entityID = imageEntities[i];

				if (!mgr.IsEntityValid(entityID) || !mgr.HasComponent<ECS::ImageComponent>(entityID)) {
					continue;
				}

				try {
					const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(entityID);

					ImGui::BeginGroup();

					if (static_cast<int>(i) == imgIndex) {
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
					}

					float aspectRatio = (imageComp.height > 0) ?
						static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height) : 1.0f;
					ImVec2 maxSize(128.0f, 128.0f);
					ImVec2 imageSize;

					if (aspectRatio > 1.0f) {
						imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
					}
					else {
						imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
					}

					ImGui::Text("%zu: %s", i, TruncateFilename(imageComp.fileName, imageSize.x).c_str());

					if (static_cast<int>(i) == imgIndex) {
						ImGui::PopStyleColor();
					}

					if (imageComp.textureID != 0) {
						if (ImGui::ImageButton(("##img" + std::to_string(i)).c_str(),
							reinterpret_cast<void*>(static_cast<intptr_t>(imageComp.textureID)),
							imageSize)) {
							imgIndex = static_cast<int>(i);
							selectedEntityID = entityID;
						}
					}
					else {
						if (ImGui::Button(("Select##" + std::to_string(i)).c_str(), imageSize)) {
							imgIndex = static_cast<int>(i);
							selectedEntityID = entityID;
						}
					}

					ImGui::EndGroup();

					float buttonWidth = imageSize.x + ImGui::GetStyle().ItemSpacing.x;
					currentRowWidth += buttonWidth;

					if (i < imageEntities.size() - 1) {
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
				catch (const std::exception& e) {
					ImGui::Text("Error with image %zu: %s", i, e.what());
				}
			}

			ImGui::End();
		}

		void RenderSelectedImage() {
			if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID) ||
				!mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
				ImGui::Text("No image selected or entity invalid.");
				return;
			}

			try {
				const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

				if (imageComp.textureID == 0 || imageComp.width <= 0 || imageComp.height <= 0) {
					ImGui::Text("Image loading... (Texture ID: %u, Size: %dx%d)",
						imageComp.textureID, imageComp.width, imageComp.height);
					return;
				}

				if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
					float newZoom = zoom + ImGui::GetIO().MouseWheel * 0.1f;
					SetZoom(newZoom);
				}

				ImVec2 imageSize = ImVec2(imageComp.width * zoom, imageComp.height * zoom);
				ImVec2 windowSize = ImGui::GetWindowSize();
				ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;

				if (zoom <= 1.0f) {
					offsetX = (windowSize.x - imageSize.x) * 0.5f;
					offsetY = (windowSize.y - imageSize.y) * 0.5f;
				}

				ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

				DrawGrid(imageComp.width, imageComp.height);

				ImGui::SetCursorPos(imagePos);

				ImGui::Image(
					reinterpret_cast<void*>(static_cast<intptr_t>(imageComp.textureID)),
					imageSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1)
				);

				if (zoom > 1.0f && ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					offsetX += ImGui::GetIO().MouseDelta.x;
					offsetY += ImGui::GetIO().MouseDelta.y;
				}

				ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
			}
			catch (const std::exception& e) {
				ImGui::Text("Error rendering image: %s", e.what());
			}
		}

		void DrawGrid(int imageWidth, int imageHeight) {
			if (imageWidth <= 0 || imageHeight <= 0) return;

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			const float gridStep = 100.0f * zoom;

			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImVec2 contentMin = windowPos;
			const ImVec2 contentMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);

			float startX = contentMin.x - fmodf(ImGui::GetScrollX(), gridStep);
			float startY = contentMin.y - fmodf(ImGui::GetScrollY(), gridStep);

			for (float x = startX; x < contentMax.x; x += gridStep) {
				draw_list->AddLine(ImVec2(x, contentMin.y), ImVec2(x, contentMax.y), IM_COL32(255, 255, 255, 50));
			}

			for (float y = startY; y < contentMax.y; y += gridStep) {
				draw_list->AddLine(ImVec2(contentMin.x, y), ImVec2(contentMax.x, y), IM_COL32(255, 255, 255, 50));
			}
		}

		void SetZoom(float newZoom) {
			zoom = std::clamp(newZoom, 0.1f, 5.0f);
		}

		void LoadImages(const std::vector<std::string>& filePaths) {
			std::cout << "[ImageView] Loading " << filePaths.size() << " images..." << std::endl;

			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (!imageSystem) {
				std::cerr << "[ImageView] Error: ImageSystem not found!" << std::endl;
				return;
			}

			try {
				for (const auto& filePath : filePaths) {
					if (filePath.empty()) continue;

					ECS::EntityID entity = mgr.AddNewEntity();
					mgr.AddComponent<ECS::ImageComponent>(entity);
					imageSystem->SetImage(entity, filePath);

					std::cout << "[ImageView] Started loading: " << filePath << " (Entity: " << entity << ")" << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ImageView] Exception loading images: " << e.what() << std::endl;
			}
		}

		void SaveSelectedImage() {
			if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

			try {
				const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

				if (Utils::ImageUtils::SaveImage(
					imageComp.filePath,
					imageComp.width,
					imageComp.height,
					imageComp.channels,
					imageComp.imageData)) {
					std::cout << "[ImageView] Image saved to: " << imageComp.filePath << std::endl;
				}
				else {
					std::cerr << "[ImageView] Failed to save image" << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ImageView] Exception saving image: " << e.what() << std::endl;
			}
		}

		void SaveSelectedImageAs(const std::string& filePath) {
			if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

			try {
				const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

				if (Utils::ImageUtils::SaveImage(
					filePath,
					imageComp.width,
					imageComp.height,
					imageComp.channels,
					imageComp.imageData)) {
					std::cout << "[ImageView] Image saved to: " << filePath << std::endl;
				}
				else {
					std::cerr << "[ImageView] Failed to save image to: " << filePath << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ImageView] Exception saving image: " << e.what() << std::endl;
			}
		}

		void RemoveSelectedImage() {
			if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

			try {
				auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
				if (imageSystem) {
					imageSystem->RemoveImage(selectedEntityID);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ImageView] Exception removing image: " << e.what() << std::endl;
			}
		}

		std::string TruncateFilename(const std::string& filename, float maxTextWidth) {
			if (filename.empty()) return "Unknown";

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

#endif // IMAGEVIEW_HPP