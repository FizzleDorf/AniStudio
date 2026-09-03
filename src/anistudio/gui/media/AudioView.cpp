#include "AudioView.hpp"
#include "TextureSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "MediaHistoryView.hpp"
#include "MetadataView.hpp"
#include "AudioPlaybackSystem.hpp"
#include "FilePathSystem.hpp"
#include "ClipboardUtilities.hpp"
#include "IconFonts.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <imgui.h>

namespace GUI {

    AudioView::AudioView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseMediaView(mgr, vm) {
        viewName = "AudioView";
    }

    void AudioView::Init() {
        std::cout << "[AudioView] Initializing..." << std::endl;

        if (!ImGui::GetCurrentContext()) {
            std::cerr << "[AudioView] No ImGui context in Init()!" << std::endl;
            return;
        }

        auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        if (!audioSystem) {
            m_entityManager.RegisterSystem<ECS::AudioSystem>();
            audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        }

        auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
        if (!playbackSystem) {
            m_entityManager.RegisterSystem<ECS::AudioPlaybackSystem>();
            playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
        }

        if (audioSystem) {
            audioSystem->RegisterAudioAddedCallback([this](ECS::EntityID entity) {
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

            audioSystem->RegisterAudioRemovedCallback([this](ECS::EntityID entity) {
                OnMediaRemoved(entity);
                });

            audioSystem->RegisterAudioDataCallback([this](ECS::EntityID entity, const float* data,
                size_t size, int channels, int sampleRate) {
                    if (entity == selectedEntityID) {
                        UpdateWaveformData();
                    }
                });
        }

        if (playbackSystem) {
            playbackSystem->RegisterPlaybackCallback([this](ECS::EntityID entity, const float* data,
                size_t size, int channels) {
                    if (entity == selectedEntityID) {
                        // Handle audio data if needed
                    }
                });
        }

        RefreshEntities();

        if (!mediaEntities.empty() && selectedEntityID == 0) {
            index = static_cast<int>(mediaEntities.size()) - 1;
            selectedEntityID = mediaEntities[index];
            UpdateWaveformData();
        }

        std::cout << "[AudioView] Initialization complete" << std::endl;

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
                std::cerr << "[AudioView] SelectMediaEntity event error: " << e.what() << std::endl;
            }
            });
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
                waveformData.clear();
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
                        waveformData.clear();
                    }
                }
            }
        }

        if (selectedEntityID != 0 && m_entityManager.IsEntityValid(selectedEntityID) &&
            m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
            if (playbackSystem) {
                double duration = playbackSystem->GetDuration(selectedEntityID);
                if (duration > 0.0) {
                    double currentPos = playbackSystem->GetCurrentPosition(selectedEntityID);
                    playbackProgress = static_cast<float>(currentPos / duration);
                    if (playbackProgress < 0) playbackProgress = 0;
                    if (playbackProgress > 1) playbackProgress = 1;
                    m_sliderValue = playbackProgress;
                }
            }
        }
    }

    void AudioView::Render() {
        if (!ImGui::GetCurrentContext()) {
            std::cerr << "[AudioView] ERROR: No ImGui context!" << std::endl;
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
            std::cerr << "[AudioView] Exception in Render: " << e.what() << std::endl;
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
                        playbackProgress = 0.0f;
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
                        playbackProgress = 0.0f;
                        m_sliderValue = 0.0f;
                    }
                }
                ImGui::PopID();
                ImGui::Separator();
                ImGui::PushID(8);
                if (ImGui::MenuItem((Icon::Music() + " Show Waveform").c_str(), nullptr, &showWaveform)) {}
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
            auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
            if (playbackSystem) {
                playbackSystem->PlayTestTone();
            }
        }
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID(108);
        if (ImGui::Button((Icon::Music() + " Waveform").c_str())) {
            showWaveform = !showWaveform;
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

                auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                if (!playbackSystem) {
                    ImGui::Text("AudioPlaybackSystem not available.");
                    return;
                }

                double duration = playbackSystem->GetDuration(selectedEntityID);
                double currentTime = playbackSystem->GetCurrentPosition(selectedEntityID);

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

                if (playbackSystem->IsPlaying(selectedEntityID)) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "| %s", Icon::Play().c_str());
                }
                else if (playbackSystem->IsPaused(selectedEntityID)) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "| %s", Icon::Pause().c_str());
                }

                if (audioComp.looping) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "| %s", Icon::Loop().c_str());
                }

                ImGui::SameLine();
                ImGui::Text("| Entity ID: %zu", selectedEntityID);

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
        ImGui::Checkbox((Icon::Music() + " Show Waveform").c_str(), &showWaveform);
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
                playbackProgress = 0.0f;
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
                playbackProgress = 0.0f;
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
                playbackProgress = 0.0f;
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
                playbackProgress = 0.0f;
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
                playbackProgress = 0.0f;
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

        if (showWaveform) {
            RenderWaveform();
        }
        else {
            const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
            ImGui::Text("Audio: %s", audioComp.fileName.c_str());
            auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
            if (playbackSystem) {
                double duration = playbackSystem->GetDuration(selectedEntityID);
                double currentTime = playbackSystem->GetCurrentPosition(selectedEntityID);
                int minutes = static_cast<int>(currentTime) / 60;
                int seconds = static_cast<int>(currentTime) % 60;
                int totalMinutes = static_cast<int>(duration) / 60;
                int totalSeconds = static_cast<int>(duration) % 60;
                ImGui::Text("Time: %02d:%02d / %02d:%02d", minutes, seconds, totalMinutes, totalSeconds);
            }
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
            std::cout << "[AudioView] Refreshed entities, found " << mediaEntities.size() << " audio files" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[AudioView] Exception refreshing entities: " << e.what() << std::endl;
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
            auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
            if (!playbackSystem) {
                ImGui::Text("AudioPlaybackSystem not available.");
                return;
            }

            auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

            bool isPlaying = playbackSystem->IsPlaying(selectedEntityID);
            bool isPaused = playbackSystem->IsPaused(selectedEntityID);

            ImGui::PushID(300);
            if (isPlaying && !isPaused) {
                if (ImGui::Button((Icon::Pause() + " Pause").c_str())) {
                    playbackSystem->Pause(selectedEntityID);
                }
            }
            else if (isPaused) {
                if (ImGui::Button((Icon::Play() + " Resume").c_str())) {
                    playbackSystem->Resume(selectedEntityID);
                }
            }
            else {
                if (ImGui::Button((Icon::Play() + " Play").c_str())) {
                    playbackSystem->Play(selectedEntityID, audioComp.looping);
                }
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID(301);
            if (ImGui::Button(Icon::Stop().c_str())) {
                playbackSystem->Stop(selectedEntityID);
                playbackProgress = 0.0f;
                m_sliderValue = 0.0f;
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID(302);
            if (ImGui::Checkbox((Icon::Loop() + " Loop").c_str(), &audioComp.looping)) {
                // Toggle loop
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::Text("Vol:");
            ImGui::SameLine();

            float volumePercent = audioComp.volume * 100.0f;
            ImGui::PushItemWidth(100.0f);
            if (ImGui::SliderFloat("##VolumeSlider", &volumePercent, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
                float newVolume = volumePercent / 100.0f;
                playbackSystem->SetVolume(selectedEntityID, newVolume);
                audioComp.volume = newVolume;
            }
            ImGui::PopItemWidth();

            double currentTime = playbackSystem->GetCurrentPosition(selectedEntityID);
            double duration = playbackSystem->GetDuration(selectedEntityID);

            m_sliderValue = playbackProgress;

            ImGui::Text("Progress:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);

            ImGui::PushID(303);
            if (ImGui::SliderFloat("##SeekSlider", &m_sliderValue, 0.0f, 1.0f, "%.1f%%")) {
                playbackSystem->Seek(selectedEntityID, m_sliderValue * duration);
                playbackProgress = m_sliderValue;
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

        if (waveformData.empty()) {
            UpdateWaveformData();
        }

        if (waveformData.empty()) {
            ImGui::Text("No waveform data available.");
            return;
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float width = avail.x;
        float height = std::min(avail.y * 0.6f, 200.0f);

        if (width <= 0 || height <= 0) return;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
            IM_COL32(20, 20, 30, 200));

        float centerY = pos.y + height * 0.5f;
        float halfHeight = height * 0.4f;

        size_t totalSamples = waveformData.size();
        size_t samplesToShow = static_cast<size_t>(width);

        if (samplesToShow > totalSamples) samplesToShow = totalSamples;
        if (samplesToShow == 0) return;

        float step = static_cast<float>(totalSamples) / static_cast<float>(samplesToShow);
        size_t currentSample = static_cast<size_t>(playbackProgress * totalSamples);

        for (size_t i = 0; i < samplesToShow; ++i) {
            size_t sampleIndex = static_cast<size_t>(i * step);
            if (sampleIndex >= totalSamples) break;

            float x = pos.x + (static_cast<float>(i) / static_cast<float>(samplesToShow)) * width;
            float sample = waveformData[sampleIndex];
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

        if (playbackProgress > 0.0f && playbackProgress < 1.0f) {
            float playheadX = pos.x + playbackProgress * width;
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
                auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
                if (playbackSystem) {
                    double duration = playbackSystem->GetDuration(selectedEntityID);
                    playbackSystem->Seek(selectedEntityID, relativeX * duration);
                    playbackProgress = relativeX;
                    m_sliderValue = relativeX;
                }
            }
        }

        ImGui::Dummy(ImVec2(0, height + 4));
    }

    void AudioView::UpdateWaveformData() {
        if (!m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            waveformData.clear();
            return;
        }

        const auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);

        if (audioComp.pcmData.empty()) {
            waveformData.clear();
            return;
        }

        const float* data = audioComp.pcmData.data();
        size_t totalSamples = audioComp.pcmData.size() / audioComp.channels;
        int channels = audioComp.channels;

        size_t targetSize = 4096;
        waveformData.resize(targetSize);

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
                waveformData[i] = peak;
            }
        }
    }

    void AudioView::LoadMedia(const std::vector<std::string>& filePaths) {
        std::cout << "[AudioView] Loading " << filePaths.size() << " audio files..." << std::endl;

        auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        if (!audioSystem) {
            std::cerr << "[AudioView] Error: AudioSystem not found!" << std::endl;
            return;
        }

        try {
            for (const auto& filePath : filePaths) {
                if (filePath.empty()) continue;

                ECS::EntityID entity = m_entityManager.AddNewEntity();
                m_entityManager.AddComponent<ECS::AudioComponent>(entity);
                audioSystem->SetAudio(entity, filePath);

                std::cout << "[AudioView] Started loading: " << filePath << " (Entity: " << entity << ")" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[AudioView] Exception loading audio: " << e.what() << std::endl;
        }
    }

    void AudioView::SaveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID) ||
            !m_entityManager.HasComponent<ECS::AudioComponent>(selectedEntityID)) {
            return;
        }

        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
        if (audioComp.pcmData.empty()) {
            std::cerr << "[AudioView] No audio data to save." << std::endl;
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
            return;
        }

        auto& audioComp = m_entityManager.GetComponent<ECS::AudioComponent>(selectedEntityID);
        if (audioComp.pcmData.empty()) {
            return;
        }

        int channels = audioComp.channels;
        int sampleRate = audioComp.sampleRate;
        const float* data = audioComp.pcmData.data();
        size_t totalSamples = audioComp.pcmData.size();

        FILE* f = fopen(filePath.c_str(), "wb");
        if (!f) {
            std::cerr << "[AudioView] Failed to open file for writing: " << filePath << std::endl;
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
        std::cout << "[AudioView] Saved audio to: " << filePath << std::endl;
    }

    void AudioView::RemoveSelectedMedia() {
        if (selectedEntityID == 0 || !m_entityManager.IsEntityValid(selectedEntityID)) return;

        try {
            auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
            if (playbackSystem) {
                playbackSystem->Stop(selectedEntityID);
            }

            auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
            if (audioSystem) {
                audioSystem->RemoveAudio(selectedEntityID);
            }

            selectedEntityID = 0;
            index = 0;
            waveformData.clear();
            playbackProgress = 0.0f;
            m_sliderValue = 0.0f;
            RefreshEntities();

            std::cout << "[AudioView] Audio removed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[AudioView] Exception removing audio: " << e.what() << std::endl;
        }
    }

    void AudioView::PauseAllAudio() {
        try {
            auto playbackSystem = m_entityManager.GetSystem<ECS::AudioPlaybackSystem>();
            if (!playbackSystem) return;

            for (auto entityID : mediaEntities) {
                if (m_entityManager.IsEntityValid(entityID) &&
                    m_entityManager.HasComponent<ECS::AudioComponent>(entityID)) {
                    if (playbackSystem->IsPlaying(entityID)) {
                        playbackSystem->Pause(entityID);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[AudioView] Exception pausing audio: " << e.what() << std::endl;
        }
    }

    void AudioView::OnMediaAdded(ECS::EntityID entity) {
        RefreshEntities();
    }

    void AudioView::OnMediaRemoved(ECS::EntityID entity) {
        RefreshEntities();
        UpdateSelectionAfterRemoval(entity);
        if (selectedEntityID == 0) {
            waveformData.clear();
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

}