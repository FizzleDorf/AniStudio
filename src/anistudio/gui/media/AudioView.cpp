#include "AudioView.hpp"
#include "TextureSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "MediaHistoryView.hpp"
#include "MetadataView.hpp"
#include "AVSystem.hpp"
#include "AudioSystem.hpp"
#include "FilePathSystem.hpp"
#include "PlaybackEvents.hpp"
#include "ClipboardUtilities.hpp"
#include "IconFonts.hpp"
#include "Log.hpp"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace GUI {

    AudioView::AudioView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm) {
        viewName = "AudioView";

        WaveformRenderer::Config wfConfig;
        wfConfig.height = 120.0f;
        m_waveformRenderer.SetConfig(wfConfig);

        ANI_LOG_DEBUG("[AudioView] Constructed");
    }

    AudioView::~AudioView() {
        if (m_audioSystem) {
            m_audioSystem->UnregisterCallbacksForOwner(this);
        }
        ANI_LOG_DEBUG("[AudioView] Destroyed");
    }

    void AudioView::Init() {
        ANI_LOG_INFO("[AudioView] Initializing...");

        if (!ImGui::GetCurrentContext()) {
            ANI_LOG_ERROR("[AudioView] No ImGui context in Init()!");
            return;
        }

        m_avSystem = m_entityManager.GetSystem<ECS::AVSystem>();
        if (!m_avSystem) {
            m_entityManager.RegisterSystem<ECS::AVSystem>();
            m_avSystem = m_entityManager.GetSystem<ECS::AVSystem>();
            if (m_avSystem) {
                m_avSystem->Start();
                ANI_LOG_INFO("[AudioView] Registered and started AVSystem");
            }
            else {
                ANI_LOG_ERROR("[AudioView] Failed to register AVSystem");
            }
        }

        auto& events = ANI::Events::Ref();

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_LOAD,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onLoad(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PLAY,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onPlay(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PAUSE,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onPause(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_STOP,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onStop(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SEEK,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onSeek(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_SPEED,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onSetSpeed(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_VOLUME,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onSetVolume(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_REMOVE,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onRemove(data);
            });

        events.RegisterEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_MODE,
            [this](const std::any& data) {
                if (m_avSystem) m_avSystem->onSetMode(data);
            });

        ANI_LOG_DEBUG("[AudioView] Registered playback events");

        if (m_avSystem) {
            m_avSystem->RegisterTrackStateCallback([this](ECS::EntityID entity, ECS::PlaybackState state) {
                if (entity != selectedEntityID) return;
                if (state == ECS::PlaybackState::EndOfStream) {
                    if (m_autoplay) {
                        bool loopEnabled = false;
                        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
                            auto& stateComp = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
                            loopEnabled = stateComp.looping;
                        }

                        if (loopEnabled) {
                            SeekAudio(entity, 0.0);
                            m_playbackProgress = 0.0f;
                            m_sliderValue = 0.0f;
                            PlayAudio(entity);
                        }
                        else if (mediaEntities.size() > 1) {
                            int newIndex = (index + 1) % static_cast<int>(mediaEntities.size());
                            if (newIndex != index) {
                                index = newIndex;
                                selectedEntityID = mediaEntities[index];
                                SeekAudio(selectedEntityID, 0.0);
                                m_playbackProgress = 0.0f;
                                m_sliderValue = 0.0f;
                                UpdateWaveformData();
                                PlayAudio(selectedEntityID);
                            }
                        }
                        else {
                            SeekAudio(entity, 0.0);
                            m_playbackProgress = 0.0f;
                            m_sliderValue = 0.0f;
                        }
                    }
                }
                });
        }

        auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        if (!audioSystem) {
            m_entityManager.RegisterSystem<ECS::AudioSystem>();
            audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
            if (!audioSystem) {
                ANI_LOG_ERROR("[AudioView] Failed to register AudioSystem");
            }
            else {
                ANI_LOG_DEBUG("[AudioView] Registered AudioSystem");
            }
        }
        m_audioSystem = audioSystem;

        if (audioSystem) {
            audioSystem->RegisterAudioAddedCallback(this, [this](ECS::EntityID entity) {
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

            audioSystem->RegisterAudioRemovedCallback(this, [this](ECS::EntityID entity) {
                OnMediaRemoved(entity);
                });

            audioSystem->RegisterAudioDataCallback(this,
                [this](ECS::EntityID entity, const float* data, size_t size, int channels, int sampleRate) {
                    (void)data; (void)size; (void)channels; (void)sampleRate;
                    if (entity == selectedEntityID) {
                        UpdateWaveformData();
                    }
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
                                m_entityManager.HasComponent<ECS::AudioComponent>(entity)) {
                                SetSelectedEntity(entity);
                                UpdateWaveformData();
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[AudioView] SelectMediaEntity event error: %s", e.what());
            }
            });

        ANI_LOG_INFO("[AudioView] Initialization complete");
    }

    void AudioView::SetPlaybackMode(ECS::PlaybackMode mode) {
        if (mode == m_playbackMode) return;
        m_playbackMode = mode;

        ANI_LOG_DEBUG("[AudioView] Playback mode set to %s",
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

    ECS::PlaybackMode AudioView::GetPlaybackMode() const {
        return m_playbackMode;
    }

    void AudioView::Update(float deltaT) {
        size_t currentCount = 0;
        for (auto entityID : m_entityManager.GetAllEntities()) {
            if (m_entityManager.HasComponent<ECS::AudioComponent>(entityID)) currentCount++;
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
            m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                if (state.duration > 0.0) {
                    m_playbackProgress = static_cast<float>(state.currentTime / state.duration);
                    if (m_playbackProgress < 0) m_playbackProgress = 0;
                    if (m_playbackProgress > 1) m_playbackProgress = 1;
                    m_sliderValue = m_playbackProgress;
                }
            }
        }
    }

    void AudioView::Render() {
        if (!ImGui::GetCurrentContext()) {
            ANI_LOG_ERROR("[AudioView] No ImGui context in Render()!");
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
        std::string windowName = "Audio Player##" + std::to_string(GetID());

        if (!ImGui::Begin(windowName.c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            ImGui::End();
            return;
        }

        try {
            RenderMenuBar();
            RenderToolbar();
            RenderMediaInfo();
            ImGui::Separator();
            if (ImGui::BeginChild("AudioViewerChild", ImVec2(0, -120), true)) {
                RenderMediaContent();
            }
            ImGui::EndChild();
            RenderControls();
            RenderPlaybackControls();
            RenderSelector();

            HandleClipboardPaste();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[AudioView] Exception in Render: %s", e.what());
            ImGui::Text("Error rendering AudioView: %s", e.what());
        }

        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void AudioView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::PushID(1);
                if (ImGui::MenuItem((Icon::Music() + " " + Icon::FolderOpen() + " Load Audio(s)").c_str())) {
                    static std::string lastAudioFolder;
                    std::vector<std::string> filePaths;
                    if (FileDialog::OpenFiles("Choose Audio File(s)", FileDialog::FilterType::AUDIO_FILE, filePaths, lastAudioFolder)) {
                        if (!filePaths.empty()) {
                            LoadMedia(filePaths);
                            lastAudioFolder = std::filesystem::path(filePaths[0]).parent_path().string();
                        }
                    }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(2);
                if (ImGui::MenuItem((Icon::Save() + " Save Audio").c_str(), nullptr, false, selectedEntityID != 0)) {
                    SaveSelectedMedia();
                }
                ImGui::PopID();
                ImGui::PushID(3);
                if (ImGui::MenuItem((Icon::SaveAs() + " Save Audio As...").c_str(), nullptr, false, selectedEntityID != 0)) {
                    auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                    std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
                    std::string defaultName = "audio.wav";
                    if (selectedEntityID != 0) {
                        const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
                        if (!audioComp.fileName.empty()) {
                            defaultName = audioComp.fileName;
                            size_t dotPos = defaultName.find_last_of('.');
                            if (dotPos != std::string::npos) defaultName = defaultName.substr(0, dotPos);
                            defaultName += ".wav";
                        }
                    }
                    std::string outPath;
                    if (FileDialog::SaveFile("Save Audio As", FileDialog::FilterType::AUDIO_FILE, defaultName, outPath, defaultPath)) {
                        if (!outPath.empty()) SaveSelectedMediaAs(outPath);
                    }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(4);
                if (ImGui::MenuItem((Icon::Trash() + " Remove Audio").c_str(), nullptr, false, selectedEntityID != 0)) {
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
                if (ImGui::MenuItem((Icon::ArrowFirst() + " First Audio").c_str(), nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = 0;
                        selectedEntityID = mediaEntities[index];
                        UpdateWaveformData();
                        PauseAllAudio();
                        m_playbackProgress = 0.0f;
                        m_sliderValue = 0.0f;
                    }
                }
                ImGui::PopID();
                ImGui::PushID(7);
                if (ImGui::MenuItem((Icon::ArrowLast() + " Last Audio").c_str(), nullptr, false, !mediaEntities.empty())) {
                    if (!mediaEntities.empty()) {
                        index = static_cast<int>(mediaEntities.size()) - 1;
                        selectedEntityID = mediaEntities[index];
                        UpdateWaveformData();
                        PauseAllAudio();
                        m_playbackProgress = 0.0f;
                        m_sliderValue = 0.0f;
                    }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(8);
                if (ImGui::MenuItem((Icon::Music() + " Show Waveform").c_str(), nullptr, &m_showWaveform)) {}
                ImGui::PopID();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void AudioView::RenderToolbar() {
        ImGui::PushID(100);

        ImGui::PushID(101);
        if (ImGui::Button((Icon::Music() + " Load").c_str())) {
            static std::string lastAudioFolder;
            std::vector<std::string> filePaths;
            if (FileDialog::OpenFiles("Choose Audio File(s)", FileDialog::FilterType::AUDIO_FILE, filePaths, lastAudioFolder)) {
                if (!filePaths.empty()) {
                    LoadMedia(filePaths);
                    lastAudioFolder = std::filesystem::path(filePaths[0]).parent_path().string();
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
            auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            std::string defaultPath = fileSys ? fileSys->GetPath("DataPath") : ".";
            std::string defaultName = "audio.wav";
            if (selectedEntityID != 0) {
                const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
                if (!audioComp.fileName.empty()) {
                    defaultName = audioComp.fileName;
                    size_t dotPos = defaultName.find_last_of('.');
                    if (dotPos != std::string::npos) defaultName = defaultName.substr(0, dotPos);
                    defaultName += ".wav";
                }
            }
            std::string outPath;
            if (FileDialog::SaveFile("Save Audio As", FileDialog::FilterType::AUDIO_FILE, defaultName, outPath, defaultPath)) {
                if (!outPath.empty()) SaveSelectedMediaAs(outPath);
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

        ImGui::SameLine();
        ImGui::PushID(107);
        if (ImGui::Button((Icon::Music() + " Test Tone").c_str())) {
            auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
            if (audioSystem) {
                ANI_LOG_DEBUG("[AudioView] Playing test tone");
                audioSystem->PlayTestTone();
            }
            else {
                ANI_LOG_WARN("[AudioView] AudioSystem unavailable, cannot play test tone");
            }
        }
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID(108);
        if (ImGui::Button((Icon::Music() + " Waveform").c_str())) {
            m_showWaveform = !m_showWaveform;
        }
        ImGui::PopID();

        ImGui::PopID();
    }

    void AudioView::RenderMediaInfo() {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            try {
                const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

                ImGui::Text("File: %s", audioComp.fileName.c_str());

                double currentTime = 0.0;
                double duration = 0.0;
                bool loopEnabled = false;
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    currentTime = state.currentTime;
                    duration = state.duration;
                    loopEnabled = state.looping;
                }

                int minutes = static_cast<int>(currentTime) / 60;
                int seconds = static_cast<int>(currentTime) % 60;
                int totalMinutes = static_cast<int>(duration) / 60;
                int totalSeconds = static_cast<int>(duration) % 60;

                ImGui::SameLine();
                ImGui::Text("| Time: %02d:%02d / %02d:%02d", minutes, seconds, totalMinutes, totalSeconds);
                ImGui::SameLine();
                ImGui::Text("| Channels: %d", audioComp.channels);
                ImGui::SameLine();
                ImGui::Text("| Sample Rate: %d Hz", audioComp.sampleRate);

                bool isPlaying = false;
                if (m_avSystem) {
                    isPlaying = (m_avSystem->GetState(selectedEntityID) == ECS::PlaybackState::Playing);
                }

                if (isPlaying) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "| %s", Icon::Play().c_str());
                }

                if (loopEnabled) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "| %s", Icon::Loop().c_str());
                }

                ImGui::SameLine();
                ImGui::Text("| Entity ID: %u", selectedEntityID);

                RenderMediaContextMenu(selectedEntityID);
            }
            catch (const std::exception& e) {
                ImGui::Text("Error reading audio info: %s", e.what());
            }
        }
        else {
            ImGui::Text("No audio loaded.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
    }

    void AudioView::RenderControls() {
        ImGui::Checkbox((Icon::Music() + " Show Waveform").c_str(), &m_showWaveform);
        ImGui::SameLine();

        if (GUI::Clipboard::HasEntity() || GUI::Clipboard::HasComponent() || GUI::Clipboard::HasProperty()) {
            ImGui::SameLine();
            std::string label;
            if (GUI::Clipboard::HasEntity()) label = "Entity";
            else if (GUI::Clipboard::HasComponent()) label = "Component";
            else if (GUI::Clipboard::HasProperty()) label = "Property";
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s", label.c_str());
        }

        ImGui::SameLine();
        ImGui::Text("Mode:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Cached", m_playbackMode == ECS::PlaybackMode::Cached)) {
            SetPlaybackMode(ECS::PlaybackMode::Cached);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Streaming", m_playbackMode == ECS::PlaybackMode::Streaming)) {
            SetPlaybackMode(ECS::PlaybackMode::Streaming);
        }

        bool loopState = false;
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
            auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
            loopState = state.looping;
        }

        ImGui::SameLine();
        ImGui::PushID(300);
        if (ImGui::Checkbox((Icon::Loop() + " Loop").c_str(), &loopState)) {
            if (m_avSystem && selectedEntityID != 0) {
                m_avSystem->SetLooping(selectedEntityID, loopState);
            }
            ANI_LOG_DEBUG("[AudioView] Loop %s", loopState ? "enabled" : "disabled");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Loop current audio");
        }
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID(301);
        if (ImGui::Checkbox((Icon::Play() + " Autoplay").c_str(), &m_autoplay)) {
            ANI_LOG_DEBUG("[AudioView] Autoplay %s", m_autoplay ? "enabled" : "disabled");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Auto play next audio when current ends");
        }
        ImGui::PopID();
    }

    void AudioView::RenderSelector() {
        if (mediaEntities.empty()) {
            ImGui::Text("No audio loaded.");
            return;
        }

        ImGui::PushItemWidth(100.0f);
        if (ImGui::InputInt("Current Audio", &index)) {
            if (!mediaEntities.empty()) {
                const int size = static_cast<int>(mediaEntities.size());
                if (size == 1) index = 0;
                else index = ((index % size) + size) % size;
                selectedEntityID = mediaEntities[index];
                UpdateWaveformData();
                PauseAllAudio();
                m_playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("Audio %d of %zu", index + 1, mediaEntities.size());
        ImGui::SameLine();
        ImGui::PushID(200);
        if (ImGui::Button(Icon::ArrowFirst().c_str())) {
            if (!mediaEntities.empty()) {
                index = 0;
                selectedEntityID = mediaEntities[index];
                UpdateWaveformData();
                PauseAllAudio();
                m_playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(201);
        if (ImGui::Button(Icon::ArrowLeft().c_str())) {
            if (!mediaEntities.empty()) {
                index = (index - 1 + static_cast<int>(mediaEntities.size())) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                UpdateWaveformData();
                PauseAllAudio();
                m_playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(202);
        if (ImGui::Button(Icon::ArrowRight().c_str())) {
            if (!mediaEntities.empty()) {
                index = (index + 1) % static_cast<int>(mediaEntities.size());
                selectedEntityID = mediaEntities[index];
                UpdateWaveformData();
                PauseAllAudio();
                m_playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(203);
        if (ImGui::Button(Icon::ArrowLast().c_str())) {
            if (!mediaEntities.empty()) {
                index = static_cast<int>(mediaEntities.size()) - 1;
                selectedEntityID = mediaEntities[index];
                UpdateWaveformData();
                PauseAllAudio();
                m_playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
        }
        ImGui::PopID();
    }

    void AudioView::RenderMediaContent() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            ImGui::Text("No audio loaded.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
            return;
        }

        if (m_showWaveform) {
            RenderWaveform();
        }
        else {
            const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
            ImGui::Text("Audio: %s", audioComp.fileName.c_str());
            double currentTime = 0.0;
            double duration = 0.0;
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                currentTime = state.currentTime;
                duration = state.duration;
            }
            int minutes = static_cast<int>(currentTime) / 60;
            int seconds = static_cast<int>(currentTime) % 60;
            int totalMinutes = static_cast<int>(duration) / 60;
            int totalSeconds = static_cast<int>(duration) % 60;
            ImGui::Text("Time: %02d:%02d / %02d:%02d", minutes, seconds, totalMinutes, totalSeconds);
            HandleFileDropTarget();
            HandleEntityDropTarget();
        }
    }

    void AudioView::RefreshEntities() {
        try {
            mediaEntities.clear();
            for (auto entityID : m_entityManager.GetAllEntities()) {
                if (m_entityManager.HasComponent<ECS::AudioComponent>(entityID)) {
                    mediaEntities.push_back(entityID);
                }
            }
            lastEntityCount = mediaEntities.size();
            ANI_LOG_DEBUG("[AudioView] Refreshed entities, found %zu audio file(s)",
                mediaEntities.size());
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[AudioView] Exception refreshing entities: %s", e.what());
            mediaEntities.clear();
            lastEntityCount = 0;
        }
    }

    void AudioView::RenderPlaybackControls() {
        ImGui::Separator();

        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            ImGui::Text("No audio selected.");
            return;
        }

        try {
            bool isPlaying = false;
            if (m_avSystem) {
                isPlaying = (m_avSystem->GetState(selectedEntityID) == ECS::PlaybackState::Playing);
            }

            ImGui::PushID(400);
            if (isPlaying) {
                if (ImGui::Button((Icon::Pause() + " Pause").c_str())) {
                    PauseAudio(selectedEntityID);
                }
            }
            else {
                if (ImGui::Button((Icon::Play() + " Play").c_str())) {
                    PlayAudio(selectedEntityID);
                }
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID(401);
            if (ImGui::Button(Icon::Stop().c_str())) {
                StopAudio(selectedEntityID);
                m_playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("Vol:");
            ImGui::SameLine();

            float volumePercent = 0.0f;
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                volumePercent = state.volume * 100.0f;
            }

            ImGui::PushItemWidth(100.0f);
            if (ImGui::SliderFloat("##VolumeSlider", &volumePercent, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
                float newVolume = volumePercent / 100.0f;
                SetAudioVolume(selectedEntityID, newVolume);
            }
            ImGui::PopItemWidth();

            double currentTime = 0.0;
            double duration = 0.0;
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                currentTime = state.currentTime;
                duration = state.duration;
            }

            m_sliderValue = m_playbackProgress;

            ImGui::Text("Progress:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);

            ImGui::PushID(402);
            if (ImGui::SliderFloat("##SeekSlider", &m_sliderValue, 0.0f, 1.0f, "%.1f%%")) {
                double seekTime = m_sliderValue * duration;
                SeekAudio(selectedEntityID, seekTime);
                m_playbackProgress = m_sliderValue;
            }
            ImGui::PopID();

            int minutes = static_cast<int>(currentTime) / 60;
            int seconds = static_cast<int>(currentTime) % 60;
            int totalMinutes = static_cast<int>(duration) / 60;
            int totalSeconds = static_cast<int>(duration) % 60;

            ImGui::SameLine();
            ImGui::Text("%02d:%02d / %02d:%02d", minutes, seconds, totalMinutes, totalSeconds);

            ImGui::Separator();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[AudioView] Exception in RenderPlaybackControls: %s", e.what());
            ImGui::Text("Error with playback controls: %s", e.what());
        }
    }

    void AudioView::RenderWaveform() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            ImGui::Text("No audio loaded.");
            return;
        }

        const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

        if (audioComp.pcmData.empty()) {
            ImGui::Text("No audio data available for waveform.");
            HandleFileDropTarget();
            HandleEntityDropTarget();
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

        std::string currentStr = GUI::FormatTimecode(currentTime);
        std::string totalStr = GUI::FormatTimecode(duration);

        m_waveformRenderer.Render(m_waveformData, m_playbackProgress, currentStr, totalStr,
            [this](double progress) {
                double duration = 0.0;
                if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
                    auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
                    duration = state.duration;
                }
                SeekAudio(selectedEntityID, progress * duration);
                m_playbackProgress = static_cast<float>(progress);
                m_sliderValue = static_cast<float>(progress);
            });
    }

    void AudioView::UpdateWaveformData() {
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

    void AudioView::LoadMedia(const std::vector<std::string>& filePaths) {
        ANI_LOG_INFO("[AudioView] LoadMedia called with %zu file(s)", filePaths.size());

        if (!m_avSystem) {
            ANI_LOG_ERROR("[AudioView] AVSystem is null!");
            return;
        }

        try {
            for (const auto& filePath : filePaths) {
                if (filePath.empty()) continue;

                ECS::EntityID entity = m_avSystem->LoadMedia(
                    filePath, ECS::TrackType::Audio, m_playbackMode);

                if (entity == 0) {
                    ANI_LOG_WARN("[AudioView] Failed to load audio: %s", filePath.c_str());
                    continue;
                }

                ANI_LOG_INFO("[AudioView] Created entity %u for: %s",
                    entity, filePath.c_str());

                auto playEvent = ANI::PlaybackEvents::MakeEntityEvent(entity);
                ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PLAY, playEvent);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[AudioView] Exception loading audio: %s", e.what());
        }
    }

    void AudioView::SaveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            ANI_LOG_WARN("[AudioView] SaveSelectedMedia: no valid audio selected");
            return;
        }

        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
        if (audioComp.pcmData.empty()) {
            ANI_LOG_WARN("[AudioView] No audio data to save.");
            return;
        }

        std::string defaultName = audioComp.fileName;
        if (defaultName.empty()) defaultName = "audio";
        size_t dotPos = defaultName.find_last_of('.');
        if (dotPos != std::string::npos) defaultName = defaultName.substr(0, dotPos);
        defaultName += ".wav";

        std::string savePath;
        std::string defaultDir = std::filesystem::current_path().string();
        if (FileDialog::SaveFile("Save Audio As", FileDialog::FilterType::AUDIO_FILE, defaultName, savePath, defaultDir)) {
            if (!savePath.empty()) {
                SaveSelectedMediaAs(savePath);
            }
        }
    }

    void AudioView::SaveSelectedMediaAs(const std::string& filePath) {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            ANI_LOG_WARN("[AudioView] SaveSelectedMediaAs: no valid audio selected");
            return;
        }

        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
        if (audioComp.pcmData.empty()) {
            ANI_LOG_WARN("[AudioView] SaveSelectedMediaAs: no audio data");
            return;
        }

        int channels = audioComp.channels;
        int sampleRate = audioComp.sampleRate;
        const float* data = audioComp.pcmData.data();
        size_t totalSamples = audioComp.pcmData.size();

        FILE* f = fopen(filePath.c_str(), "wb");
        if (!f) {
            ANI_LOG_ERROR("[AudioView] Failed to open file for writing: %s", filePath.c_str());
            return;
        }

        int bitsPerSample = 32;
        int bytesPerSample = bitsPerSample / 8;
        int byteRate = sampleRate * channels * bytesPerSample;
        int blockAlign = channels * bytesPerSample;

        uint32_t dataSize = static_cast<uint32_t>(totalSamples * bytesPerSample);
        uint32_t fileSize = 44 + dataSize;

        fwrite("RIFF", 1, 4, f);
        fwrite(&fileSize, 4, 1, f);
        fwrite("WAVE", 1, 4, f);
        fwrite("fmt ", 1, 4, f);
        uint32_t fmtSize = 16;
        fwrite(&fmtSize, 4, 1, f);
        uint16_t audioFormat = 3;
        fwrite(&audioFormat, 2, 1, f);
        fwrite(&channels, 2, 1, f);
        fwrite(&sampleRate, 4, 1, f);
        fwrite(&byteRate, 4, 1, f);
        fwrite(&blockAlign, 2, 1, f);
        fwrite(&bitsPerSample, 2, 1, f);
        fwrite("data", 1, 4, f);
        fwrite(&dataSize, 4, 1, f);

        for (size_t i = 0; i < totalSamples; ++i) {
            float sample = data[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            fwrite(&sample, 4, 1, f);
        }

        fclose(f);
        ANI_LOG_INFO("[AudioView] Saved audio to: %s", filePath.c_str());
    }

    void AudioView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) {
            ANI_LOG_WARN("[AudioView] RemoveSelectedMedia: no valid entity selected");
            return;
        }

        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(selectedEntityID)) {
            auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(selectedEntityID);
            state.isLoaded = false;
        }

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(selectedEntityID);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_REMOVE, eventData);

        ANI_LOG_DEBUG("[AudioView] Removed entity %u", selectedEntityID);

        selectedEntityID = 0;
        index = 0;
        m_waveformData.Clear();
        m_playbackProgress = 0.0f;
        m_sliderValue = 0.0f;
        RefreshEntities();
    }

    void AudioView::PauseAllAudio() {
        if (!m_avSystem) return;
        for (auto entityID : mediaEntities) {
            if (m_entityManager.IsEntityValid(entityID) &&
                m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entityID)) {
                if (m_avSystem->GetState(entityID) == ECS::PlaybackState::Playing) {
                    PauseAudio(entityID);
                }
            }
        }
    }

    void AudioView::OnMediaAdded(ECS::EntityID entity) {
        ANI_LOG_TRACE("[AudioView] Media added: entity %u", entity);
        RefreshEntities();
    }

    void AudioView::OnMediaRemoved(ECS::EntityID entity) {
        ANI_LOG_TRACE("[AudioView] Media removed: entity %u", entity);
        RefreshEntities();
        UpdateSelectionAfterRemoval(entity);
        if (selectedEntityID == 0) {
            m_waveformData.Clear();
        }
        else {
            UpdateWaveformData();
        }
    }

    bool AudioView::IsHistoryVisible() const {
        return GetViewManager().HasView<MediaHistoryView>(GetID());
    }

    std::string AudioView::GetHistoryViewTypeName() const {
        return "MediaHistoryView";
    }

    std::string AudioView::GetSelectedFilePath() const {
        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            return m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID).filePath;
        }
        return "";
    }

    void AudioView::PlayAudio(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("[AudioView] PlayAudio: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("[AudioView] PlayAudio: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.state = ECS::PlaybackState::Playing;
        state.isPaused = false;

        ANI_LOG_TRACE("[AudioView] PlayAudio: entity %u", entity);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PLAY, eventData);
    }

    void AudioView::PauseAudio(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("[AudioView] PauseAudio: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("[AudioView] PauseAudio: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.state = ECS::PlaybackState::Paused;
        state.isPaused = true;

        ANI_LOG_TRACE("[AudioView] PauseAudio: entity %u", entity);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_PAUSE, eventData);
    }

    void AudioView::StopAudio(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("[AudioView] StopAudio: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("[AudioView] StopAudio: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.state = ECS::PlaybackState::Stopped;
        state.isPaused = false;
        state.currentTime = 0.0;

        ANI_LOG_TRACE("[AudioView] StopAudio: entity %u", entity);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_STOP, eventData);
    }

    void AudioView::SeekAudio(ECS::EntityID entity, double time) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("[AudioView] SeekAudio: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("[AudioView] SeekAudio: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.currentTime = time;

        ANI_LOG_TRACE("[AudioView] SeekAudio: entity %u to %.3f", entity, time);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SEEK, eventData);
    }

    void AudioView::SetAudioVolume(ECS::EntityID entity, float volume) {
        if (!m_entityManager.IsEntityValid(entity)) {
            ANI_LOG_TRACE("[AudioView] SetAudioVolume: invalid entity %u", entity);
            return;
        }
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            ANI_LOG_TRACE("[AudioView] SetAudioVolume: entity %u has no PlaybackStateComponent", entity);
            return;
        }

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.volume = volume;

        ANI_LOG_TRACE("[AudioView] SetAudioVolume: entity %u volume=%.2f", entity, volume);

        auto eventData = ANI::PlaybackEvents::MakeEntityEvent(entity);
        ANI::Events::Ref().QueueEventWithData(ANI::PlaybackEvents::EVENT_PLAYBACK_SET_VOLUME, eventData);
    }

} // namespace GUI