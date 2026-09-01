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
#include "VideoPlaybackSystem.hpp"
#include "VideoAudioSystem.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <imgui.h>

namespace GUI {

    VideoView::VideoView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm), lastGeneratedVideoID(0) {
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

        auto playbackSystem = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (!playbackSystem) {
            m_entityManager.RegisterSystem<ECS::VideoPlaybackSystem>();
            playbackSystem = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        }

        auto vaSystem = m_entityManager.GetSystem<ECS::VideoAudioSystem>();
        if (!vaSystem) {
            m_entityManager.RegisterSystem<ECS::VideoAudioSystem>();
        }

        if (playbackSystem) {
            playbackSystem->RegisterVideoPlaybackCallback([this](ECS::EntityID entity, const unsigned char* data, int width, int height) {
                if (entity != selectedEntityID) return;
                if (!m_entityManager.IsEntityValid(entity) || !m_entityManager.HasComponent<ECS::VideoComponent>(entity)) return;

                (void)data;
                (void)width;
                (void)height;
                });
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
            videoSystem->RegisterVideoAddedCallback([this](ECS::EntityID entity) {
                OnMediaAdded(entity);
                if (!mediaEntities.empty()) {
                    auto it = std::find(mediaEntities.begin(), mediaEntities.end(), entity);
                    if (it != mediaEntities.end()) {
                        index = static_cast<int>(std::distance(mediaEntities.begin(), it));
                        selectedEntityID = entity;
                        UpdateWaveformData();
                    }
                }
                });
            videoSystem->RegisterVideoRemovedCallback([this](ECS::EntityID entity) {
                OnMediaRemoved(entity);
                });
        }

        RefreshEntities();

        if (!mediaEntities.empty() && selectedEntityID == 0) {
            index = static_cast<int>(mediaEntities.size()) - 1;
            selectedEntityID = mediaEntities[index];
            UpdateWaveformData();
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
                                m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
                                SetSelectedEntity(entity);
                                UpdateWaveformData();
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
            if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                currentCount++;
            }
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
                m_waveformData.clear();
            }
            else if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
                index = static_cast<int>(mediaEntities.size()) - 1;
                selectedEntityID = mediaEntities[index];
                UpdateWaveformData();
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
                        UpdateWaveformData();
                    }
                    else {
                        selectedEntityID = 0;
                        index = 0;
                        m_waveformData.clear();
                    }
                }
            }
        }

        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            if (videoComp.needsTextureUpdate) {
                auto textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
                if (textureSystem) {
                    std::shared_lock lock(videoComp.dataMutex);
                    if (!videoComp.frameDataRGBA.empty()) {
                        // Always queue a new texture – the texture system will delete the old one
                        unsigned char* copyData = (unsigned char*)malloc(videoComp.frameDataRGBA.size());
                        if (copyData) {
                            memcpy(copyData, videoComp.frameDataRGBA.data(), videoComp.frameDataRGBA.size());
                            textureSystem->QueueVideoTextureCreation(
                                selectedEntityID,
                                copyData,
                                videoComp.width,
                                videoComp.height,
                                4,
                                &videoComp.currentTexture
                            );
                        }
                        videoComp.needsTextureUpdate = false;
                    }
                }
            }

            auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
            if (playback) {
                double duration = GetVideoDuration(selectedEntityID);
                if (duration > 0.0) {
                    m_playbackProgress = static_cast<float>(
                        playback->GetCurrentPosition(selectedEntityID) / duration
                        );
                    if (m_playbackProgress < 0) m_playbackProgress = 0;
                    if (m_playbackProgress > 1) m_playbackProgress = 1;
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

            if (HasAudioTrack(selectedEntityID) && m_showWaveform) {
                ImGui::Separator();
                RenderWaveform();
            }

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
                        SeekVideo(selectedEntityID, 0.0);
                        UpdateWaveformData();
                    }
                }
                if (ImGui::MenuItem("Last Video", nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = static_cast<int>(mediaEntities.size()) - 1;
                        selectedEntityID = mediaEntities[index];
                        SeekVideo(selectedEntityID, GetVideoDuration(selectedEntityID) - 0.001);
                        UpdateWaveformData();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Show Waveform", nullptr, &m_showWaveform)) {}
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

    static std::string FormatTimecode(double seconds) {
        if (seconds < 0) seconds = 0;
        int totalSeconds = static_cast<int>(seconds);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int secs = totalSeconds % 60;
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << secs;
        return oss.str();
    }

    void VideoView::RenderVideoInfo() {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            try {
                const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
                ImGui::Text("File: %s", videoComp.fileName.c_str());
                ImGui::Text("Dimensions: %dx%d, FPS: %.2f, Frames: %d",
                    videoComp.width, videoComp.height, videoComp.fps, videoComp.frameCount);

                bool hasAudio = HasAudioTrack(selectedEntityID);
                double totalDuration = GetVideoDuration(selectedEntityID);
                double currentTime = GetVideoCurrentTime(selectedEntityID);

                ImGui::Text("Time: %s / %s", FormatTimecode(currentTime).c_str(), FormatTimecode(totalDuration).c_str());
                ImGui::Text("Current Frame: %d / %d", videoComp.currentFrame, videoComp.frameCount);
                ImGui::Text("Entity ID: %zu", selectedEntityID);

                if (hasAudio) {
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
        ImGui::SameLine();
        if (selectedEntityID != 0 && ImGui::Button("Save Video")) {
            SaveSelectedMedia();
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
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
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
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Prev")) {
            if (!mediaEntities.empty()) {
                index = (index - 1 + static_cast<int>(mediaEntities.size())) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Next")) {
            if (!mediaEntities.empty()) {
                index = (index + 1) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Last")) {
            if (!mediaEntities.empty()) {
                index = static_cast<int>(mediaEntities.size() - 1);
                selectedEntityID = mediaEntities[index];
                SeekVideo(selectedEntityID, GetVideoDuration(selectedEntityID) - 0.001);
                UpdateWaveformData();
            }
        }
    }

    double VideoView::GetVideoCurrentTime(ECS::EntityID entity) const {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback) {
            return playback->GetCurrentPosition(entity);
        }
        if (m_entityManager.IsEntityValid(entity) && m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
            return m_entityManager.GetComponent<ECS::VideoComponent>(entity).currentTime;
        }
        return 0.0;
    }

    double VideoView::GetVideoDuration(ECS::EntityID entity) const {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback) {
            return playback->GetDuration(entity);
        }
        if (m_entityManager.IsEntityValid(entity) && m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(entity);
            return videoComp.frameCount / videoComp.fps;
        }
        return 0.0;
    }

    void VideoView::SeekVideo(ECS::EntityID entity, double time) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return;

        if (m_isSeeking) {
            m_pendingSeek = true;
            m_pendingSeekTime = time;
            m_pendingSeekEntity = entity;
            return;
        }

        m_isSeeking = true;

        try {
            auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
            if (playback) {
                playback->Seek(entity, time);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Seek error: " << e.what() << std::endl;
        }

        m_isSeeking = false;

        if (m_pendingSeek) {
            m_pendingSeek = false;
            SeekVideo(m_pendingSeekEntity, m_pendingSeekTime);
        }
    }

    void VideoView::PlayVideo(ECS::EntityID entity, bool loop) {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback) {
            playback->Play(entity, loop);
        }
    }

    void VideoView::PauseVideo(ECS::EntityID entity) {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback) {
            playback->Pause(entity);
        }
    }

    void VideoView::StopVideo(ECS::EntityID entity) {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback) {
            playback->Stop(entity);
        }
    }

    void VideoView::SetVideoSpeed(ECS::EntityID entity, float speed) {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback) {
            playback->SetSpeed(entity, speed);
        }
    }

    void VideoView::SetVideoVolume(ECS::EntityID entity, float volume) {
        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (playback && HasAudioTrack(entity)) {
            playback->SetVolume(entity, volume);
        }
    }

    void VideoView::RenderPlaybackControls() {
        ImGui::Separator();
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            ImGui::Text("No video selected.");
            return;
        }

        auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
        bool isLoaded = (videoComp.width > 0 && videoComp.height > 0);

        if (!isLoaded) {
            ImGui::Text("Loading video...");
            return;
        }

        bool hasAudio = HasAudioTrack(selectedEntityID);

        try {
            auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
            if (!playback) {
                ImGui::Text("VideoPlaybackSystem not available.");
                return;
            }

            double totalDuration = GetVideoDuration(selectedEntityID);
            double currentTime = GetVideoCurrentTime(selectedEntityID);

            bool loopState = videoComp.looping;

            if (hasAudio) {
                bool useTimeSlider = true;
                ImGui::Checkbox("Use Time Slider", &useTimeSlider);

                if (useTimeSlider) {
                    float timeSlider = static_cast<float>(currentTime);
                    if (ImGui::SliderFloat("Time", &timeSlider, 0.0f, static_cast<float>(totalDuration), "%.3f s")) {
                        double newTime = static_cast<double>(timeSlider);
                        if (newTime < 0) newTime = 0;
                        if (newTime >= totalDuration) newTime = totalDuration - 0.001;
                        playback->Seek(selectedEntityID, newTime);
                    }
                }
                else {
                    long long frame = videoComp.currentFrame;
                    int frameSlider = static_cast<int>(frame);
                    if (ImGui::SliderInt("Frame", &frameSlider, 0, static_cast<int>(videoComp.frameCount - 1))) {
                        double newTime = static_cast<double>(frameSlider) / videoComp.fps;
                        playback->Seek(selectedEntityID, newTime);
                    }
                }

                float speed = 1.0f;
                if (ImGui::SliderFloat("Speed", &speed, 0.1f, 4.0f, "%.1fx")) {
                    playback->SetSpeed(selectedEntityID, speed);
                }

                ImGui::SameLine();
                bool playing = playback->IsPlaying(selectedEntityID);
                if (ImGui::Button(playing ? "Pause" : "Play")) {
                    if (playing) {
                        playback->Pause(selectedEntityID);
                    }
                    else {
                        playback->Play(selectedEntityID, loopState);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    playback->Stop(selectedEntityID);
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Loop", &loopState)) {
                    videoComp.looping = loopState;
                }
            }
            else {
                bool useTimeSlider = true;
                ImGui::Checkbox("Use Time Slider", &useTimeSlider);

                if (useTimeSlider) {
                    float timeSlider = static_cast<float>(currentTime);
                    if (ImGui::SliderFloat("Time", &timeSlider, 0.0f, static_cast<float>(totalDuration), "%.3f s")) {
                        double newTime = static_cast<double>(timeSlider);
                        if (newTime < 0) newTime = 0;
                        if (newTime >= totalDuration) newTime = totalDuration - 0.001;
                        playback->Seek(selectedEntityID, newTime);
                    }
                }
                else {
                    long long frame = videoComp.currentFrame;
                    int frameSlider = static_cast<int>(frame);
                    if (ImGui::SliderInt("Frame", &frameSlider, 0, static_cast<int>(videoComp.frameCount - 1))) {
                        double newTime = static_cast<double>(frameSlider) / videoComp.fps;
                        playback->Seek(selectedEntityID, newTime);
                    }
                }

                float speed = 1.0f;
                if (ImGui::SliderFloat("Speed", &speed, 0.1f, 4.0f, "%.1fx")) {
                    playback->SetSpeed(selectedEntityID, speed);
                    videoComp.playbackSpeed = speed;
                }

                ImGui::SameLine();
                bool playing = playback->IsPlaying(selectedEntityID);
                if (ImGui::Button(playing ? "Pause" : "Play")) {
                    if (playing) {
                        playback->Pause(selectedEntityID);
                    }
                    else {
                        playback->Play(selectedEntityID, loopState);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    playback->Stop(selectedEntityID);
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Loop", &loopState)) {
                    videoComp.looping = loopState;
                }
            }
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
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);

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

    void VideoView::RenderWaveform() {
        if (!HasAudioTrack(selectedEntityID)) return;

        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            return;
        }

        const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

        if (audioComp.pcmData.empty()) {
            ImGui::Text("No audio data available for waveform.");
            return;
        }

        if (m_waveformData.empty()) {
            UpdateWaveformData();
        }

        if (m_waveformData.empty()) {
            ImGui::Text("No waveform data available.");
            return;
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float width = avail.x;
        float height = std::min(avail.y * 0.4f, 128.0f);

        if (width <= 0 || height <= 0) return;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
            IM_COL32(20, 20, 30, 200));

        float centerY = pos.y + height * 0.5f;
        float halfHeight = height * 0.4f;

        size_t totalSamples = m_waveformData.size();
        size_t samplesToShow = static_cast<size_t>(width);

        if (samplesToShow > totalSamples) samplesToShow = totalSamples;
        if (samplesToShow == 0) return;

        float step = static_cast<float>(totalSamples) / static_cast<float>(samplesToShow);
        size_t currentSample = static_cast<size_t>(m_playbackProgress * totalSamples);

        for (size_t i = 0; i < samplesToShow; ++i) {
            size_t sampleIndex = static_cast<size_t>(i * step);
            if (sampleIndex >= totalSamples) break;

            float x = pos.x + (static_cast<float>(i) / static_cast<float>(samplesToShow)) * width;
            float sample = m_waveformData[sampleIndex];
            float heightPos = sample * halfHeight;

            bool isBeforePlayhead = (sampleIndex <= currentSample);
            ImU32 color = isBeforePlayhead ?
                IM_COL32(100, 255, 100, 200) :
                IM_COL32(100, 150, 255, 150);

            drawList->AddLine(
                ImVec2(x, centerY - heightPos),
                ImVec2(x, centerY + heightPos),
                color,
                1.0f
            );
        }

        if (m_playbackProgress > 0.0f && m_playbackProgress < 1.0f) {
            float playheadX = pos.x + m_playbackProgress * width;
            drawList->AddLine(
                ImVec2(playheadX, pos.y),
                ImVec2(playheadX, pos.y + height),
                IM_COL32(255, 255, 255, 200),
                2.0f
            );
        }

        drawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height),
            IM_COL32(100, 100, 120, 255));

        ImGui::InvisibleButton("WaveformClick", ImVec2(width, height));
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relativeX = (mousePos.x - pos.x) / width;
            if (relativeX >= 0.0f && relativeX <= 1.0f) {
                auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
                if (playback) {
                    double duration = playback->GetDuration(selectedEntityID);
                    playback->Seek(selectedEntityID, relativeX * duration);
                    m_playbackProgress = relativeX;
                }
            }
        }

        ImGui::Dummy(ImVec2(0, height + 4));
    }

    void VideoView::UpdateWaveformData() {
        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            m_waveformData.clear();
            return;
        }

        const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

        if (audioComp.pcmData.empty()) {
            m_waveformData.clear();
            return;
        }

        const float* data = audioComp.pcmData.data();
        size_t totalSamples = audioComp.pcmData.size() / audioComp.channels;
        int channels = audioComp.channels;

        size_t targetSize = 4096;
        m_waveformData.resize(targetSize);

        if (totalSamples > 0) {
            float step = static_cast<float>(totalSamples) / static_cast<float>(targetSize);

            for (size_t i = 0; i < targetSize; ++i) {
                size_t startIdx = static_cast<size_t>(i * step);
                size_t endIdx = static_cast<size_t>((i + 1) * step);
                if (endIdx > totalSamples) endIdx = totalSamples;

                float peak = 0.0f;
                for (size_t j = startIdx; j < endIdx; ++j) {
                    float sample = 0.0f;
                    for (int c = 0; c < channels; ++c) {
                        sample += std::abs(data[j * channels + c]);
                    }
                    sample /= channels;
                    if (sample > peak) peak = sample;
                }
                m_waveformData[i] = peak;
            }
        }
    }

    bool VideoView::HasAudioTrack(ECS::EntityID entity) const {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return false;
        return m_entityManager.HasComponent<ECS::AudioComponent>(entity);
    }

    void VideoView::RenderAudioControls(ECS::EntityID entity) {
        if (!HasAudioTrack(entity)) return;

        auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
        if (!playback) return;

        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(entity);

        ImGui::SameLine();
        ImGui::Text("Audio:");
        ImGui::SameLine();

        bool audioEnabled = (audioComp.volume > 0.0f);
        if (ImGui::Checkbox("##AudioEnabled", &audioEnabled)) {
            float newVolume = audioEnabled ? 1.0f : 0.0f;
            playback->SetVolume(entity, newVolume);
            audioComp.volume = newVolume;
        }

        ImGui::SameLine();
        float volumePercent = audioComp.volume * 100.0f;
        if (ImGui::SliderFloat("##AudioVolume", &volumePercent, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
            float newVolume = volumePercent / 100.0f;
            playback->SetVolume(entity, newVolume);
            audioComp.volume = newVolume;
        }
    }

    void VideoView::LoadMedia(const std::vector<std::string>& filePaths) {
        std::cout << "[VideoView] Loading " << filePaths.size() << " videos..." << std::endl;
        auto vaSystem = m_entityManager.GetSystem<ECS::VideoAudioSystem>();
        if (!vaSystem) {
            std::cerr << "[VideoView] VideoAudioSystem not found!" << std::endl;
            return;
        }
        try {
            for (const auto& filePath : filePaths) {
                if (filePath.empty()) continue;
                ECS::EntityID entity = vaSystem->LoadVideoWithAudio(filePath);
                std::cout << "[VideoView] Loaded: " << filePath << " (Entity: " << entity << ")" << std::endl;
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
        if (videoComp.filePath.empty() || !videoComp.fmtCtx) {
            std::cerr << "[VideoView] Video not loaded or invalid" << std::endl;
            return;
        }

        std::string outputPath = filePath.empty() ? videoComp.filePath : filePath;

        std::vector<Utils::VideoFrame> frames;
        long long originalFrame = videoComp.currentFrame;

        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) return;

        bool isNewFrame = false;
        if (!videoSystem->DecodeFrameForSave(videoComp, 0, isNewFrame)) {
            std::cerr << "[VideoView] Failed to seek to beginning" << std::endl;
            return;
        }
        videoComp.currentFrame = 0;

        while (videoComp.currentFrame < videoComp.frameCount) {
            Utils::VideoFrame frame;
            frame.width = videoComp.width;
            frame.height = videoComp.height;
            frame.channels = 4;

            std::shared_lock lock(videoComp.dataMutex);
            unsigned char* data = (unsigned char*)malloc(videoComp.frameDataRGBA.size());
            if (!data) {
                break;
            }
            std::memcpy(data, videoComp.frameDataRGBA.data(), videoComp.frameDataRGBA.size());
            lock.unlock();
            frame.data = data;
            frames.push_back(frame);

            long long nextFrame = videoComp.currentFrame + 1;
            if (nextFrame >= videoComp.frameCount) break;

            if (!videoSystem->DecodeFrameForSave(videoComp, nextFrame, isNewFrame)) {
                break;
            }
            if (isNewFrame) {
                videoComp.currentFrame = nextFrame;
            }
            else {
                break;
            }
        }

        if (!frames.empty()) {
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

        videoSystem->DecodeFrameForSave(videoComp, originalFrame, isNewFrame);
        if (isNewFrame) {
            videoComp.currentFrame = originalFrame;
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
            auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
            if (playback) {
                playback->Stop(selectedEntityID);
            }

            auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
            if (videoSystem) videoSystem->RemoveVideo(selectedEntityID);
            selectedEntityID = 0;
            index = 0;
            m_waveformData.clear();
            RefreshEntities();
            std::cout << "[VideoView] Video removed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception removing video: " << e.what() << std::endl;
        }
    }

    void VideoView::PauseAllVideos() {
        try {
            auto playback = m_entityManager.GetSystem<ECS::VideoPlaybackSystem>();
            if (!playback) return;

            for (auto entityID : mediaEntities) {
                if (m_entityManager.IsEntityValid(entityID) &&
                    m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                    if (playback->IsPlaying(entityID)) {
                        playback->Pause(entityID);
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
        UpdateWaveformData();
        if (entity == selectedEntityID) {
            if (m_entityManager.IsEntityValid(entity) &&
                m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
                auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(entity);
                videoComp.needsTextureUpdate = true;
            }
        }
    }

    void VideoView::OnMediaRemoved(ECS::EntityID entity) {
        RefreshEntities();
        UpdateSelectionAfterRemoval(entity);
        if (selectedEntityID == 0) {
            m_waveformData.clear();
        }
        else {
            UpdateWaveformData();
        }
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