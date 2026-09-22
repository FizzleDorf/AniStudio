#include "Log.hpp"
#include "VideoView.hpp"
#include "TextureSystem.hpp"
#include "TextureComponent.hpp"
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
#include "PlaybackEvents.hpp"
#include "IconFonts.hpp"
#include "ClipboardUtilities.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <imgui.h>

namespace GUI {

    VideoView::VideoView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm), lastGeneratedVideoID(0) {
        viewName = "VideoView";

        WaveformRenderer::Config wfConfig;
        wfConfig.height = 64.0f;
        m_waveformRenderer.SetConfig(wfConfig);

        ANI_LOG_DEBUG("Constructed");
    }

    VideoView::~VideoView() {
        if (m_videoSystem) {
            m_videoSystem->UnregisterCallbacksForOwner(this);
        }
        ANI_LOG_DEBUG("Destroyed");
    }

    void VideoView::Init() {
        ANI_LOG_INFO("Initializing...");

        if (!ImGui::GetCurrentContext()) {
            ANI_LOG_ERROR("No ImGui context in Init()!");
            return;
        }

        m_audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();

        m_mediaEngine = m_entityManager.GetSystem<ECS::MediaEngineSystem>();
        if (!m_mediaEngine) {
            m_entityManager.RegisterSystem<ECS::MediaEngineSystem>();
            m_mediaEngine = m_entityManager.GetSystem<ECS::MediaEngineSystem>();
            if (m_mediaEngine) {
                m_mediaEngine->Start();
                ANI_LOG_INFO("Registered and started MediaEngineSystem");
            }
            else {
                ANI_LOG_ERROR("Failed to register MediaEngineSystem");
            }
        }

        auto textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
        if (textureSystem && m_mediaEngine) {
            m_mediaEngine->SetVideoTextureCallback(
                [textureSystem](ECS::EntityID entity, unsigned char* data, int w, int h, int ch, GLuint* target) {
                    textureSystem->QueueVideoTextureCreation(entity, data, w, h, ch, target);
                }
            );
        }
        else if (!textureSystem) {
            ANI_LOG_WARN("TextureSystem unavailable; video textures will not be created");
        }

        auto& events = ANI::Events::Ref();

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_LOAD,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onLoad(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PLAY,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onPlay(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PAUSE,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onPause(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_STOP,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onStop(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SEEK,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onSeek(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_SPEED,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onSetSpeed(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_VOLUME,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onSetVolume(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_REMOVE,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onRemove(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_MODE,
            [this](const std::any& data) {
                if (m_mediaEngine) m_mediaEngine->onSetMode(data);
            });

        ANI_LOG_DEBUG("Registered playback events");

        if (m_mediaEngine) {
            m_mediaEngine->RegisterTrackStateCallback([this](ECS::EntityID entity, ECS::PlaybackState state) {
                if (entity != selectedEntityID) return;
                if (state == ECS::PlaybackState::EndOfStream) {
                    if (m_autoplay) {
                        bool loopEnabled = false;
                        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
                            auto& stateComp = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
                            loopEnabled = stateComp.looping;
                        }

                        if (loopEnabled) {
                            SeekVideo(entity, 0.0);
                            m_playbackProgress = 0.0f;
                            PlayVideo(entity);
                        }
                        else if (mediaEntities.size() > 1) {
                            int newIndex = (index + 1) % static_cast<int>(mediaEntities.size());
                            if (newIndex != index) {
                                index = newIndex;
                                selectedEntityID = mediaEntities[index];
                                SeekVideo(selectedEntityID, 0.0);
                                m_playbackProgress = 0.0f;
                                UpdateWaveformData();
                                PlayVideo(selectedEntityID);
                            }
                        }
                        else {
                            SeekVideo(entity, 0.0);
                            m_playbackProgress = 0.0f;
                        }
                    }
                }
                });
        }

        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            m_videoSystem = videoSystem;

            auto textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
            if (textureSystem) {
                videoSystem->SetVideoTextureCallback(
                    [textureSystem](ECS::EntityID entityID, unsigned char* data,
                        int width, int height, int channels, GLuint* targetTexture) {
                            textureSystem->QueueVideoTextureCreation(entityID, data, width, height, channels, targetTexture);
                    }
                );
            }
            videoSystem->RegisterVideoAddedCallback(this, [this](ECS::EntityID entity) {
                OnMediaAdded(entity);
                });
            videoSystem->RegisterVideoRemovedCallback(this, [this](ECS::EntityID entity) {
                OnMediaRemoved(entity);
                });
        }
        else {
            ANI_LOG_WARN("VideoSystem unavailable");
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
                ANI_LOG_ERROR("SelectMediaEntity event error: %s", e.what());
            }
            });

        ANI_LOG_INFO("Initialization complete");
    }

    void VideoView::SetPlaybackMode(ECS::PlaybackMode mode) {
        if (mode == m_playbackMode) return;
        m_playbackMode = mode;

        ANI_LOG_DEBUG("Playback mode set to %s",
            mode == ECS::PlaybackMode::Streaming ? "Streaming" : "Cached");

        for (auto entity : mediaEntities) {
            if (m_entityManager.IsEntityValid(entity) &&
                m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
                state.mode = mode;
                auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
                ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_MODE, eventData);
            }
        }
    }

    ECS::PlaybackMode VideoView::GetPlaybackMode() const {
        return m_playbackMode;
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
                m_waveformData.Clear();
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
                else if (!mediaEntities.empty()) {
                    index = static_cast<int>(mediaEntities.size()) - 1;
                    selectedEntityID = mediaEntities[index];
                    UpdateWaveformData();
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
                        if (!m_entityManager.HasComponent<ECS::TextureComponent>(selectedEntityID)) {
                            m_entityManager.AddComponent<ECS::TextureComponent>(selectedEntityID);
                            ANI_LOG_TRACE("Added TextureComponent to entity %u", selectedEntityID);
                        }
                        auto& texComp = m_entityManager.GetComponent<ECS::TextureComponent>(selectedEntityID);
                        unsigned char* copyData = (unsigned char*)malloc(videoComp.frameDataRGBA.size());
                        if (copyData) {
                            memcpy(copyData, videoComp.frameDataRGBA.data(), videoComp.frameDataRGBA.size());
                            textureSystem->QueueVideoTextureCreation(
                                selectedEntityID,
                                copyData,
                                videoComp.width,
                                videoComp.height,
                                4,
                                &texComp.textureID
                            );
                        }
                        else {
                            ANI_LOG_ERROR("malloc failed for texture copy (entity %u, %zu bytes)",
                                selectedEntityID, videoComp.frameDataRGBA.size());
                        }
                        videoComp.needsTextureUpdate = false;
                    }
                }
            }

            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                if (state.duration > 0.0) {
                    m_playbackProgress = static_cast<float>(state.currentTime / state.duration);
                    if (m_playbackProgress < 0) m_playbackProgress = 0;
                    if (m_playbackProgress > 1) m_playbackProgress = 1;
                }
            }
        }
    }

    void VideoView::Render() {
        if (!ImGui::GetCurrentContext()) {
            ANI_LOG_ERROR("No ImGui context in Render()!");
            return;
        }

        if (m_isFullscreen) {
            RenderFullscreen();
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);
        std::string windowName = "Video Viewer##" + std::to_string(GetID());
        if (!ImGui::Begin(windowName.c_str(), &windowOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar)) {
            ImGui::End();
            return;
        }

        try {
            RenderMenuBar();
            RenderToolbar();
            RenderMediaInfo();
            ImGui::Separator();

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float timelineHeight = 64.0f;
            float controlsHeight = 86.0f;
            float selectorHeight = 64.0f;
            float totalControlsHeight = timelineHeight + controlsHeight + selectorHeight + 20.0f;
            float videoHeight = avail.y - totalControlsHeight;
            if (videoHeight < 100.0f) videoHeight = 100.0f;
            ImVec2 videoSize = ImVec2(avail.x, videoHeight);

            ImGui::PushID("VideoDisplay");
            if (ImGui::BeginChild("VideoDisplayChild", videoSize, false, ImGuiWindowFlags_NoScrollbar)) {
                RenderMediaContent();
            }
            ImGui::EndChild();
            ImGui::PopID();

            ImGui::PushID("Timeline");
            if (HasAudioTrack(selectedEntityID) && m_showWaveform && !m_waveformData.IsEmpty()) {
                RenderWaveform();
            }
            else {
                RenderTimeline();
            }
            ImGui::PopID();

            ImGui::PushID("Controls");
            RenderControls();
            ImGui::PopID();

            ImGui::PushID("Selector");
            RenderSelector();
            ImGui::PopID();

            HandleClipboardPaste();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception in Render: %s", e.what());
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

    void VideoView::RenderFullscreen() {
        if (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)) {
            m_showFullscreenControls = true;
            m_fullscreenControlsTimer = 5.0f;
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportPos = viewport->Pos;
        ImVec2 viewportSize = viewport->Size;

        ImGui::SetNextWindowPos(viewportPos);
        ImGui::SetNextWindowSize(viewportSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

        if (ImGui::Begin("Fullscreen Video", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

            ImGuiIO& io = ImGui::GetIO();

            if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_F11)) {
                ToggleFullscreen();
            }

            ImVec2 currentMousePos = io.MousePos;
            bool mouseMoved = (std::abs(currentMousePos.x - m_lastMousePos.x) > 2.0f ||
                std::abs(currentMousePos.y - m_lastMousePos.y) > 2.0f);
            m_lastMousePos = currentMousePos;

            bool keyPressed = false;
            for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key))) {
                    keyPressed = true;
                    break;
                }
            }

            if (mouseMoved || keyPressed) {
                m_showFullscreenControls = true;
                m_fullscreenControlsTimer = 5.0f;
            }

            if (m_showFullscreenControls) {
                m_fullscreenControlsTimer -= ImGui::GetIO().DeltaTime;
                if (m_fullscreenControlsTimer <= 0.0f) {
                    m_showFullscreenControls = false;
                }
            }

            ImVec2 avail = ImGui::GetContentRegionAvail();
            float overlayHeight = m_showFullscreenControls ? 140.0f : 0.0f;
            float displayHeight = avail.y - overlayHeight;
            if (displayHeight < 100.0f) displayHeight = 100.0f;

            if (ImGui::BeginChild("FullscreenVideoDisplay", ImVec2(avail.x, displayHeight), false, ImGuiWindowFlags_NoScrollbar)) {
                RenderMediaContent();
            }
            ImGui::EndChild();

            if (m_showFullscreenControls) {
                ImVec2 overlayPos = ImVec2(0, displayHeight);
                ImGui::SetCursorPos(overlayPos);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.6f));
                if (ImGui::BeginChild("FullscreenControls", ImVec2(avail.x, overlayHeight), false, ImGuiWindowFlags_NoScrollbar)) {
                    float totalWidth = 700.0f;
                    float centerX = (avail.x - totalWidth) * 0.5f;
                    if (centerX < 0) centerX = 0;

                    ImGui::SetCursorPosX(centerX);

                    if (HasAudioTrack(selectedEntityID) && m_showWaveform && !m_waveformData.IsEmpty()) {
                        float savedCursorX = ImGui::GetCursorPosX();
                        ImGui::SetCursorPosX(0);
                        RenderWaveform();
                        ImGui::SetCursorPosX(centerX);
                    }
                    else {
                        RenderTimeline();
                    }

                    RenderControls();

                    ImVec2 hintPos = ImVec2(avail.x - 150.0f, 10.0f);
                    ImGui::SetCursorPos(hintPos);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.4f));
                    ImGui::Text("Esc to exit");
                    ImGui::PopStyleColor();
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
        }

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    void VideoView::RenderMediaContent() {
        ImVec2 avail = ImGui::GetContentRegionAvail();

        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            const char* text = "No video selected.";
            float textWidth = ImGui::CalcTextSize(text).x;
            float textHeight = ImGui::GetTextLineHeight();
            ImVec2 pos = ImVec2(
                (avail.x - textWidth) * 0.5f,
                (avail.y - textHeight) * 0.5f
            );
            ImGui::SetCursorPos(pos);
            ImGui::Text("%s", text);
            HandleFileDropTarget();
            HandleEntityDropTarget();
            return;
        }

        try {
            auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            GLuint texID = 0;
            if (m_entityManager.HasComponent<ECS::TextureComponent>(selectedEntityID)) {
                texID = m_entityManager.GetComponent<ECS::TextureComponent>(selectedEntityID).textureID;
            }

            if (texID == 0 || !glIsTexture(texID) || videoComp.width <= 0 || videoComp.height <= 0) {
                const char* text = "Video loading...";
                float textWidth = ImGui::CalcTextSize(text).x;
                float textHeight = ImGui::GetTextLineHeight();
                ImVec2 pos = ImVec2(
                    (avail.x - textWidth) * 0.5f,
                    (avail.y - textHeight) * 0.5f
                );
                ImGui::SetCursorPos(pos);
                ImGui::Text("%s", text);
                HandleFileDropTarget();
                HandleEntityDropTarget();
                return;
            }

            m_videoAspectRatio = static_cast<float>(videoComp.width) / static_cast<float>(videoComp.height);

            ImVec2 imageSize;

            switch (m_displayMode) {
            case DisplayMode::ActualResolution:
                imageSize = ImVec2(static_cast<float>(videoComp.width), static_cast<float>(videoComp.height));
                break;
            case DisplayMode::Fullscreen:
            case DisplayMode::FitToWindow:
            default:
                float availRatio = avail.x / avail.y;
                if (availRatio > m_videoAspectRatio) {
                    imageSize.y = avail.y;
                    imageSize.x = avail.y * m_videoAspectRatio;
                }
                else {
                    imageSize.x = avail.x;
                    imageSize.y = avail.x / m_videoAspectRatio;
                }
                break;
            }

            ImVec2 offset = ImVec2(
                (avail.x - imageSize.x) * 0.5f,
                (avail.y - imageSize.y) * 0.5f
            );

            ImGui::SetCursorPos(offset);
            ImGui::Image((ImTextureID)(intptr_t)texID, imageSize, ImVec2(0, 0), ImVec2(1, 1));

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                ToggleFullscreen();
            }

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
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception in RenderMediaContent: %s", e.what());
            const char* text = "Error rendering video";
            float textWidth = ImGui::CalcTextSize(text).x;
            float textHeight = ImGui::GetTextLineHeight();
            ImVec2 pos = ImVec2(
                (avail.x - textWidth) * 0.5f,
                (avail.y - textHeight) * 0.5f
            );
            ImGui::SetCursorPos(pos);
            ImGui::Text("%s", text);
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
    }

    void VideoView::RenderTimeline() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            return;
        }

        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);

        std::string currentStr = FormatTimecode(state.currentTime);
        std::string totalStr = FormatTimecode(state.duration);

        float availWidth = ImGui::GetContentRegionAvail().x;
        float sliderWidth = availWidth - 20.0f;

        if (sliderWidth < 50.0f) sliderWidth = 50.0f;

        float startX = (availWidth - sliderWidth) * 0.5f;
        if (startX < 0) startX = 0;
        ImGui::SetCursorPosX(startX);

        ImGui::PushItemWidth(sliderWidth);
        ImGui::PushID("TimelineSlider");
        if (ImGui::SliderFloat("##Timeline", &m_playbackProgress, 0.0f, 1.0f, "")) {
            SeekVideo(selectedEntityID, m_playbackProgress * state.duration);
        }
        if (ImGui::IsItemHovered()) {
            float hoverPos = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x;
            float hoverProgress = hoverPos / ImGui::GetItemRectSize().x;
            hoverProgress = std::clamp(hoverProgress, 0.0f, 1.0f);
            double hoverTime = hoverProgress * state.duration;
            ImGui::SetTooltip("%s", FormatTimecode(hoverTime).c_str());
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        std::string timeDisplay = currentStr + " / " + totalStr;
        float textWidth = ImGui::CalcTextSize(timeDisplay.c_str()).x;
        float centerX = (availWidth - textWidth) * 0.5f;
        if (centerX < 0) centerX = 0;
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("%s", timeDisplay.c_str());
    }

    std::string VideoView::FormatTimecode(double seconds) const {
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

    void VideoView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::PushID(1);
                if (ImGui::MenuItem((Icon::Video() + " " + Icon::FolderOpen() + " Load Video(s)").c_str())) {
                    static std::string lastVideoFolder;
                    std::vector<std::string> filePaths;
                    if (FileDialog::OpenFiles("Choose Video(s)", FileDialog::FilterType::VIDEO_FILE, filePaths, lastVideoFolder)) {
                        if (!filePaths.empty()) {
                            LoadMedia(filePaths);
                            lastVideoFolder = std::filesystem::path(filePaths[0]).parent_path().string();
                        }
                    }
                }
                ImGui::PopID();
                ImGui::Separator();

                ImGui::PushID(2);
                if (ImGui::BeginMenu((Icon::Save() + " Save").c_str(), selectedEntityID != 0)) {
                    if (ImGui::MenuItem((Icon::Music() + " Save Video (with Audio)").c_str(), nullptr, false,
                        selectedEntityID != 0 && HasAudioTrack(selectedEntityID))) {
                        SaveSelectedMedia();
                    }

                    if (ImGui::MenuItem((Icon::File() + " Save Video (No Audio)").c_str(), nullptr, false, selectedEntityID != 0)) {
                        SaveSelectedMediaNoAudio();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem((Icon::SaveAs() + " Save Video As (with Audio)").c_str(), nullptr, false,
                        selectedEntityID != 0 && HasAudioTrack(selectedEntityID))) {
                        SaveSelectedMediaAsWithAudio();
                    }

                    if (ImGui::MenuItem((Icon::SaveAs() + " Save Video As (No Audio)").c_str(), nullptr, false, selectedEntityID != 0)) {
                        SaveSelectedMediaAsNoAudio();
                    }

                    ImGui::EndMenu();
                }
                ImGui::PopID();

                ImGui::Separator();
                ImGui::PushID(3);
                if (ImGui::MenuItem((Icon::Trash() + " Remove Video").c_str(), nullptr, false, selectedEntityID != 0)) {
                    RemoveSelectedMedia();
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(4);
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
                ImGui::PushID(5);
                if (ImGui::MenuItem((Icon::ArrowFirst() + " First Video").c_str(), nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = 0;
                        selectedEntityID = mediaEntities[index];
                        SeekVideo(selectedEntityID, 0.0);
                        UpdateWaveformData();
                    }
                }
                ImGui::PopID();
                ImGui::PushID(6);
                if (ImGui::MenuItem((Icon::ArrowLast() + " Last Video").c_str(), nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = static_cast<int>(mediaEntities.size()) - 1;
                        selectedEntityID = mediaEntities[index];
                        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                            auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                            SeekVideo(selectedEntityID, state.duration - 0.001);
                        }
                        UpdateWaveformData();
                    }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(7);
                if (ImGui::MenuItem((Icon::Music() + " Show Waveform").c_str(), nullptr, &m_showWaveform)) {}
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(8);
                if (ImGui::MenuItem((Icon::Zoom() + " Fit to Window").c_str(), nullptr, m_displayMode == DisplayMode::FitToWindow)) {
                    m_displayMode = DisplayMode::FitToWindow;
                }
                ImGui::PopID();
                ImGui::PushID(9);
                if (ImGui::MenuItem("Actual Resolution", nullptr, m_displayMode == DisplayMode::ActualResolution)) {
                    m_displayMode = DisplayMode::ActualResolution;
                }
                ImGui::PopID();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void VideoView::RenderToolbar() {
        ImGui::PushID(100);

        ImGui::PushID(101);
        if (ImGui::Button((Icon::Video() + " Load").c_str())) {
            static std::string lastVideoFolder;
            std::vector<std::string> filePaths;
            if (FileDialog::OpenFiles("Choose Video(s)", FileDialog::FilterType::VIDEO_FILE, filePaths, lastVideoFolder)) {
                if (!filePaths.empty()) {
                    LoadMedia(filePaths);
                    lastVideoFolder = std::filesystem::path(filePaths[0]).parent_path().string();
                }
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
            if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
                !m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
                ANI_LOG_WARN("Save As: no valid video selected");
            }
            else {
                auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
                std::string outPath;
                const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
                std::string defaultName = videoComp.fileName;
                if (FileDialog::SaveFile("Save Video As", FileDialog::FilterType::VIDEO_FILE, defaultName, outPath, defaultPath)) {
                    if (!outPath.empty()) SaveSelectedMediaAs(outPath);
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

    void VideoView::RenderMediaInfo() {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            try {
                const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);

                double currentTime = 0.0;
                double duration = 0.0;
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    currentTime = state.currentTime;
                    duration = state.duration;
                }

                ImGui::Text("File: %s | %dx%d | %.2f FPS | %d frames | Time: %s / %s | Frame: %d/%d | Entity: %u",
                    videoComp.fileName.c_str(),
                    videoComp.width, videoComp.height,
                    videoComp.fps,
                    videoComp.frameCount,
                    FormatTimecode(currentTime).c_str(),
                    FormatTimecode(duration).c_str(),
                    videoComp.currentFrame,
                    videoComp.frameCount,
                    selectedEntityID);

                if (HasAudioTrack(selectedEntityID)) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%s", Icon::Music().c_str());
                }

                if (selectedEntityID == lastGeneratedVideoID) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "NEWLY GENERATED");
                }

                RenderMediaContextMenu(selectedEntityID);
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("Exception in RenderMediaInfo: %s", e.what());
                ImGui::Text("Error reading video info: %s", e.what());
            }
        }
        else {
            ImGui::Text("No video loaded.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
    }

    void VideoView::RenderControls() {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(24.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

        float availWidth = ImGui::GetContentRegionAvail().x;
        float totalWidth = 0.0f;

        totalWidth += 110.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 80.0f + ImGui::GetStyle().ItemSpacing.x;

        float startX = (availWidth - totalWidth) * 0.5f;
        if (startX < 0) startX = 0;
        ImGui::SetCursorPosX(startX);

        bool hasVideo = (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID));

        if (!hasVideo) {
            ImGui::TextDisabled("No video loaded");
            ImGui::PopStyleVar(2);
            ImGui::Separator();

            ImGui::Text("Mode:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Cached", m_playbackMode == ECS::PlaybackMode::Cached)) {
                SetPlaybackMode(ECS::PlaybackMode::Cached);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Streaming", m_playbackMode == ECS::PlaybackMode::Streaming)) {
                SetPlaybackMode(ECS::PlaybackMode::Streaming);
            }
            return;
        }

        auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
        bool isLoaded = (videoComp.width > 0 && videoComp.height > 0);

        if (!isLoaded) {
            ImGui::TextDisabled("Loading video...");
            ImGui::PopStyleVar(2);
            ImGui::Separator();

            ImGui::Text("Mode:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Cached", m_playbackMode == ECS::PlaybackMode::Cached)) {
                SetPlaybackMode(ECS::PlaybackMode::Cached);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Streaming", m_playbackMode == ECS::PlaybackMode::Streaming)) {
                SetPlaybackMode(ECS::PlaybackMode::Streaming);
            }
            return;
        }

        try {
            bool playing = false;
            bool loopState = false;
            float speed = videoComp.playbackSpeed;

            if (m_mediaEngine) {
                playing = (m_mediaEngine->GetState(selectedEntityID) == ECS::PlaybackState::Playing);
            }

            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                loopState = state.looping;
            }

            ImGui::Text("%s", Icon::ClockRotateLeft().c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Playback Speed");
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(80.0f);
            ImGui::PushID(305);
            if (ImGui::DragFloat("##Speed", &speed, 0.05f, 0.1f, 4.0f, "%.1fx")) {
                speed = std::clamp(speed, 0.1f, 4.0f);
                videoComp.playbackSpeed = speed;
                SetVideoSpeed(selectedEntityID, speed);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Adjust playback speed (0.1x - 4.0x)");
            }
            ImGui::PopID();
            ImGui::PopItemWidth();

            ImGui::SameLine();

            if (HasAudioTrack(selectedEntityID)) {
                RenderAudioControls(selectedEntityID);
                ImGui::SameLine();
            }

            ImGui::PushID(308);
            if (ImGui::Button(Icon::ArrowFirst().c_str())) {
                SeekVideo(selectedEntityID, 0.0);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Go to first frame");
            }
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(302);
            bool held = false;
            if (ImGui::Button(Icon::ArrowLeft().c_str())) {
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Previous frame (hold for repeat)");
            }
            ImGuiID prevID = ImGui::GetItemID();
            if (m_repeatButtonHandler.Process(prevID, 0.4, 0.1, held)) {
                float fps = videoComp.fps;
                float step = 1.0f / fps;
                auto& states = m_repeatButtonHandler.GetAllStates();
                auto it = states.find(prevID);
                if (it != states.end()) {
                    int count = it->second.repeatCount;
                    if (count > 10) step *= 30.0f;
                    else if (count > 5) step *= 10.0f;
                }
                double current = 0.0;
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    current = state.currentTime;
                }
                double newTime = current - step;
                if (newTime < 0.0) newTime = 0.0;
                SeekVideo(selectedEntityID, newTime);
            }
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(300);
            if (ImGui::Button(playing ? Icon::Pause().c_str() : Icon::Play().c_str())) {
                if (playing) {
                    PauseVideo(selectedEntityID);
                }
                else {
                    PlayVideo(selectedEntityID);
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(playing ? "Pause playback" : "Play video");
            }
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(301);
            if (ImGui::Button(Icon::Stop().c_str())) {
                StopVideo(selectedEntityID);
                m_playbackProgress = 0.0f;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Stop playback and go to beginning");
            }
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(303);
            if (ImGui::Button(Icon::ArrowRight().c_str())) {
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Next frame (hold for repeat)");
            }
            ImGuiID nextID = ImGui::GetItemID();
            if (m_repeatButtonHandler.Process(nextID, 0.4, 0.1, held)) {
                float fps = videoComp.fps;
                float step = 1.0f / fps;
                auto& states = m_repeatButtonHandler.GetAllStates();
                auto it = states.find(nextID);
                if (it != states.end()) {
                    int count = it->second.repeatCount;
                    if (count > 10) step *= 30.0f;
                    else if (count > 5) step *= 10.0f;
                }
                double current = 0.0;
                double duration = 0.0;
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    current = state.currentTime;
                    duration = state.duration;
                }
                double newTime = current + step;
                if (newTime > duration) newTime = duration;
                SeekVideo(selectedEntityID, newTime);
            }
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(309);
            if (ImGui::Button(Icon::ArrowLast().c_str())) {
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    SeekVideo(selectedEntityID, state.duration - 0.001);
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Go to last frame");
            }
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID(306);
            if (ImGui::Button(Icon::Expand().c_str())) {
                ToggleFullscreen();
            }
            ImGui::PopID();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle Fullscreen (F11 or Double-click video)");
            }
            ImGui::SameLine();

            ImGui::PushID(307);
            if (ImGui::Checkbox((Icon::Music() + " Waveform").c_str(), &m_showWaveform)) {
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Show/hide audio waveform timeline");
            }
            ImGui::PopID();

        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception in RenderControls: %s", e.what());
            ImGui::Text("Error with playback controls: %s", e.what());
        }

        ImGui::PopStyleVar(2);
        ImGui::Separator();

        ImGui::Text("Mode:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Cached", m_playbackMode == ECS::PlaybackMode::Cached)) {
            SetPlaybackMode(ECS::PlaybackMode::Cached);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Streaming", m_playbackMode == ECS::PlaybackMode::Streaming)) {
            SetPlaybackMode(ECS::PlaybackMode::Streaming);
        }

        if (hasVideo) {
            bool loopState = false;
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                loopState = state.looping;
            }

            ImGui::SameLine();
            ImGui::PushID(310);
            if (ImGui::Checkbox((Icon::Loop() + " Loop").c_str(), &loopState)) {
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    state.looping = loopState;
                }
                if (m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
                    auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
                    videoComp.looping = loopState;
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Loop current video");
            }
            ImGui::PopID();
        }

        ImGui::SameLine();
        ImGui::PushID(311);
        if (ImGui::Checkbox((Icon::Play() + " Autoplay").c_str(), &m_autoplay)) {
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Auto play next video when current ends");
        }
        ImGui::PopID();
    }

    void VideoView::RenderSelector() {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));

        float availWidth = ImGui::GetContentRegionAvail().x;

        if (mediaEntities.empty()) {
            ImGui::TextDisabled("No videos loaded.");
            ImGui::PopStyleVar(2);
            return;
        }

        int currentFrame = 0;
        int totalFrames = 0;
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
            currentFrame = videoComp.currentFrame;
            totalFrames = videoComp.frameCount;
        }

        float totalWidth = 0.0f;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 70.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 200.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 50.0f + ImGui::GetStyle().ItemSpacing.x;
        totalWidth += 20.0f + ImGui::GetStyle().ItemSpacing.x;

        float startX = (availWidth - totalWidth) * 0.5f;
        if (startX < 0) startX = 0;
        ImGui::SetCursorPosX(startX);

        ImGui::PushID(205);
        if (ImGui::Button(Icon::ArrowFirst().c_str())) {
            if (!mediaEntities.empty()) {
                index = 0;
                selectedEntityID = mediaEntities[index];
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("First video");
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(206);
        if (ImGui::Button(Icon::ArrowLeft().c_str())) {
            PreviousVideo();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Previous video");
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushItemWidth(60.0f);
        ImGui::PushID(209);
        if (ImGui::InputInt("##VideoSelector", &index, 0, 0)) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                if (size == 1) index = 0;
                else index = ((index % size) + size) % size;
                selectedEntityID = mediaEntities[index];
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Video index (zero indexed)");
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::Text("/ %d videos", (int)mediaEntities.size());
        ImGui::SameLine();

        ImGui::PushID(207);
        if (ImGui::Button(Icon::ArrowRight().c_str())) {
            NextVideo();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Next video");
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(208);
        if (ImGui::Button(Icon::ArrowLast().c_str())) {
            if (!mediaEntities.empty()) {
                index = static_cast<int>(mediaEntities.size()) - 1;
                selectedEntityID = mediaEntities[index];
                SeekVideo(selectedEntityID, 0.0);
                UpdateWaveformData();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Last video");
        }
        ImGui::PopID();

        ImGui::PopStyleVar(2);
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
            ANI_LOG_DEBUG("Refreshed entities, found %zu video(s)", mediaEntities.size());
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception refreshing entities: %s", e.what());
            mediaEntities.clear();
            lastEntityCount = 0;
        }
    }

    void VideoView::SeekVideo(ECS::EntityID entity, double time) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("SeekVideo: invalid entity %u", entity);
            return;
        }

        if (m_isSeeking) {
            m_pendingSeek = true;
            m_pendingSeekTime = time;
            m_pendingSeekEntity = entity;
            ANI_LOG_TRACE("SeekVideo: queued pending seek for entity %u to %.3f", entity, time);
            return;
        }

        m_isSeeking = true;

        try {
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
                state.currentTime = time;
            }

            auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
            ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SEEK, eventData);

            ANI_LOG_TRACE("SeekVideo: entity %u to %.3f", entity, time);
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Seek error: %s", e.what());
        }

        m_isSeeking = false;

        if (m_pendingSeek) {
            m_pendingSeek = false;
            SeekVideo(m_pendingSeekEntity, m_pendingSeekTime);
        }
    }

    void VideoView::PlayVideo(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("PlayVideo: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("PlayVideo: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.state = ECS::PlaybackState::Playing;
        state.isPaused = false;

        ANI_LOG_TRACE("PlayVideo: entity %u", entity);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PLAY, eventData);
    }

    void VideoView::PauseVideo(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("PauseVideo: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("PauseVideo: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.state = ECS::PlaybackState::Paused;
        state.isPaused = true;

        ANI_LOG_TRACE("PauseVideo: entity %u", entity);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PAUSE, eventData);
    }

    void VideoView::StopVideo(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("StopVideo: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("StopVideo: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.state = ECS::PlaybackState::Stopped;
        state.isPaused = false;
        state.currentTime = 0.0;

        ANI_LOG_TRACE("StopVideo: entity %u", entity);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_STOP, eventData);
    }

    void VideoView::SetVideoSpeed(ECS::EntityID entity, float speed) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("SetVideoSpeed: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("SetVideoSpeed: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.speed = speed;

        ANI_LOG_TRACE("SetVideoSpeed: entity %u speed=%.2f", entity, speed);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_SPEED, eventData);
    }

    void VideoView::SetVideoVolume(ECS::EntityID entity, float volume) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("SetVideoVolume: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("SetVideoVolume: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.volume = volume;

        ANI_LOG_TRACE("SetVideoVolume: entity %u volume=%.2f", entity, volume);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_VOLUME, eventData);
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

        if (m_waveformData.IsEmpty()) {
            UpdateWaveformData();
        }
        if (m_waveformData.IsEmpty()) {
            ImGui::Text("No waveform data available.");
            return;
        }

        double currentTime = 0.0;
        double duration = 0.0;
        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
            auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
            currentTime = state.currentTime;
            duration = state.duration;
        }

        std::string currentStr = FormatTimecode(currentTime);
        std::string totalStr = FormatTimecode(duration);

        m_waveformRenderer.Render(m_waveformData, m_playbackProgress, currentStr, totalStr,
            [this](double progress) {
                double duration = 0.0;
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    duration = state.duration;
                }
                SeekVideo(selectedEntityID, progress * duration);
            });
    }

    void VideoView::UpdateWaveformData() {
        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            m_waveformData.Clear();
            return;
        }

        const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

        if (audioComp.pcmData.empty()) {
            m_waveformData.Clear();
            return;
        }

        m_waveformData.Update(audioComp.pcmData.data(), audioComp.pcmData.size(), audioComp.channels);
    }

    bool VideoView::HasAudioTrack(ECS::EntityID entity) const {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return false;
        return m_entityManager.HasComponent<ECS::AudioComponent>(entity);
    }

    void VideoView::RenderAudioControls(ECS::EntityID entity) {
        if (!HasAudioTrack(entity)) return;

        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(entity);

        float vol = audioComp.volume;
        if (vol <= 0.0f) {
            ImGui::Text("%s", Icon::VolumeOff().c_str());
        }
        else if (vol < 0.33f) {
            ImGui::Text("%s", Icon::VolumeLow().c_str());
        }
        else if (vol < 0.66f) {
            ImGui::Text("%s", Icon::VolumeMedium().c_str());
        }
        else {
            ImGui::Text("%s", Icon::VolumeHigh().c_str());
        }
        ImGui::SameLine();

        bool audioEnabled = (audioComp.volume > 0.0f);
        ImGui::PushID(400);
        if (ImGui::Checkbox("##AudioEnabled", &audioEnabled)) {
            float newVolume = audioEnabled ? 1.0f : 0.0f;
            SetVideoVolume(entity, newVolume);
            audioComp.volume = newVolume;
        }
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushItemWidth(70.0f);
        float volumePercent = audioComp.volume * 100.0f;
        ImGui::PushID(401);
        if (ImGui::SliderFloat("##AudioVolume", &volumePercent, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
            float newVolume = volumePercent / 100.0f;
            SetVideoVolume(entity, newVolume);
            audioComp.volume = newVolume;
        }
        ImGui::PopID();
        ImGui::PopItemWidth();
    }

    void VideoView::ToggleFullscreen() {
        m_isFullscreen = !m_isFullscreen;
        ANI_LOG_DEBUG("Fullscreen %s", m_isFullscreen ? "enabled" : "disabled");
    }

    void VideoView::NextVideo() {
        if (mediaEntities.empty()) {
            ANI_LOG_TRACE("NextVideo: no media entities");
            return;
        }
        int newIndex = (index + 1) % static_cast<int>(mediaEntities.size());
        if (newIndex != index) {
            if (m_mediaEngine) {
                StopVideo(selectedEntityID);
            }
            index = newIndex;
            selectedEntityID = mediaEntities[index];
            SeekVideo(selectedEntityID, 0.0);
            UpdateWaveformData();
        }
    }

    void VideoView::PreviousVideo() {
        if (mediaEntities.empty()) {
            ANI_LOG_TRACE("PreviousVideo: no media entities");
            return;
        }
        int newIndex = (index - 1 + static_cast<int>(mediaEntities.size())) % static_cast<int>(mediaEntities.size());
        if (newIndex != index) {
            if (m_mediaEngine) {
                StopVideo(selectedEntityID);
            }
            index = newIndex;
            selectedEntityID = mediaEntities[index];
            SeekVideo(selectedEntityID, 0.0);
            UpdateWaveformData();
        }
    }

    void VideoView::LoadMedia(const std::vector<std::string>& filePaths) {
        ANI_LOG_INFO("LoadMedia called with %zu file(s)", filePaths.size());

        if (!m_mediaEngine) {
            ANI_LOG_ERROR("MediaEngineSystem is null!");
            return;
        }

        try {
            for (const auto& filePath : filePaths) {
                if (filePath.empty()) continue;

                ECS::EntityID entity = m_mediaEngine->LoadMedia(filePath, ECS::TrackType::Both, m_playbackMode);
                if (entity == 0) {
                    ANI_LOG_WARN("Failed to load video: %s", filePath.c_str());
                    continue;
                }

                ANI_LOG_INFO("Created entity %u for: %s", entity, filePath.c_str());

                auto playEvent = ANI::PlaybackEvents::MakeEntityEvent(entity);
                ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PLAY, playEvent);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception loading videos: %s", e.what());
        }
    }

    void VideoView::LoadVideo(const std::string& filePath) {
        LoadMedia({ filePath });
    }

    void VideoView::SaveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
            ANI_LOG_WARN("SaveSelectedMedia: no valid video selected");
            return;
        }
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            videoSystem->SaveVideoAsync(selectedEntityID);
        }
        else {
            ANI_LOG_WARN("SaveSelectedMedia: VideoSystem unavailable");
        }
    }

    void VideoView::SaveSelectedMediaAs(const std::string& filePath) {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
            ANI_LOG_WARN("SaveSelectedMediaAs: no valid video selected");
            return;
        }
        if (filePath.empty()) {
            ANI_LOG_WARN("SaveSelectedMediaAs: empty file path");
            return;
        }
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            videoSystem->SaveVideoAsync(selectedEntityID, filePath);
        }
        else {
            ANI_LOG_WARN("SaveSelectedMediaAs: VideoSystem unavailable");
        }
    }

    void VideoView::SaveSelectedMediaNoAudio() {
        SaveVideoNoAudio(selectedEntityID, "");
    }

    void VideoView::SaveSelectedMediaAsWithAudio() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
            ANI_LOG_WARN("SaveSelectedMediaAsWithAudio: no valid video selected");
            return;
        }

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
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
            ANI_LOG_WARN("SaveSelectedMediaAsNoAudio: no valid video selected");
            return;
        }

        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
        std::string outPath;

        const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(selectedEntityID);
        std::string defaultName = videoComp.fileName;

        if (FileDialog::SaveFile("Save Video As (No Audio)", FileDialog::FilterType::VIDEO_FILE, defaultName, outPath, defaultPath)) {
            SaveVideoNoAudio(selectedEntityID, outPath);
        }
    }

    void VideoView::SaveVideoWithAudio(ECS::EntityID entity, const std::string& filePath) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_WARN("SaveVideoWithAudio: invalid entity %u", entity);
            return;
        }
        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (videoSystem) {
            videoSystem->SaveVideoAsync(entity, filePath);
        }
        else {
            ANI_LOG_WARN("SaveVideoWithAudio: VideoSystem unavailable");
        }
    }

    void VideoView::SaveVideoNoAudio(ECS::EntityID entity, const std::string& filePath) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_WARN("SaveVideoNoAudio: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
            ANI_LOG_WARN("SaveVideoNoAudio: entity %u has no VideoComponent", entity);
            return;
        }

        auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(entity);
        if (videoComp.filePath.empty() || !videoComp.fmtCtx) {
            ANI_LOG_WARN("SaveVideoNoAudio: video not loaded or invalid (entity %u)", entity);
            return;
        }

        std::string outputPath = filePath.empty() ? videoComp.filePath : filePath;

        std::vector<Utils::VideoFrame> frames;
        long long originalFrame = videoComp.currentFrame;

        auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            ANI_LOG_WARN("SaveVideoNoAudio: VideoSystem unavailable");
            return;
        }

        bool isNewFrame = false;
        if (!videoSystem->DecodeFrameForSave(videoComp, 0, isNewFrame)) {
            ANI_LOG_ERROR("SaveVideoNoAudio: failed to seek to beginning");
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
                ANI_LOG_WARN("SaveVideoNoAudio: malloc failed, aborting at frame %lld",
                    videoComp.currentFrame);
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
                ANI_LOG_INFO("Saved video without audio to: %s", outputPath.c_str());
                if (filePath.empty()) {
                    videoComp.filePath = outputPath;
                    videoComp.fileName = std::filesystem::path(outputPath).filename().string();
                }
            }
            else {
                ANI_LOG_ERROR("Failed to save video without audio: %s", outputPath.c_str());
            }
        }
        else {
            ANI_LOG_WARN("SaveVideoNoAudio: no frames collected");
        }

        videoSystem->DecodeFrameForSave(videoComp, originalFrame, isNewFrame);
        if (isNewFrame) {
            videoComp.currentFrame = originalFrame;
        }
    }

    void VideoView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
            ANI_LOG_WARN("RemoveSelectedMedia: no valid video selected");
            return;
        }

        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
            auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
            state.isLoaded = false;
        }

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(selectedEntityID);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_REMOVE, eventData);

        ANI_LOG_DEBUG("Removed entity %u", selectedEntityID);

        selectedEntityID = 0;
        index = 0;
        m_waveformData.Clear();
        RefreshEntities();
    }

    void VideoView::PauseAllVideos() {
        if (!m_mediaEngine) return;
        for (auto entityID : mediaEntities) {
            if (m_entityManager.IsEntityValid(entityID) &&
                m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entityID)) {
                if (m_mediaEngine->GetState(entityID) == ECS::PlaybackState::Playing) {
                    PauseVideo(entityID);
                }
            }
        }
    }

    void VideoView::OnMediaAdded(ECS::EntityID entity) {
        ANI_LOG_TRACE("Media added: entity %u", entity);
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
        ANI_LOG_TRACE("Media removed: entity %u", entity);
        RefreshEntities();
        UpdateSelectionAfterRemoval(entity);
        if (selectedEntityID == 0) {
            m_waveformData.Clear();
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

} // namespace GUI