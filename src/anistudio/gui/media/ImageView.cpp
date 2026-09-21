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
#include "MediaHistoryView.hpp"
#include "MetadataView.hpp"
#include "IconFonts.hpp"
#include "Log.hpp"
#include <algorithm>

namespace GUI {

    ImageView::ImageView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm), imageSystem(nullptr), autoSwitchOnLoad(true) {
        viewName = "ImageView";
    }

    ImageView::~ImageView() {
        if (imageSystem) {
            imageSystem->UnregisterCallbacksForOwner(this);
        }
    }

    void ImageView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback(this, [this](ECS::EntityID entityID) {
                OnMediaAdded(entityID);
                });
            imageSystem->RegisterImageRemovedCallback(this, [this](ECS::EntityID entityID) {
                OnMediaRemoved(entityID);
                });
        }
        RefreshEntities();

        if (!mediaEntities.empty() && selectedEntityID == 0) {
            index = static_cast<int>(mediaEntities.size()) - 1;
            selectedEntityID = mediaEntities[index];
        }

        ANI::Events::Ref().RegisterEventWithData("SelectMediaEntity", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                auto it = eventData.find("workspaceID");
                if (it != eventData.end()) {
                    WorkspaceID wsID = std::any_cast<WorkspaceID>(it->second);
                    if (wsID == GetID()) {
                        auto entityIt = eventData.find("entityID");
                        if (entityIt != eventData.end()) {
                            ECS::EntityID entity = std::any_cast<ECS::EntityID>(entityIt->second);
                            if (m_entityManager.IsEntityValid(entity) &&
                                m_entityManager.HasComponent<ECS::ImageComponent>(entity)) {
                                SetSelectedEntity(entity);
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[ImageView] SelectMediaEntity event error: %s", e.what());
            }
            });
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
            else if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
                index = static_cast<int>(mediaEntities.size()) - 1;
                selectedEntityID = mediaEntities[index];
            }
            else {
                auto it = std::find(mediaEntities.begin(), mediaEntities.end(), selectedEntityID);
                if (it != mediaEntities.end()) {
                    index = static_cast<int>(std::distance(mediaEntities.begin(), it));
                }
                else {
                    if (!mediaEntities.empty()) {
                        index = static_cast<int>(mediaEntities.size()) - 1;
                        selectedEntityID = mediaEntities[index];
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
            RenderToolbar();
            RenderMediaInfo();
            ImGui::Separator();
            if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, -60), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                RenderMediaContent();
            }
            ImGui::EndChild();
            RenderControls();
            RenderSelector();
            HandleClipboardPaste();
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
                ImGui::PushID(1);
                if (ImGui::MenuItem((Icon::Image() + " " + Icon::FolderOpen() + " Load Image(s)").c_str())) {
                    auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                    std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
                    std::vector<std::string> outPaths;
                    if (FileDialog::OpenFiles("Choose Image(s)", FileDialog::FilterType::IMAGE_FILE, outPaths, defaultPath)) {
                        if (!outPaths.empty()) LoadMedia(outPaths);
                    }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(2);
                if (ImGui::MenuItem((Icon::Save() + " Save Image").c_str(), nullptr, false, selectedEntityID != 0)) {
                    SaveSelectedMedia();
                }
                ImGui::PopID();
                ImGui::PushID(3);
                if (ImGui::MenuItem((Icon::SaveAs() + " Save Image As...").c_str(), nullptr, false, selectedEntityID != 0)) {
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
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(4);
                if (ImGui::MenuItem((Icon::Trash() + " Remove Image").c_str(), nullptr, false, selectedEntityID != 0)) {
                    RemoveSelectedMedia();
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(5);
                if (ImGui::MenuItem((Icon::Refresh() + " Refresh").c_str())) {
                    RefreshEntities();
                }
                ImGui::PopID();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                bool visible = IsHistoryVisible();
                if (ImGui::MenuItem("Show History", nullptr, &visible)) {
                    ToggleHistoryView(visible);
                }
                ImGui::Separator();
                ImGui::PushID(6);
                if (ImGui::MenuItem((Icon::ArrowFirst() + " First Image").c_str(), nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) { index = 0; selectedEntityID = mediaEntities[index]; }
                }
                ImGui::PopID();
                ImGui::PushID(7);
                if (ImGui::MenuItem((Icon::ArrowLast() + " Last Image").c_str(), nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) { index = static_cast<int>(mediaEntities.size()) - 1; selectedEntityID = mediaEntities[index]; }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(8);
                if (ImGui::MenuItem("Auto-switch on Load", nullptr, &autoSwitchOnLoad)) {}
                ImGui::PopID();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void ImageView::RenderToolbar() {
        ImGui::PushID(100);

        ImGui::PushID(101);
        if (ImGui::Button((Icon::Image() + " Load").c_str())) {
            auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
            std::vector<std::string> outPaths;
            if (FileDialog::OpenFiles("Choose Image(s)", FileDialog::FilterType::IMAGE_FILE, outPaths, defaultPath)) {
                if (!outPaths.empty()) LoadMedia(outPaths);
            }
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(102);
        if (ImGui::Button((Icon::Save() + " Save").c_str())) {
            SaveSelectedMedia();
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(103);
        if (ImGui::Button((Icon::SaveAs() + " Save As").c_str())) {
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
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(104);
        if (ImGui::Button((Icon::Trash() + " Remove").c_str())) {
            RemoveSelectedMedia();
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(105);
        if (ImGui::Button(Icon::Refresh().c_str())) {
            RefreshEntities();
        }
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID(106);
        if (ImGui::Button("Send to Metadata")) {
            SendSelectedToMetadataView();
        }
        ImGui::PopID();

        ImGui::PopID();
    }

    void ImageView::RenderMediaInfo() {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
            try {
                const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
                ImGui::Text("File: %s", imageComp.fileName.c_str());
                ImGui::SameLine();
                ImGui::Text("| Dimensions: %dx%d", imageComp.width, imageComp.height);
                ImGui::SameLine();
                ImGui::Text("| Channels: %d", imageComp.channels);
                ImGui::SameLine();
                ImGui::Text("| Entity ID: %zu", selectedEntityID);
                RenderMediaContextMenu(selectedEntityID);
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
        ImGui::PushID(200);
        if (ImGui::Button(Icon::Refresh().c_str())) {
            RefreshEntities();
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(201);
        if (selectedEntityID != 0 && ImGui::Button((Icon::Save() + " Save").c_str())) {
            SaveSelectedMedia();
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(202);
        if (selectedEntityID != 0 && ImGui::Button((Icon::Trash() + " Remove").c_str())) {
            RemoveSelectedMedia();
        }
        ImGui::PopID();
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
        ImGui::SameLine();
        ImGui::PushID(203);
        if (ImGui::Button(Icon::ArrowFirst().c_str())) {
            if (!mediaEntities.empty()) { index = 0; selectedEntityID = mediaEntities[index]; }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(204);
        if (ImGui::Button(Icon::ArrowLeft().c_str())) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                index = (index - 1 + size) % size;
                selectedEntityID = mediaEntities[index];
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(205);
        if (ImGui::Button(Icon::ArrowRight().c_str())) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                index = (index + 1) % size;
                selectedEntityID = mediaEntities[index];
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(206);
        if (ImGui::Button(Icon::ArrowLast().c_str())) {
            if (!mediaEntities.empty()) { index = static_cast<int>(mediaEntities.size()) - 1; selectedEntityID = mediaEntities[index]; }
        }
        ImGui::PopID();
    }

    void ImageView::RenderMediaContent() {
        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
            ImGui::Text("No image selected or entity invalid.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
            return;
        }

        try {
            const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);

            GLuint texID = 0;
            if (m_entityManager.HasComponent<ECS::TextureComponent>(selectedEntityID)) {
                texID = m_entityManager.GetComponent<ECS::TextureComponent>(selectedEntityID).textureID;
            }

            if (texID == 0 || !glIsTexture(texID) || imageComp.width <= 0 || imageComp.height <= 0) {
                ImGui::Text("Image loading... (Texture ID: %u, Size: %dx%d)", texID, imageComp.width, imageComp.height);
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

            ImGui::Image((ImTextureID)(intptr_t)texID, imageSize);

            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !IsAltKeyDown()) {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    nlohmann::json payload;
                    payload["entityID"] = selectedEntityID;
                    ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                        payload.dump().c_str(), payload.dump().size() + 1);
                    ImGui::Image((ImTextureID)(intptr_t)texID, ImVec2(64, 64));
                    ImGui::Text("%s", imageComp.fileName.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            RenderMediaContextMenu(selectedEntityID);

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
            ANI_LOG_ERROR("[ImageView] ImageSystem not available!");
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
                ANI_LOG_INFO("[ImageView] Started loading: %s (Entity: %llu)",
                    filePath.c_str(), static_cast<unsigned long long>(entity));
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ImageView] Exception loading images: %s", e.what());
        }
    }

    void ImageView::SaveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        try {
            const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
            if (imageComp.imageData && imageComp.width > 0 && imageComp.height > 0) {
                Utils::ImageUtils::SaveImage(imageComp.filePath, imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);
                ANI_LOG_INFO("[ImageView] Saved image: %s", imageComp.filePath.c_str());
            }
            else {
                ANI_LOG_WARN("[ImageView] No image data available to save");
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ImageView] Exception saving image: %s", e.what());
        }
    }

    void ImageView::SaveSelectedMediaAs(const std::string& filePath) {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        try {
            const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID);
            if (imageComp.imageData && imageComp.width > 0 && imageComp.height > 0) {
                Utils::ImageUtils::SaveImage(filePath, imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);
                ANI_LOG_INFO("[ImageView] Saved image as: %s", filePath.c_str());
            }
            else {
                ANI_LOG_WARN("[ImageView] No image data available to save");
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ImageView] Exception saving image: %s", e.what());
        }
    }

    void ImageView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        try {
            if (imageSystem) {
                imageSystem->RemoveImage(selectedEntityID);
            }

            if (m_entityManager.IsEntityValid(selectedEntityID)) {
                if (auto texSys = m_entityManager.GetSystem<ECS::TextureSystem>()) {
                    texSys->RemoveTexture(selectedEntityID);
                }
                m_entityManager.DestroyEntity(selectedEntityID);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ImageView] Exception removing image: %s", e.what());
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
            !m_entityManager.HasComponent<ECS::OutputImageComponent>(entityId) &&
            !m_entityManager.HasComponent<ECS::PreviewImageComponent>(entityId);
    }

    bool ImageView::IsHistoryVisible() const {
        return GetViewManager().HasView<MediaHistoryView>(GetID());
    }

    std::string ImageView::GetHistoryViewTypeName() const {
        return "MediaHistoryView";
    }

    std::string ImageView::GetSelectedFilePath() const {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
            return m_entityManager.GetComponent<ECS::ImageComponent>(selectedEntityID).filePath;
        }
        return "";
    }

}