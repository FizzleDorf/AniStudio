#include "ImageView.hpp"
#include "ImageUtils.hpp"
#include "ImGuiFileDialog.h"
#include "Events.hpp"
#include "AssetManager.hpp"
#include "TextureSystem.hpp"
#include <algorithm>

namespace GUI {

	ImageView::ImageView(ECS::EntityManager& entityMgr)
		: BaseView(entityMgr),
		selectedEntityID(0),
		imgIndex(0),
		showHistory(true),
		zoom(1.0f),
		offsetX(0.0f),
		offsetY(0.0f),
		lastEntityCount(0),
		filters("Image files{.png,.jpg,.jpeg,.bmp,.tga}"
			".png,.jpg,.jpeg,.bmp,.tga"
			"{.png},PNG"
			"{.jpg,.jpeg},JPEG"
			"{.bmp},BMP"
			"{.tga},TGA"),
		contextMenuUtils(std::make_unique<Utils::ContextMenuUtils>(entityMgr))
	{
		viewName = "ImageView";
	}

	ImageView::~ImageView() {
	}

	void ImageView::Init() {
		// Ensure TextureSystem is registered for handling image textures
		auto textureSystem = mgr.GetSystem<ECS::TextureSystem>();
		if (!textureSystem) {
			mgr.RegisterSystem<ECS::TextureSystem>();
		}

		RefreshImageEntities();
	}

	void ImageView::Update(const float deltaT) {
		// Poll for changes in image entities
		size_t currentCount = 0;
		for (auto entityID : mgr.GetAllEntities()) {
			if (HasImageComponents(entityID)) {
				currentCount++;
			}
		}

		if (currentCount != lastEntityCount) {
			RefreshImageEntities();
			lastEntityCount = currentCount;

			// Handle selection changes when entities are added/removed
			if (imageEntities.empty()) {
				selectedEntityID = 0;
				imgIndex = 0;
			}
			else if (selectedEntityID == 0) {
				// Auto-select first image if none selected
				selectedEntityID = imageEntities[0];
				imgIndex = 0;
			}
			else {
				// Update imgIndex if selected entity still exists
				auto it = std::find(imageEntities.begin(), imageEntities.end(), selectedEntityID);
				if (it != imageEntities.end()) {
					imgIndex = static_cast<int>(std::distance(imageEntities.begin(), it));
				}
				else {
					// Selected entity was removed, select first available
					if (!imageEntities.empty()) {
						selectedEntityID = imageEntities[0];
						imgIndex = 0;
					}
					else {
						selectedEntityID = 0;
						imgIndex = 0;
					}
				}
			}
		}
	}

	void ImageView::Render() {
		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			

			RenderImageInfo();
			RenderControls();
			RenderSelector();

			ImGui::SameLine();
			ImGui::Checkbox("Show History", &showHistory);

			ImGui::Separator();

			if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
				RenderSelectedImage();
			}
			ImGui::EndChild();
		}
		ImGui::End();

		if (!windowOpen) {
			std::unordered_map<std::string, std::any> eventData;
			eventData["workspaceID"] = GetID();
			eventData["viewTypeName"] = viewName;
			ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
		}

		if (showHistory) {
			RenderHistory();
		}

		
	}

	void ImageView::OnImageLoaded(ECS::EntityID entityID) {
		RefreshImageEntities();

		if (!imageEntities.empty()) {
			auto it = std::find(imageEntities.begin(), imageEntities.end(), entityID);
			if (it != imageEntities.end()) {
				imgIndex = static_cast<int>(std::distance(imageEntities.begin(), it));
				selectedEntityID = entityID;
			}
		}
	}

	void ImageView::OnImageRemoved(ECS::EntityID entityID) {
		int previousIndex = imgIndex;
		RefreshImageEntities();

		if (selectedEntityID == entityID) {
			if (!imageEntities.empty()) {
				if (previousIndex >= static_cast<int>(imageEntities.size())) {
					imgIndex = static_cast<int>(imageEntities.size()) - 1;
				}
				else {
					imgIndex = std::max(0, std::min(previousIndex, static_cast<int>(imageEntities.size()) - 1));
				}

				if (imgIndex >= 0 && imgIndex < static_cast<int>(imageEntities.size())) {
					selectedEntityID = imageEntities[imgIndex];
				}
				else {
					selectedEntityID = 0;
					imgIndex = 0;
				}
			}
			else {
				selectedEntityID = 0;
				imgIndex = 0;
			}
		}
	}

	void ImageView::RefreshImageEntities() {
		try {
			imageEntities.clear();
			for (auto entityID : mgr.GetAllEntities()) {
				if (HasImageComponents(entityID)) {
					imageEntities.push_back(entityID);
				}
			}
			lastEntityCount = imageEntities.size();
		}
		catch (const std::exception& e) {
			imageEntities.clear();
			lastEntityCount = 0;
		}
	}

	void ImageView::RenderImageInfo() {
		if (selectedEntityID != 0 && mgr.IsEntityValid(selectedEntityID) &&
			mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
			try {
				const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);
				ImGui::Text("File: %s", imageComp.fileName.c_str());
				ImGui::Text("Dimensions: %dx%d", imageComp.width, imageComp.height);
				ImGui::Text("Channels: %d", imageComp.channels);
				ImGui::Text("Entity ID: %zu", selectedEntityID);

				// Show asset loading status
				if (imageComp.imageAssetId != INVALID_RESOURCE_ID) {
					auto imageAsset = AssetManager::Instance().GetAsset(imageComp.imageAssetId);
					if (imageAsset) {
						switch (imageAsset->GetLoadState()) {
						case LoadState::Loading:
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Loading...");
							break;
						case LoadState::Loaded:
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Loaded");
							break;
						case LoadState::Failed:
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: Failed to load");
							break;
						default:
							ImGui::Text("Status: Unknown");
							break;
						}
					}
				}

				contextMenuUtils->RenderImageContextMenu(selectedEntityID);

				ImGui::Separator();
			}
			catch (const std::exception& e) {
				ImGui::Text("Error reading image info: %s", e.what());
			}
		}
	}

	void ImageView::RenderControls() {
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

		if (contextMenuUtils->HasValidClipboardData()) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s", contextMenuUtils->GetClipboardPreview().c_str());
		}
	}

	void ImageView::RenderSelector() {
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

	void ImageView::RenderHistory() {
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
						(ImTextureID)(intptr_t)imageComp.textureID,
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

				contextMenuUtils->RenderImageContextMenu(entityID);

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

	void ImageView::RenderSelectedImage() {
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
			ImGui::Dummy(imageSize);

			ImGui::SetCursorPos(imagePos);

			ImGui::Image(
				(ImTextureID)(intptr_t)imageComp.textureID,
				imageSize
			);

			contextMenuUtils->RenderImageContextMenu(selectedEntityID);

			if (zoom > 1.0f && ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				offsetX += ImGui::GetIO().MouseDelta.x;
				offsetY += ImGui::GetIO().MouseDelta.y;
			}

			ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
			ImGui::Dummy(ImVec2(0, 0));
		}
		catch (const std::exception& e) {
			ImGui::Text("Error rendering image: %s", e.what());
		}
	}

	void ImageView::DrawGrid(int imageWidth, int imageHeight) {
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

	void ImageView::SetZoom(float newZoom) {
		zoom = std::clamp(newZoom, 0.1f, 5.0f);
	}

	void ImageView::LoadImages(const std::vector<std::string>& filePaths) {
		auto textureSystem = mgr.GetSystem<ECS::TextureSystem>();
		if (!textureSystem) {
			std::cerr << "[ImageView] TextureSystem not found!" << std::endl;
			return;
		}

		try {
			for (const auto& filePath : filePaths) {
				if (filePath.empty()) continue;

				ECS::EntityID entity = mgr.AddNewEntity();
				mgr.AddComponent<ECS::ImageComponent>(entity);

				// Load image using TextureSystem with AssetManager integration
				textureSystem->LoadImageTexture(entity, filePath, mgr);

				std::cout << "[ImageView] Started loading: " << filePath << " (Entity: " << entity << ")" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception loading images: " << e.what() << std::endl;
		}
	}

	void ImageView::SaveSelectedImage() {
		if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

		try {
			const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

			if (imageComp.imageAssetId != INVALID_RESOURCE_ID) {
				auto imageAsset = AssetManager::Instance().GetAsset<ImageAsset>(imageComp.imageAssetId);
				if (imageAsset && imageAsset->IsLoaded()) {
					int w, h, c;
					imageAsset->GetDimensions(w, h, c);
					unsigned char* data = imageAsset->GetImageData();

					if (data) {
						Utils::ImageUtils::SaveImage(imageComp.filePath, w, h, c, data);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception saving image: " << e.what() << std::endl;
		}
	}

	void ImageView::SaveSelectedImageAs(const std::string& filePath) {
		if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

		try {
			const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

			if (imageComp.imageAssetId != INVALID_RESOURCE_ID) {
				auto imageAsset = AssetManager::Instance().GetAsset<ImageAsset>(imageComp.imageAssetId);
				if (imageAsset && imageAsset->IsLoaded()) {
					int w, h, c;
					imageAsset->GetDimensions(w, h, c);
					unsigned char* data = imageAsset->GetImageData();

					if (data) {
						Utils::ImageUtils::SaveImage(filePath, w, h, c, data);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception saving image: " << e.what() << std::endl;
		}
	}

	void ImageView::RemoveSelectedImage() {
		if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

		try {
			const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

			// Unload assets from AssetManager
			if (imageComp.imageAssetId != INVALID_RESOURCE_ID) {
				AssetManager::Instance().UnloadAsset(imageComp.imageAssetId);
			}
			if (imageComp.textureAssetId != INVALID_RESOURCE_ID) {
				AssetManager::Instance().UnloadAsset(imageComp.textureAssetId);
			}

			// Destroy the entity
			mgr.DestroyEntity(selectedEntityID);

			// Update the view
			OnImageRemoved(selectedEntityID);
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception removing image: " << e.what() << std::endl;
		}
	}

	std::string ImageView::TruncateFilename(const std::string& filename, float maxTextWidth) {
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

	bool ImageView::HasImageComponents(ECS::EntityID entityId) const {
		return mgr.IsEntityValid(entityId) && (
			mgr.HasComponent<ECS::ImageComponent>(entityId) ||
			mgr.HasComponent<ECS::InputImageComponent>(entityId) ||
			mgr.HasComponent<ECS::OutputImageComponent>(entityId)
			);
	}

} // namespace GUI