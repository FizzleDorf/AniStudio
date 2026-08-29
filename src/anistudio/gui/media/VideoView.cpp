#include "VideoView.hpp"
#include "TextureSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "MediaHistoryView.hpp"
#include "MetadataView.hpp"
#include "AudioPlaybackSystem.hpp"
#include "AudioSystem.hpp"
#include "FilePathSystem.hpp"
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

        auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
        if (!audioPlaybackSystem) {
            m_entityManager.RegisterSystem<ECS::AudioPlaybackSystem>();
            audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
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

            videoSystem->RegisterVideoAudioCallback([this](ECS::EntityID videoEntity, ECS::EntityID audioEntity) {
                std::cout << "[VideoView] Audio track linked to video entity " << videoEntity
                    << " with audio entity " << audioEntity << std::endl;
                });
        }

        RefreshEntities();

        if (!mediaEntities.empty() && selectedEntityID == 0) {
            index = static_cast<int>(mediaEntities.size()) - 1;
            selectedEntityID = mediaEntities[index];
        }

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

                if (videoComp.isPlaying) {
                    ECS::EntityID audioEntity = GetAudioEntityForVideo(selectedEntityID);
                    if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity) &&
                        m_entityManager.HasComponent<ECS::AudioComponent>(audioEntity)) {
                        auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                        if (audioPlaybackSystem && !audioPlaybackSystem->IsPlaying(audioEntity)) {
                            ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(selectedEntityID);
                            if (videoAudioComp && videoAudioComp->audioEnabled) {
                                audioPlaybackSystem->Play(audioEntity, false);
                                audioPlaybackSystem->SetPlaybackSpeed(audioEntity, videoComp.playbackSpeed);
                            }
                        }
                    }
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
        if (!ImGui::Begin(windowName.c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            ImGui::End();
            return;
        }
        try {
            RenderMenuBar();
            RenderVideoInfo();
            RenderControls();
            RenderSelector();
            RenderPlaybackControls();

            if (HasAudioTrack(selectedEntityID)) {
                ImGui::SameLine();
                RenderAudioControls(selectedEntityID);
            }

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

    void VideoView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Load Video(s)")) {
                    static std::string lastVideoFolder;
                    std::vector<std::string> filePaths;
                    if (FileDialog::OpenFiles("Choose Video(s)", FileDialog::FilterType::VIDEO_FILE, filePaths, lastVideoFolder)) {
                        if (!filePaths.empty()) {
                            LoadMedia(filePaths);
                            lastVideoFolder = std::filesystem::path(filePaths[0]).parent_path().string();
                        }
                    }
                }
                ImGui::Separator();

                if (ImGui::BeginMenu("Save", selectedEntityID != 0)) {
                    if (ImGui::MenuItem("Save Video (with Audio)", nullptr, false,
                        selectedEntityID != 0 && HasAudioTrack(selectedEntityID))) {
                        SaveSelectedMedia();
                    }

                    if (ImGui::MenuItem("Save Video (No Audio)", nullptr, false, selectedEntityID != 0)) {
                        SaveSelectedMediaNoAudio();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Save Video As (with Audio)", nullptr, false,
                        selectedEntityID != 0 && HasAudioTrack(selectedEntityID))) {
                        SaveSelectedMediaAsWithAudio();
                    }

                    if (ImGui::MenuItem("Save Video As (No Audio)", nullptr, false, selectedEntityID != 0)) {
                        SaveSelectedMediaAsNoAudio();
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Remove Video", nullptr, false, selectedEntityID != 0)) {
                    RemoveSelectedMedia();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Refresh")) {
                    RefreshEntities();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                bool visible = IsHistoryVisible();
                if (ImGui::MenuItem("Show History", nullptr, &visible)) {
                    ToggleHistoryView(visible);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("First Video", nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = 0;
                        selectedEntityID = mediaEntities[index];
                        PauseAllVideos();
                        SeekAudioToFrame(selectedEntityID);
                    }
                }
                if (ImGui::MenuItem("Last Video", nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = static_cast<int>(mediaEntities.size()) - 1;
                        selectedEntityID = mediaEntities[index];
                        PauseAllVideos();
                        SeekAudioToFrame(selectedEntityID);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
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

                if (HasAudioTrack(selectedEntityID)) {
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Audio Track: Yes");
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Audio Track: No");
                }

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
        ImGui::PushItemWidth(100.0f);
        if (ImGui::InputFloat("Zoom", &zoom, 0.1f, 0.5f, "%.1f")) {
            SetZoom(zoom);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshEntities();
        }
        ImGui::SameLine();
        if (selectedEntityID != 0 && ImGui::Button("Remove Video")) {
            RemoveSelectedMedia();
        }
    }

    void VideoView::RenderSelector() {
        if (mediaEntities.empty()) {
            ImGui::Text("No videos loaded.");
            return;
        }
        ImGui::PushItemWidth(100.0f);
        if (ImGui::InputInt("Current Video", &index)) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                if (size == 1) index = 0;
                else index = ((index % size) + size) % size;
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
                SeekAudioToFrame(selectedEntityID);
            }
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("Video %d of %zu", index + 1, mediaEntities.size());
        ImGui::SameLine();
        if (ImGui::Button("First")) {
            if (!mediaEntities.empty()) {
                index = 0;
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
                SeekAudioToFrame(selectedEntityID);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Prev")) {
            if (!mediaEntities.empty()) {
                index = (index - 1 + static_cast<int>(mediaEntities.size())) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
                SeekAudioToFrame(selectedEntityID);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Next")) {
            if (!mediaEntities.empty()) {
                index = (index + 1) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
                SeekAudioToFrame(selectedEntityID);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Last")) {
            if (!mediaEntities.empty()) {
                index = static_cast<int>(mediaEntities.size() - 1);
                selectedEntityID = mediaEntities[index];
                PauseAllVideos();
                SeekAudioToFrame(selectedEntityID);
            }
        }
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
            int currentFrame = static_cast<int>(videoComp.currentFrame);
            if (ImGui::SliderInt("Frame", &currentFrame, 0, static_cast<int>(videoComp.frameCount - 1))) {
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                if (videoSystem) {
                    bool wasPlaying = videoComp.isPlaying;
                    videoComp.isPlaying = false;
                    videoSystem->SeekToFrame(videoComp, currentFrame);
                    SeekAudioToFrame(selectedEntityID);
                    if (wasPlaying) {
                        videoComp.isPlaying = true;
                        videoComp.frameAccumulator = 0.0f;
                        ECS::EntityID audioEntity = GetAudioEntityForVideo(selectedEntityID);
                        if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity) &&
                            m_entityManager.HasComponent<ECS::AudioComponent>(audioEntity)) {
                            auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                            if (audioPlaybackSystem) {
                                ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(selectedEntityID);
                                if (videoAudioComp && videoAudioComp->audioEnabled) {
                                    audioPlaybackSystem->Play(audioEntity, false);
                                    audioPlaybackSystem->SetPlaybackSpeed(audioEntity, videoComp.playbackSpeed);
                                }
                            }
                        }
                    }
                }
            }

            float speed = videoComp.playbackSpeed;
            if (ImGui::SliderFloat("Speed", &speed, 0.1f, 4.0f, "%.1fx")) {
                videoComp.playbackSpeed = speed;
                ECS::EntityID audioEntity = GetAudioEntityForVideo(selectedEntityID);
                if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity)) {
                    auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                    if (audioPlaybackSystem) {
                        audioPlaybackSystem->SetPlaybackSpeed(audioEntity, speed);
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button(videoComp.isPlaying ? "Pause" : "Play")) {
                videoComp.isPlaying = !videoComp.isPlaying;
                if (videoComp.isPlaying) videoComp.frameAccumulator = 0.0f;

                ECS::EntityID audioEntity = GetAudioEntityForVideo(selectedEntityID);
                if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity) &&
                    m_entityManager.HasComponent<ECS::AudioComponent>(audioEntity)) {
                    auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                    if (audioPlaybackSystem) {
                        if (videoComp.isPlaying) {
                            ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(selectedEntityID);
                            if (videoAudioComp && videoAudioComp->audioEnabled) {
                                SeekAudioToFrame(selectedEntityID);
                                audioPlaybackSystem->Play(audioEntity, false);
                                audioPlaybackSystem->SetPlaybackSpeed(audioEntity, videoComp.playbackSpeed);
                            }
                        }
                        else {
                            audioPlaybackSystem->Pause(audioEntity);
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                videoComp.isPlaying = false;
                videoComp.frameAccumulator = 0.0f;
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                if (videoSystem) {
                    videoSystem->SeekToFrame(videoComp, 0);
                    SeekAudioToFrame(selectedEntityID);
                }

                ECS::EntityID audioEntity = GetAudioEntityForVideo(selectedEntityID);
                if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity) &&
                    m_entityManager.HasComponent<ECS::AudioComponent>(audioEntity)) {
                    auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                    if (audioPlaybackSystem) {
                        audioPlaybackSystem->Stop(audioEntity);
                    }
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Loop", &videoComp.looping);
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

    bool VideoView::HasAudioTrack(ECS::EntityID entity) const {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return false;
        if (m_entityManager.HasComponent<ECS::VideoAudioComponent>(entity)) {
            ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(entity);
            return videoAudioComp != nullptr && videoAudioComp->hasAudio;
        }
        return false;
    }

    ECS::EntityID VideoView::GetAudioEntityForVideo(ECS::EntityID videoEntity) const {
        if (videoEntity == 0 || !m_entityManager.IsEntityValid(videoEntity)) return 0;
        if (m_entityManager.HasComponent<ECS::VideoAudioComponent>(videoEntity)) {
            ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(videoEntity);
            if (videoAudioComp && videoAudioComp->hasAudio) {
                return videoAudioComp->audioEntityID;
            }
        }
        return 0;
    }

    void VideoView::SeekAudioToFrame(ECS::EntityID videoEntity) {
        if (videoEntity == 0 || !m_entityManager.IsEntityValid(videoEntity)) return;
        if (!m_entityManager.HasComponent<ECS::VideoComponent>(videoEntity)) return;
        if (!HasAudioTrack(videoEntity)) return;

        ECS::EntityID audioEntity = GetAudioEntityForVideo(videoEntity);
        if (audioEntity == 0 || !m_entityManager.IsEntityValid(audioEntity) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(audioEntity)) return;

        auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(videoEntity);
        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(audioEntity);
        auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
        if (!audioPlaybackSystem) return;

        bool wasPlaying = audioPlaybackSystem->IsPlaying(audioEntity);
        bool wasPaused = audioPlaybackSystem->IsPaused(audioEntity);

        audioPlaybackSystem->Stop(audioEntity);

        double currentTime = static_cast<double>(videoComp.currentFrame) / videoComp.fps;

        if (currentTime >= audioComp.duration) {
            currentTime = std::max(0.0, audioComp.duration - 0.01);
        }
        if (currentTime < 0) currentTime = 0;

        audioPlaybackSystem->Seek(audioEntity, currentTime);

        audioComp.currentTime = currentTime;
        audioComp.currentSampleIndex = static_cast<size_t>(currentTime * audioComp.sampleRate) * audioComp.channels;

        if (wasPlaying && !wasPaused) {
            ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(videoEntity);
            if (videoAudioComp && videoAudioComp->audioEnabled) {
                audioPlaybackSystem->Play(audioEntity, false);
                audioPlaybackSystem->SetPlaybackSpeed(audioEntity, videoComp.playbackSpeed);
            }
        }
        else if (wasPaused) {
            audioPlaybackSystem->Pause(audioEntity);
        }
    }

    void VideoView::RenderAudioControls(ECS::EntityID videoEntity) {
        if (videoEntity == 0 || !m_entityManager.IsEntityValid(videoEntity)) return;

        if (!m_entityManager.HasComponent<ECS::VideoAudioComponent>(videoEntity)) return;

        ECS::VideoAudioComponent* videoAudioComp = &m_entityManager.GetComponent<ECS::VideoAudioComponent>(videoEntity);
        if (!videoAudioComp || !videoAudioComp->hasAudio) return;

        ECS::EntityID audioEntity = videoAudioComp->audioEntityID;
        if (audioEntity == 0 || !m_entityManager.IsEntityValid(audioEntity) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(audioEntity)) return;

        auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
        if (!audioPlaybackSystem) return;

        auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(videoEntity);

        ImGui::SameLine();
        ImGui::Text("Audio:");
        ImGui::SameLine();

        bool audioEnabled = videoAudioComp->audioEnabled;
        if (ImGui::Checkbox("##AudioEnabled", &audioEnabled)) {
            videoAudioComp->audioEnabled = audioEnabled;
            if (!audioEnabled) {
                audioPlaybackSystem->Pause(audioEntity);
            }
            else if (audioPlaybackSystem->IsPaused(audioEntity)) {
                audioPlaybackSystem->Resume(audioEntity);
                audioPlaybackSystem->SetPlaybackSpeed(audioEntity, videoComp.playbackSpeed);
                SeekAudioToFrame(videoEntity);
            }
        }

        ImGui::SameLine();
        float volumePercent = videoAudioComp->volume * 100.0f;
        if (ImGui::SliderFloat("##AudioVolume", &volumePercent, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
            videoAudioComp->volume = volumePercent / 100.0f;
            audioPlaybackSystem->SetVolume(audioEntity, videoAudioComp->volume);
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
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            videoSystem->SaveVideoAsync(selectedEntityID);
        }
    }

    void VideoView::SaveSelectedMediaAs(const std::string& filePath) {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        if (filePath.empty()) return;
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            videoSystem->SaveVideoAsync(selectedEntityID, filePath);
        }
    }

    void VideoView::SaveVideoWithAudio(ECS::EntityID entity, const std::string& filePath) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return;
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            videoSystem->SaveVideoAsync(entity, filePath);
        }
    }

    void VideoView::SaveVideoNoAudio(ECS::EntityID entity, const std::string& filePath) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return;
        if (!m_entityManager.HasComponent<ECS::VideoComponent>(entity)) return;

        auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(entity);
        if (videoComp.filePath.empty()) return;

        std::string outputPath = filePath.empty() ? videoComp.filePath : filePath;

        std::vector<Utils::VideoFrame> frames;

        long long originalFrame = videoComp.currentFrame;

        avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, 0, 0, 0);
        avcodec_flush_buffers(videoComp.codecCtx);
        videoComp.currentFrame = 0;
        videoComp.frameAccumulator = 0.0f;

        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) return;

        while (true) {
            if (!videoSystem->AdvanceOneFrame(videoComp)) {
                break;
            }

            Utils::VideoFrame frame;
            frame.width = videoComp.width;
            frame.height = videoComp.height;
            frame.channels = 4;

            unsigned char* data = (unsigned char*)malloc(videoComp.frameDataRGBA.size());
            if (!data) {
                videoSystem->SeekToFrame(videoComp, originalFrame);
                return;
            }
            std::memcpy(data, videoComp.frameDataRGBA.data(), videoComp.frameDataRGBA.size());
            frame.data = data;

            frames.push_back(frame);
        }

        videoSystem->SeekToFrame(videoComp, originalFrame);

        if (frames.empty()) {
            std::cerr << "[VideoView] No frames to save" << std::endl;
            return;
        }

        nlohmann::json metadata;
        metadata["fps"] = videoComp.fps;
        metadata["width"] = videoComp.width;
        metadata["height"] = videoComp.height;
        metadata["frameCount"] = frames.size();
        metadata["originalFile"] = videoComp.filePath;
        metadata["hasAudio"] = false;

        bool result = Utils::VideoUtils::EncodeFramesToVideo(
            frames,
            outputPath,
            static_cast<int>(videoComp.fps),
            metadata,
            nullptr
        );

        for (auto& frame : frames) {
            if (frame.data) {
                free((void*)frame.data);
            }
        }

        if (result) {
            std::cout << "[VideoView] Saved video without audio to: " << outputPath << std::endl;
            if (filePath.empty()) {
                videoComp.filePath = outputPath;
                videoComp.fileName = std::filesystem::path(outputPath).filename().string();
            }
        }
        else {
            std::cerr << "[VideoView] Failed to save video without audio" << std::endl;
        }
    }

    void VideoView::SaveSelectedMediaNoAudio() {
        SaveVideoNoAudio(selectedEntityID, "");
    }

    void VideoView::SaveSelectedMediaAsWithAudio() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;

        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
        std::string outPath;

        const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
        std::string defaultName = videoComp.fileName;

        if (FileDialog::SaveFile("Save Video As (with Audio)", FileDialog::FilterType::VIDEO_FILE, defaultName, outPath, defaultPath)) {
            SaveVideoWithAudio(selectedEntityID, outPath);
        }
    }

    void VideoView::SaveSelectedMediaAsNoAudio() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;

        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
        std::string outPath;

        const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
        std::string defaultName = videoComp.fileName;

        if (FileDialog::SaveFile("Save Video As (No Audio)", FileDialog::FilterType::VIDEO_FILE, defaultName, outPath, defaultPath)) {
            SaveVideoNoAudio(selectedEntityID, outPath);
        }
    }

    void VideoView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;
        try {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            videoComp.isPlaying = false;
            videoComp.frameAccumulator = 0.0f;

            ECS::EntityID audioEntity = GetAudioEntityForVideo(selectedEntityID);
            if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity)) {
                auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                if (audioPlaybackSystem) {
                    audioPlaybackSystem->Stop(audioEntity);
                }
                auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
                if (audioSystem) {
                    audioSystem->RemoveAudio(audioEntity);
                }
            }

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

                    ECS::EntityID audioEntity = GetAudioEntityForVideo(entityID);
                    if (audioEntity != 0 && m_entityManager.IsEntityValid(audioEntity)) {
                        auto audioPlaybackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                        if (audioPlaybackSystem) {
                            audioPlaybackSystem->Pause(audioEntity);
                        }
                    }
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