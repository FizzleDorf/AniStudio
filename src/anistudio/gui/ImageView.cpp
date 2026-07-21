// ImageView.cpp
#include "ImageView.hpp"
#include "ImageUtils.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "TextureSystem.hpp"
#include "FilePathSystem.hpp"
#include <algorithm>

namespace GUI {

	void ImageView::Init() {
		imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
		if (!imageSystem) {
			m_entityManager.RegisterSystem<ECS::ImageSystem>();
			imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
		}

		if (imageSystem) {
			imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entityID) {
				OnImageLoaded(entityID);
				});

			imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entityID) {
				OnImageRemoved(entityID);
				});
		}

		RefreshImageEntities();
	}

	void ImageView::Update(const float deltaT) {
		size_t currentCount = 0;
		for (auto entityID : m_entityManager.GetAllEntities()) {
			if (IsImageComponentOnly(entityID)) {
				currentCount++;
			}
		}

		if (currentCount != lastEntityCount) {
			RefreshImageEntities();
			lastEntityCount = currentCount;

			if (imageEntities.empty()) {
				selectedEntityID = 0;
				imgIndex = 0;
			}
			else if (selectedEntityID == 0) {
				selectedEntityID = imageEntities[0];
				imgIndex = 0;
			}
			else {
				auto it = std::find(imageEntities.begin(), imageEntities.end(), selectedEntityID);
				if (it != imageEntities.end()) {
					imgIndex = static_cast<int>(std::distance(imageEntities.begin(), it));
				}
				else {
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
		if (showHistory) {
			RenderHistory();
		}

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
			RenderMenuBar();

			RenderImageInfo();
			RenderControls();
			RenderSelector();

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
	}

	void ImageView::RenderMenuBar() {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Load Image(s)")) {
					auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
					std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : "";
					if (defaultPath.empty()) {
						defaultPath = ".";
					}
					std::vector<std::string> outPaths;
					if (FileDialog::OpenFiles("Choose Image(s)", FileDialog::FilterType::IMAGE_FILE, outPaths, defaultPath)) {
						if (!outPaths.empty()) {
							LoadImages(outPaths);
						}
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save Image", nullptr, false, selectedEntityID != 0)) {
					SaveSelectedImage();
				}

				if (ImGui::MenuItem("Save Image As...", nullptr, false, selectedEntityID != 0)) {
					auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
					std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : "";
					if (defaultPath.empty()) {
						defaultPath = ".";
					}
					std::string outPath;
					if (selectedEntityID != 0) {
						const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
						std::string defaultName = imageComp.fileName;
						if (FileDialog::SaveFile("Save Image As", FileDialog::FilterType::IMAGE_FILE, defaultName, outPath, defaultPath)) {
							SaveSelectedImageAs(outPath);
						}
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Remove Image", nullptr, false, selectedEntityID != 0)) {
					RemoveSelectedImage();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Refresh")) {
					RefreshImageEntities();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				if (ImGui::MenuItem("Show History", nullptr, &showHistory)) {
				}

				ImGui::Separator();

				if (ImGui::MenuItem("First Image", nullptr, false, !imageEntities.empty())) {
					if (!imageEntities.empty()) {
						imgIndex = 0;
						selectedEntityID = imageEntities[imgIndex];
					}
				}

				if (ImGui::MenuItem("Last Image", nullptr, false, !imageEntities.empty())) {
					if (!imageEntities.empty()) {
						imgIndex = static_cast<int>(imageEntities.size() - 1);
						selectedEntityID = imageEntities[imgIndex];
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Auto-switch on Load", nullptr, &autoSwitchOnLoad)) {
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	void ImageView::OnImageLoaded(ECS::EntityID entityID) {
		RefreshImageEntities();

		if (!imageEntities.empty() && autoSwitchOnLoad) {
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
		imageEntities.clear();
		for (auto entityID : m_entityManager.GetAllEntities()) {
			if (IsImageComponentOnly(entityID)) {
				imageEntities.push_back(entityID);
			}
		}
		lastEntityCount = imageEntities.size();
	}

	void ImageView::RenderImageInfo() {
		if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
			m_entityManager.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
			try {
				const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
				ImGui::Text("File: %s", imageComp.fileName.c_str());
				ImGui::Text("Dimensions: %dx%d", imageComp.width, imageComp.height);
				ImGui::SameLine();
				ImGui::Text("Channels: %d", imageComp.channels);
				ImGui::SameLine();
				ImGui::Text("Entity ID: %zu", selectedEntityID);

				contextMenuUtils->RenderImageContextMenu(selectedEntityID);

				ImGui::Separator();
			}
			catch (const std::exception& e) {
				ImGui::Text("Error reading image info: %s", e.what());
			}
		}
	}

	void ImageView::RenderControls() {
		ImGui::PushItemWidth(100.0f);
		if (ImGui::InputFloat("Zoom", &zoom, 0.1f, 0.5f, "%.1f")) {
			SetZoom(zoom);
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (contextMenuUtils->HasClipboardEntity()) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s", contextMenuUtils->GetClipboardPreview().c_str());
		}
	}

	void ImageView::RenderSelector() {
		if (imageEntities.empty()) {
			ImGui::Text("No images loaded.");
			return;
		}

		ImGui::PushItemWidth(100.0f);
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
		ImGui::PopItemWidth();

		ImGui::SameLine();
		ImGui::Text("Image %d of %zu", imgIndex + 1, imageEntities.size());
	}

	void ImageView::RenderHistory() {
		ImGui::Begin("Image History", &showHistory);

		float currentRowWidth = 0.0f;

		for (size_t i = 0; i < imageEntities.size(); ++i) {
			ECS::EntityID entityID = imageEntities[i];

			if (!m_entityManager.IsEntityValid(entityID) || !m_entityManager.HasComponent<ECS::ImageComponent>(entityID)) {
				continue;
			}

			try {
				const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);

				ImGui::BeginGroup();

				if (static_cast<int>(i) == imgIndex) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
				}

				float aspectRatio = (imageComp.width > 0 && imageComp.height > 0)
					? static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height) : 1.0f;
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
		if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
			!m_entityManager.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
			ImGui::Text("No image selected or entity invalid.");
			return;
		}

		try {
			const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);

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
		if (!imageSystem) {
			std::cerr << "[ImageView] ImageSystem not available!" << std::endl;
			return;
		}

		try {
			for (const auto& filePath : filePaths) {
				if (filePath.empty()) continue;

				ECS::EntityID entity = m_entityManager.AddNewEntity();
				auto& imageComp = m_entityManager.AddComponent<ECS::ImageComponent>(entity);

				imageComp.filePath = filePath;
				imageComp.fileName = std::filesystem::path(filePath).filename().string();

				imageSystem->SetImage(entity, filePath);

				std::cout << "[ImageView] Started loading: " << filePath << " (Entity: " << entity << ")" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception loading images: " << e.what() << std::endl;
		}
	}

	void ImageView::SaveSelectedImage() {
		if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;

		try {
			const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);

			if (imageComp.imageData && imageComp.width > 0 && imageComp.height > 0) {
				Utils::ImageUtils::SaveImage(imageComp.filePath, imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);
				std::cout << "[ImageView] Saved image: " << imageComp.filePath << std::endl;
			}
			else {
				std::cerr << "[ImageView] No image data available to save" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception saving image: " << e.what() << std::endl;
		}
	}

	void ImageView::SaveSelectedImageAs(const std::string& filePath) {
		if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;

		try {
			const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);

			if (imageComp.imageData && imageComp.width > 0 && imageComp.height > 0) {
				Utils::ImageUtils::SaveImage(filePath, imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);
				std::cout << "[ImageView] Saved image as: " << filePath << std::endl;
			}
			else {
				std::cerr << "[ImageView] No image data available to save" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ImageView] Exception saving image: " << e.what() << std::endl;
		}
	}

	void ImageView::RemoveSelectedImage() {
		if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;

		try {
			if (imageSystem) {
				imageSystem->RemoveImage(selectedEntityID);
			}
			else {
				m_entityManager.DestroyEntity(selectedEntityID);
			}

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

	bool ImageView::IsImageComponentOnly(ECS::EntityID entityId) const {
		if (!m_entityManager.IsEntityValid(entityId)) return false;

		return m_entityManager.HasComponent<ECS::ImageComponent>(entityId) &&
			!m_entityManager.HasComponent<ECS::InputImageComponent>(entityId) &&
			!m_entityManager.HasComponent<ECS::OutputImageComponent>(entityId);
	}

} // namespace GUI