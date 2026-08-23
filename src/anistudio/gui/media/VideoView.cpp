#include "VideoView.hpp"
#include "TextureSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "MediaHistoryView.hpp"
#include "MetadataView.hpp"
#include <algorithm>
#include <iostream>

namespace GUI {

    VideoView::VideoView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm), isPlaying(false), playbackSpeed(1.0f), lastGeneratedVideoID(0) {
        viewName = "VideoView";
    }

    void VideoView::Init() {
        std::cout << "[VideoView] Initializing..." << std::endl;
        if (!ImGui::GetCurrentContext()) {
            std::cerr << "[VideoView] No ImGui context in Init()!" << std::endl;
            return;
        }
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            m_entityManager.RegisterSystem<ECS::VideoSystem>();
            videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        }
        if (videoSystem) {
            auto textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
            if (textureSystem) {
                videoSystem->SetVideoTextureCallback(
                    [textureSystem](ECS::EntityID entityID, unsigned char* data,
                        int width, int height, int channels, GLuint* targetTexture) {
                            textureSystem->QueueVideoTextureCreation(entityID, data, width, height, channels, targetTexture);
                    }
                );
            }
            else {
                std::cerr << "[VideoView] WARNING: TextureSystem not found; video textures will not update!" << std::endl;
            }
            videoSystem->RegisterVideoAddedCallback([this](ECS::EntityID entity) {
                OnMediaAdded(entity);
                if (!mediaEntities.empty()) {
                    auto it = std::find(mediaEntities.begin(), mediaEntities.end(), entity);
                    if (it != mediaEntities.end()) {
                        index = static_cast<int>(std::distance(mediaEntities.begin(), it));
                        selectedEntityID = entity;
                    }
                }
                });
            videoSystem->RegisterVideoRemovedCallback([this](ECS::EntityID entity) {
                OnMediaRemoved(entity);
                });
        }
        RefreshEntities();
        std::cout << "[VideoView] Initialization complete" << std::endl;

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
                                m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
                                SetSelectedEntity(entity);
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[VideoView] SelectMediaEntity event error: " << e.what() << std::endl;
            }
            });
    }

    void VideoView::Update(float deltaT) {
        size_t currentCount = 0;
        for (auto entityID : m_entityManager.GetAllEntities()) {
            if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) currentCount++;
        }
        if (currentCount != lastEntityCount) {
            RefreshEntities();
            if (selectedEntityID != 0 && !m_entityManager.IsEntityValid(selectedEntityID)) {
                selectedEntityID = 0;
                index = 0;
            }
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

        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            if (videoComp.isPlaying && videoComp.fmtCtx) {
                videoComp.frameAccumulator += deltaT * videoComp.playbackSpeed;
                float frameDuration = 1.0f / static_cast<float>(videoComp.fps);
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                while (videoComp.frameAccumulator >= frameDuration) {
                    videoComp.frameAccumulator -= frameDuration;
                    if (videoSystem) {
                        if (!m_entityManager.IsEntityValid(selectedEntityID) || !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
                            videoComp.isPlaying = false;
                            break;
                        }
                        if (!videoSystem->AdvanceOneFrame(videoComp)) {
                            videoComp.isPlaying = false;
                            break;
                        }
                    }
                    else break;
                }
            }
        }
    }

    void VideoView::Render() {
        if (!ImGui::GetCurrentContext()) {
            std::cerr << "[VideoView] ERROR: No ImGui context!" << std::endl;
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);
        std::string windowName = "Video Viewer##" + std::to_string(GetID());
        if (!ImGui::Begin(windowName.c_str(), &windowOpen)) {
            ImGui::End();
            return;
        }
        try {
            RenderVideoInfo();
            RenderSelector();
            RenderControls();
            RenderPlaybackControls();
            ImGui::SameLine();
            bool visible = IsHistoryVisible();
            if (ImGui::Checkbox("Show History", &visible)) {
                ToggleHistoryView(visible);
            }
            ImGui::Separator();
            if (ImGui::BeginChild("VideoViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                RenderSelected();
            }
            ImGui::EndChild();
            HandleClipboardPaste();
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception in Render: " << e.what() << std::endl;
            ImGui::Text("Error rendering VideoView: %s", e.what());
        }
        ImGui::End();
        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void VideoView::RefreshEntities() {
        try {
            mediaEntities.clear();
            for (auto entityID : m_entityManager.GetAllEntities()) {
                if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                    mediaEntities.push_back(entityID);
                }
            }
            lastEntityCount = mediaEntities.size();
            std::cout << "[VideoView] Refreshed entities, found " << mediaEntities.size() << " videos" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception refreshing entities: " << e.what() << std::endl;
            mediaEntities.clear();
            lastEntityCount = 0;
        }
    }

    void VideoView::RenderVideoInfo() {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            try {
                const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
                ImGui::Text("File: %s", videoComp.fileName.c_str());
                ImGui::Text("Dimensions: %dx%d, FPS: %.2f, Frames: %d",
                    videoComp.width, videoComp.height, videoComp.fps, videoComp.frameCount);
                ImGui::Text("Current Frame: %d / %d", videoComp.currentFrame, videoComp.frameCount);
                ImGui::Text("Entity ID: %zu", selectedEntityID);
                if (selectedEntityID == lastGeneratedVideoID) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "✦ NEWLY GENERATED");
                }
                RenderMediaContextMenu(selectedEntityID);
                if (ImGui::Button("Send to Metadata Viewer")) {
                    SendSelectedToMetadataView();
                }
                ImGui::Separator();
            }
            catch (const std::exception& e) {
                ImGui::Text("Error reading video info: %s", e.what());
            }
        }
    }

    void VideoView::RenderControls() {
        static std::string lastVideoFolder;
        if (ImGui::Button("Load Video(s)")) {
            std::vector<std::string> filePaths;
            if (FileDialog::OpenFiles("Choose Video(s)", FileDialog::FilterType::VIDEO_FILE, filePaths, lastVideoFolder)) {
                if (!filePaths.empty()) {
                    LoadMedia(filePaths);
                    lastVideoFolder = std::filesystem::path(filePaths[0]).parent_path().string();
                }
            }
        }
        ImGui::SameLine();
        if (selectedEntityID != 0 && ImGui::Button("Remove Video")) {
            RemoveSelectedMedia();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshEntities();
        }
    }

    void VideoView::RenderSelector() {
        if (mediaEntities.empty()) {
            ImGui::Text("No videos loaded.");
            return;
        }
        if (ImGui::Button("First")) {
            if (!mediaEntities.empty()) { index = 0; selectedEntityID = mediaEntities[index]; PauseAllVideos(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Previous")) {
            if (!mediaEntities.empty()) {
                index = (index - 1 + static_cast<int>(mediaEntities.size())) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Next")) {
            if (!mediaEntities.empty()) {
                index = (index + 1) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Last")) {
            if (!mediaEntities.empty()) {
                index = static_cast<int>(mediaEntities.size() - 1);
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
            }
        }
        ImGui::SameLine();
        if (ImGui::InputInt("Current Video", &index)) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                if (size == 1) index = 0;
                else index = ((index % size) + size) % size;
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
            }
        }
        ImGui::Text("Video %d of %zu", index + 1, mediaEntities.size());
    }

    void VideoView::RenderPlaybackControls() {
        ImGui::Separator();
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            ImGui::Text("No video selected.");
            return;
        }
        try {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            int currentFrame = videoComp.currentFrame;
            if (ImGui::SliderInt("Frame", &currentFrame, 0, videoComp.frameCount - 1)) {
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                if (videoSystem) videoSystem->SeekToFrame(videoComp, currentFrame);
            }
            if (ImGui::Button(videoComp.isPlaying ? "Pause" : "Play")) {
                videoComp.isPlaying = !videoComp.isPlaying;
                if (videoComp.isPlaying) videoComp.frameAccumulator = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                videoComp.isPlaying = false;
                videoComp.frameAccumulator = 0.0f;
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                if (videoSystem) videoSystem->SeekToFrame(videoComp, 0);
            }
            ImGui::SameLine();
            ImGui::Checkbox("Loop", &videoComp.looping);
            ImGui::SameLine();
            ImGui::SliderFloat("Speed", &videoComp.playbackSpeed, 0.1f, 2.0f, "%.1fx");
            ImGui::Separator();
        }
        catch (const std::exception& e) {
            ImGui::Text("Error with playback controls: %s", e.what());
        }
    }

    void VideoView::RenderSelected() {
        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            ImGui::Text("No video selected or entity invalid.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
            return;
        }

        try {
            const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);

            GLuint texID = videoComp.currentTexture;
            if (texID == 0 || !glIsTexture(texID) || videoComp.width <= 0 || videoComp.height <= 0) {
                ImGui::Text("Video loading... (Texture ID: %u, Size: %dx%d)",
                    texID, videoComp.width, videoComp.height);
                HandleFileDropTarget();
                HandleEntityDropTarget();
                return;
            }

            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
                SetZoom(zoom + ImGui::GetIO().MouseWheel * 0.1f);
            }

            ImVec2 imageSize = ImVec2(videoComp.width * zoom, videoComp.height * zoom);
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
            if (zoom <= 1.0f) {
                offsetX = (windowSize.x - imageSize.x) * 0.5f;
                offsetY = (windowSize.y - imageSize.y) * 0.5f;
            }
            ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

            DrawGrid(videoComp.width, videoComp.height);
            ImGui::SetCursorPos(imagePos);
            ImGui::Dummy(imageSize);
            ImGui::SetCursorPos(imagePos);

            ImGui::Image((ImTextureID)(intptr_t)texID, imageSize, ImVec2(0, 0), ImVec2(1, 1));

            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !IsAltKeyDown()) {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    nlohmann::json payload;
                    payload["entityID"] = selectedEntityID;
                    ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                        payload.dump().c_str(), payload.dump().size() + 1);
                    ImGui::Image((ImTextureID)(intptr_t)texID, ImVec2(64, 64));
                    ImGui::Text("%s", videoComp.fileName.c_str());
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
            ImGui::Text("Error rendering video: %s", e.what());
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
    }

    void VideoView::LoadMedia(const std::vector<std::string>& filePaths) {
        std::cout << "[VideoView] Loading " << filePaths.size() << " videos..." << std::endl;
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            std::cerr << "[VideoView] Error: VideoSystem not found!" << std::endl;
            return;
        }
        try {
            for (const auto& filePath : filePaths) {
                if (filePath.empty()) continue;
                ECS::EntityID entity = m_entityManager.AddNewEntity();
                m_entityManager.AddComponent<ECS::VideoComponent>(entity);
                videoSystem->SetVideo(entity, filePath);
                std::cout << "[VideoView] Started loading: " << filePath << " (Entity: " << entity << ")" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception loading videos: " << e.what() << std::endl;
        }
    }

    void VideoView::LoadVideo(const std::string& filePath) {
        LoadMedia({ filePath });
    }

    void VideoView::SaveSelectedMedia() {
        std::cerr << "[VideoView] SaveSelectedMedia not implemented" << std::endl;
    }

    void VideoView::SaveSelectedMediaAs(const std::string& filePath) {
        std::cerr << "[VideoView] SaveSelectedMediaAs not implemented" << std::endl;
    }

    void VideoView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        try {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            videoComp.isPlaying = false;
            videoComp.frameAccumulator = 0.0f;
            auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
            if (videoSystem) videoSystem->RemoveVideo(selectedEntityID);
            selectedEntityID = 0;
            index = 0;
            RefreshEntities();
            std::cout << "[VideoView] Video removed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception removing video: " << e.what() << std::endl;
        }
    }

    void VideoView::PauseAllVideos() {
        try {
            for (auto entityID : mediaEntities) {
                if (m_entityManager.IsEntityValid(entityID) && m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                    auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
                    videoComp.isPlaying = false;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception pausing videos: " << e.what() << std::endl;
        }
    }

    void VideoView::OnMediaAdded(ECS::EntityID entity) {
        RefreshEntities();
    }

    void VideoView::OnMediaRemoved(ECS::EntityID entity) {
        int previousIndex = index;
        RefreshEntities();
        UpdateSelectionAfterRemoval(entity);
    }

    bool VideoView::IsHistoryVisible() const {
        return GetViewManager().HasView<MediaHistoryView>(GetID());
    }

    std::string VideoView::GetHistoryViewTypeName() const {
        return "MediaHistoryView";
    }

    std::string VideoView::GetSelectedFilePath() const {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            return m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID).filePath;
        }
        return "";
    }

}