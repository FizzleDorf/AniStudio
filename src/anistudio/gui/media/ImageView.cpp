#include "ImageView.hpp"
#include "ImageUtils.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "TextureSystem.hpp"
#include "FilePathSystem.hpp"
#include "PngMetadataUtils.hpp"
#include "ClipboardUtilities.hpp"
#include "DragDropUtils.hpp"
#include <algorithm>

namespace GUI {

    ImageView::ImageView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm), imageSystem(nullptr), autoSwitchOnLoad(true) {
        viewName = "ImageView";
    }

    void ImageView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entityID) {
                OnMediaAdded(entityID);
                });
            imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entityID) {
                OnMediaRemoved(entityID);
                });
        }
        RefreshEntities();
    }

    void ImageView::Update(float deltaT) {
        size_t currentCount = 0;
        for (auto entityID : m_entityManager.GetAllEntities()) {
            if (IsImageComponentOnly(entityID)) currentCount++;
        }
        if (currentCount != lastEntityCount) {
            RefreshEntities();
            lastEntityCount = currentCount;
            if (mediaEntities.empty()) {
                selectedEntityID = 0;
                index = 0;
            }
            else if (selectedEntityID == 0) {
                selectedEntityID = mediaEntities[0];
                index = 0;
            }
            else {
                auto it = std::find(mediaEntities.begin(), mediaEntities.end(), selectedEntityID);
                if (it != mediaEntities.end()) {
                    index = static_cast<int>(std::distance(mediaEntities.begin(), it));
                }
                else {
                    if (!mediaEntities.empty()) {
                        selectedEntityID = mediaEntities[0];
                        index = 0;
                    }
                    else {
                        selectedEntityID = 0;
                        index = 0;
                    }
                }
            }
        }
    }

    void ImageView::Render() {
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            RenderMenuBar();
            RenderImageInfo();
            RenderControls();
            RenderSelector();
            ImGui::Separator();
            if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                RenderSelected();
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
                    std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
                    std::vector<std::string> outPaths;
                    if (FileDialog::OpenFiles("Choose Image(s)", FileDialog::FilterType::IMAGE_FILE, outPaths, defaultPath)) {
                        if (!outPaths.empty()) LoadMedia(outPaths);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save Image", nullptr, false, selectedEntityID != 0)) {
                    SaveSelectedMedia();
                }
                if (ImGui::MenuItem("Save Image As...", nullptr, false, selectedEntityID != 0)) {
                    auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                    std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
                    std::string outPath;
                    if (selectedEntityID != 0) {
                        const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
                        std::string defaultName = imageComp.fileName;
                        if (FileDialog::SaveFile("Save Image As", FileDialog::FilterType::IMAGE_FILE, defaultName, outPath, defaultPath)) {
                            SaveSelectedMediaAs(outPath);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Remove Image", nullptr, false, selectedEntityID != 0)) {
                    RemoveSelectedMedia();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Refresh")) {
                    RefreshEntities();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Show History", nullptr, &historyViewVisible)) {
                    ToggleHistoryView(historyViewVisible);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("First Image", nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) { index = 0; selectedEntityID = mediaEntities[index]; }
                }
                if (ImGui::MenuItem("Last Image", nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) { index = static_cast<int>(mediaEntities.size()) - 1; selectedEntityID = mediaEntities[index]; }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Auto-switch on Load", nullptr, &autoSwitchOnLoad)) {}
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
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
                RenderMediaContextMenu(selectedEntityID);
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
        if (GUI::Clipboard::HasEntity() || GUI::Clipboard::HasComponent() || GUI::Clipboard::HasProperty()) {
            ImGui::SameLine();
            std::string label;
            if (GUI::Clipboard::HasEntity()) label = "Entity";
            else if (GUI::Clipboard::HasComponent()) label = "Component";
            else if (GUI::Clipboard::HasProperty()) label = "Property";
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s", label.c_str());
        }
    }

    void ImageView::RenderSelector() {
        if (mediaEntities.empty()) {
            ImGui::Text("No images loaded.");
            return;
        }
        ImGui::PushItemWidth(100.0f);
        if (ImGui::InputInt("Current Image", &index)) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                if (size == 1) index = 0;
                else index = ((index % size) + size) % size;
                selectedEntityID = mediaEntities[index];
            }
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("Image %d of %zu", index + 1, mediaEntities.size());
    }

    void ImageView::RenderSelected() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
            ImGui::Text("No image selected or entity invalid.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
            return;
        }
        try {
            const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
            if (imageComp.textureID == 0 || imageComp.width <= 0 || imageComp.height <= 0) {
                ImGui::Text("Image loading... (Texture ID: %u, Size: %dx%d)", imageComp.textureID, imageComp.width, imageComp.height);
                HandleFileDropTarget();
                HandleEntityDropTarget();
                return;
            }
            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
                SetZoom(zoom + ImGui::GetIO().MouseWheel * 0.1f);
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

            ImGui::Image((ImTextureID)(intptr_t)imageComp.textureID, imageSize);

            // Internal drag-drop (within the app)
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !IsAltKeyDown()) {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    nlohmann::json payload;
                    payload["entityID"] = selectedEntityID;
                    ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                        payload.dump().c_str(), payload.dump().size() + 1);
                    ImGui::Image((ImTextureID)(intptr_t)imageComp.textureID, ImVec2(64, 64));
                    ImGui::Text("%s", imageComp.fileName.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            RenderMediaContextMenu(selectedEntityID);

            // Pan with Alt+LMB drag
            if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && IsAltKeyDown()) {
                offsetX += ImGui::GetIO().MouseDelta.x;
                offsetY += ImGui::GetIO().MouseDelta.y;
            }

            ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
            ImGui::Dummy(ImVec2(0, 0));

            HandleFileDropTarget();
            HandleEntityDropTarget();

        }
        catch (const std::exception& e) {
            ImGui::Text("Error rendering image: %s", e.what());
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
    }

    void ImageView::LoadMedia(const std::vector<std::string>& filePaths) {
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

    void ImageView::SaveSelectedMedia() {
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

    void ImageView::SaveSelectedMediaAs(const std::string& filePath) {
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

    void ImageView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        try {
            if (imageSystem) imageSystem->RemoveImage(selectedEntityID);
            else m_entityManager.DestroyEntity(selectedEntityID);
            OnMediaRemoved(selectedEntityID);
        }
        catch (const std::exception& e) {
            std::cerr << "[ImageView] Exception removing image: " << e.what() << std::endl;
        }
    }

    void ImageView::RefreshEntities() {
        mediaEntities.clear();
        for (auto entityID : m_entityManager.GetAllEntities()) {
            if (IsImageComponentOnly(entityID)) {
                mediaEntities.push_back(entityID);
            }
        }
        lastEntityCount = mediaEntities.size();
    }

    void ImageView::OnMediaAdded(ECS::EntityID entity) {
        RefreshEntities();
        if (!mediaEntities.empty() && autoSwitchOnLoad) {
            auto it = std::find(mediaEntities.begin(), mediaEntities.end(), entity);
            if (it != mediaEntities.end()) {
                index = static_cast<int>(std::distance(mediaEntities.begin(), it));
                selectedEntityID = entity;
            }
        }
    }

    void ImageView::OnMediaRemoved(ECS::EntityID entity) {
        int previousIndex = index;
        RefreshEntities();
        UpdateSelectionAfterRemoval(entity);
    }

    void ImageView::SetSelectedEntity(ECS::EntityID entity) {
        BaseMediaView::SetSelectedEntity(entity);
    }

    bool ImageView::IsImageComponentOnly(ECS::EntityID entityId) const {
        if (!m_entityManager.IsEntityValid(entityId)) return false;
        return m_entityManager.HasComponent<ECS::ImageComponent>(entityId) &&
            !m_entityManager.HasComponent<ECS::InputImageComponent>(entityId) &&
            !m_entityManager.HasComponent<ECS::OutputImageComponent>(entityId);
    }

    std::string ImageView::GetHistoryViewTypeName() const {
        return "ImageHistoryView";
    }

} // namespace GUI