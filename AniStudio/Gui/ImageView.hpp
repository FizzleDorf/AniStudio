#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Base/BaseView.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "../Events/Events.hpp"
#include <pch.h>

namespace GUI {

	class ImageView : public BaseView {
	public:
		ImageView(ECS::EntityManager& entityMgr)
			: BaseView(entityMgr),
			selectedEntityID(0),
			imgIndex(0),
			showHistory(true),
			zoom(1.0f),
			offsetX(0.0f),
			offsetY(0.0f)
		{
			viewName = "ImageView";
		}

		void Init() override {
			// Ensure ImageSystem is registered
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (!imageSystem) {
				mgr.RegisterSystem<ECS::ImageSystem>();
				imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			}

			// Register callbacks to update the view when images change
			// We're registering these in Init instead of constructor to ensure everything is properly initialized
			if (imageSystem) {
				imageSystem->RegisterImageAddedCallback(std::bind(&ImageView::HandleImageAdded, this, std::placeholders::_1));
				imageSystem->RegisterImageRemovedCallback(std::bind(&ImageView::HandleImageRemoved, this, std::placeholders::_1));
			}

			// Try to select first image if nothing is selected
			if (selectedEntityID == 0) {
				auto imageEntities = GetImageEntities();
				if (!imageEntities.empty()) {
					selectedEntityID = imageEntities[0];
					imgIndex = 0;
				}
			}
		}

		void Render() override {
			ImGui::SetNextWindowSize(ImVec2(1024, 1024), ImGuiCond_FirstUseEver);
			ImGui::Begin("Image Viewer", nullptr);

			// Show image information if one is selected
			if (selectedEntityID != 0 && mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
				const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);
				ImGui::Text("File: %s", imageComp.fileName.c_str());
				ImGui::Text("Dimensions: %dx%d", imageComp.width, imageComp.height);
				ImGui::Separator();
			}

			RenderSelector();

			if (ImGui::Button("Load Image(s)")) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				// Enable multiple selection
				config.countSelectionMax = 0; // 0 means infinite selections
				ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image(s)",
					filters, config);
			}

			if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					// Get multiple selections
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

			// Option to show/hide history panel
			ImGui::SameLine();
			ImGui::Checkbox("Show History", &showHistory);

			if (showHistory) {
				RenderHistory();
			}

			ImGui::Separator();

			if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
				RenderSelectedImage();
				ImGui::EndChild();
			}

			ImGui::End();
		}

		~ImageView() {
			// Unregister callbacks if possible to prevent accessing deleted object
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (imageSystem) {
				// We would need a way to unregister callbacks here
				// This would require the ImageSystem to have a method to remove callbacks
				// For now, we'll rely on the system not calling callbacks on deleted views
			}
		}

	private:
		ECS::EntityID selectedEntityID;
		int imgIndex;
		bool showHistory;

		// Values for zoom and panning
		float zoom;
		float offsetX;
		float offsetY;

		// File filters
		const char* filters = "Image files{.png,.jpg,.jpeg,.bmp,.tga}"
			".png,.jpg,.jpeg,.bmp,.tga"
			"{.png},PNG"
			"{.jpg,.jpeg},JPEG"
			"{.bmp},BMP"
			"{.tga},TGA";

		// Get current list of image entities directly from the ImageSystem
		std::vector<ECS::EntityID> GetImageEntities() const {
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (imageSystem) {
				return imageSystem->GetAllImageEntities();
			}
			return {}; // Empty vector if no system or no entities
		}

		// Get the index of the currently selected image
		int GetCurrentImageIndex() const {
			if (selectedEntityID == 0) return -1;

			auto imageEntities = GetImageEntities();
			auto it = std::find(imageEntities.begin(), imageEntities.end(), selectedEntityID);
			if (it != imageEntities.end()) {
				return static_cast<int>(std::distance(imageEntities.begin(), it));
			}
			return -1; // Not found
		}

		// Callback handlers
		void HandleImageAdded(ECS::EntityID entityID) {
			// Set the newly added image as the selected one
			imageEntities = GetImageEntities();
			selectedEntityID = entityID;
			imgIndex = GetCurrentImageIndex();

			// Log the operation
			std::cout << "New image added and selected: EntityID=" << entityID << ", Index=" << imgIndex << std::endl;
		}

		void HandleImageRemoved(ECS::EntityID entityID) {
			imageEntities = GetImageEntities();
			// Check if the removed entity was the selected one
			if (selectedEntityID == entityID) {
				// Get current image list

				if (imageEntities.empty()) {
					// No images left
					selectedEntityID = 0;
					imgIndex = 0;
					std::cout << "Selected image was removed. No images remaining." << std::endl;
				}
				else {
					// Try to keep the same index if possible
					if (imgIndex >= static_cast<int>(imageEntities.size())) {
						imgIndex = static_cast<int>(imageEntities.size() - 1);
					}

					// Set the new selected entity
					selectedEntityID = imageEntities[imgIndex];
					std::cout << "Selected image was removed. New selection: EntityID="
						<< selectedEntityID << ", Index=" << imgIndex << std::endl;
				}
			}
			else {
				// If a different image was removed, we need to update the imgIndex
				// since the overall collection has changed
				imgIndex = GetCurrentImageIndex();
				std::cout << "Image removed: EntityID=" << entityID << ". Current selection updated to index: " << imgIndex << std::endl;
			}
		}

		void RenderSelector() {
			if (imageEntities.empty()) {
				ImGui::Text("No images loaded.");
				return;
			}

			// Navigation buttons
			if (ImGui::Button("First")) {
				if (!imageEntities.empty()) {
					imgIndex = 0;
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Last")) {
				if (!imageEntities.empty()) {
					// Fix potential size_t to int conversion warning
					imgIndex = static_cast<int>(imageEntities.size() - 1);
					selectedEntityID = imageEntities[imgIndex];
				}
			}

			ImGui::SameLine();

			// Image index selection
			if (ImGui::InputInt("Current Image", &imgIndex)) {
				if (imageEntities.empty()) {
					imgIndex = 0;
					return;
				}

				// Fix potential size_t to int conversion warning
				const int size = static_cast<int>(imageEntities.size());
				if (size == 1) {
					imgIndex = 0;
				}
				else {
					if (imgIndex < 0) {
						imgIndex = size - 1;
					}
					imgIndex = (imgIndex % size + size) % size;
				}

				selectedEntityID = imageEntities[imgIndex];
			}
		}

		void RenderHistory() {
			ImGui::Begin("History", &showHistory);

			if (ImGui::Button("Refresh")) {
				imageEntities = GetImageEntities();
			}

			if (imageEntities.empty()) {
				ImGui::Text("No images available.");
				ImGui::End();
				return;
			}

			// Track the total width of buttons in the current row
			float currentRowWidth = 0.0f;

			// Create scrollable history panel
			for (size_t i = 0; i < imageEntities.size(); ++i) {
				ECS::EntityID entityID = imageEntities[i];

				if (!mgr.HasComponent<ECS::ImageComponent>(entityID)) {
					continue;
				}

				const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(entityID);
				if (imageComp.textureID != 0) {
					ImGui::BeginGroup();

					// Highlight the selected image - Fix size_t to int comparison
					if (static_cast<int>(i) == imgIndex) {
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
					}

					// Calculate image dimensions for the thumbnail
					float aspectRatio = static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height);
					ImVec2 maxSize(128.0f, 128.0f);
					ImVec2 imageSize;

					if (aspectRatio > 1.0f) {
						imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
					}
					else {
						imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
					}

					// Display the filename - use size_t for index formatting
					ImGui::Text("%zu: %s", i, TruncateFilename(imageComp.fileName, imageSize.x).c_str());

					if (static_cast<int>(i) == imgIndex) {
						ImGui::PopStyleColor();
					}

					// Make the image clickable
					if (imageComp.textureID != 0) {
						if (ImGui::ImageButton(("##img" + std::to_string(i)).c_str(),
							reinterpret_cast<void*>(static_cast<intptr_t>(imageComp.textureID)),
							imageSize)) {
							// Fix potential size_t to int conversion warning
							imgIndex = static_cast<int>(i);
							selectedEntityID = entityID;
						}
					}
					else {
						// Fallback if no texture
						if (ImGui::Button(("Select##" + std::to_string(i)).c_str(), imageSize)) {
							// Fix potential size_t to int conversion warning
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
			}

			ImGui::End();
		}

		void RenderSelectedImage() {
			if (selectedEntityID == 0 || !mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
				ImGui::Text("No image selected.");
				return;
			}

			const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

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

			// Calculate image size and position
			ImVec2 imageSize = ImVec2(imageComp.width * zoom, imageComp.height * zoom);

			// Center the image when zoomed out
			if (zoom <= 1.0f) {
				offsetX = (windowSize.x - imageSize.x) * 0.5f;
				offsetY = (windowSize.y - imageSize.y) * 0.5f;
			}

			ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

			// Draw grid before the image
			DrawGrid(imageComp.width, imageComp.height);

			// Set cursor position and render image
			ImGui::SetCursorPos(imagePos);
			if (imageComp.textureID != 0) {
				ImGui::Image((void*)(intptr_t)imageComp.textureID, imageSize,
					ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1));

				// Handle panning only when the image is hovered and zoomed in
				if (zoom > 1.0f && ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					offsetX += ImGui::GetIO().MouseDelta.x;
					offsetY += ImGui::GetIO().MouseDelta.y;
				}
			}
			else {
				ImGui::Text("No image data or texture available.");
			}

			// Ensure the content size is set to accommodate the image
			ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
		}

		void DrawGrid(int imageWidth, int imageHeight) {
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

		void LoadImages(const std::vector<std::string>& filePaths) {
			// Debug output first
			std::cout << "Selected files to load:" << std::endl;
			for (const auto& filePath : filePaths) {
				std::cout << "  - " << filePath << std::endl;
			}

			// Get ImageSystem
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (!imageSystem) {
				std::cerr << "Error: ImageSystem not found!" << std::endl;
				return;
			}

			// Create entities and load images directly
			ECS::EntityID lastEntity = 0;
			for (const auto& filePath : filePaths) {
				if (filePath.empty()) {
					continue; // Skip empty paths
				}

				// Create entity with component
				ECS::EntityID entity = mgr.AddNewEntity();
				lastEntity = entity; // Keep track of the last entity loaded

				mgr.AddComponent<ECS::ImageComponent>(entity);

				// Load the image directly - this will also trigger the callback
				imageSystem->SetImage(entity, filePath);
			}
		}

		void SaveSelectedImage() {
			if (selectedEntityID == 0) return;

			// Use existing filepath
			const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

			// Save directly via ImageSystem instead of using events
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (imageSystem) {
				Utils::ImageUtils::SaveImage(
					imageComp.filePath,
					imageComp.width,
					imageComp.height,
					imageComp.channels,
					imageComp.imageData
				);
				std::cout << "Image saved to: " << imageComp.filePath << std::endl;
			}
		}

		void SaveSelectedImageAs(const std::string& filePath) {
			if (selectedEntityID == 0) return;

			// Get the image component
			const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

			// Save directly via ImageUtils
			bool success = Utils::ImageUtils::SaveImage(
				filePath,
				imageComp.width,
				imageComp.height,
				imageComp.channels,
				imageComp.imageData
			);

			if (success) {
				std::cout << "Image saved to: " << filePath << std::endl;
			}
			else {
				std::cerr << "Failed to save image to: " << filePath << std::endl;
			}
		}

		void RemoveSelectedImage() {
			if (selectedEntityID == 0) return;

			// Get the image system
			auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
			if (imageSystem) {
				// Remove via the system to trigger callbacks properly
				imageSystem->RemoveImage(selectedEntityID);
			}
		}

		// Truncate the filename to fit the width of the image
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
		private:
			std::vector<ECS::EntityID> imageEntities;
	};

} // namespace GUI

#endif // IMAGEVIEW_HPP