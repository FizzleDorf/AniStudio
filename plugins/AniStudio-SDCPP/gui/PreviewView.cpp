// PreviewView.cpp
#include "PreviewView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "SDCPPSettingsComponent.hpp"
#include "SettingsSystem.hpp"
#include "TextureSystem.hpp"
#include "IconFonts.hpp"
#include "Log.hpp"
#include "ECS.h"
#include <cstdint>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace GUI {

    PreviewView::PreviewView(ECS::EntityManager& entityMgr, ViewManager& viewMgr)
        : BaseView(entityMgr, viewMgr) {
        viewName = "PreviewView";
    }

    PreviewView::~PreviewView() {
        if (m_showVideoCallback) {
            ANI::Events::Ref().UnregisterEventWithData(
                "ShowVideoInPreview", m_showVideoCallback);
            m_showVideoCallback = nullptr;
        }
        if (auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>()) {
            sys->UnregisterCallbacksForOwner(this);
        }
        if (m_textureSystem && m_entityManager.IsEntityValid(m_previewEntity)) {
            m_textureSystem->RemoveTexture(m_previewEntity);
        }
        if (m_textureSystem && m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            m_textureSystem->RemoveTexture(m_videoPreviewEntity);
        }
        if (m_previewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_previewEntity)) {
            m_entityManager.DestroyEntity(m_previewEntity);
        }
        if (m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            m_entityManager.DestroyEntity(m_videoPreviewEntity);
        }
        m_previewEntity = 0;
        m_videoPreviewEntity = 0;
        m_previewEntityHasComponent = false;
        m_videoPreviewEntityHasComponent = false;
    }

    int PreviewView::PreviewModeToInt(PreviewMode m) {
        return static_cast<int>(m);
    }

    void PreviewView::ApplyModeToLibrary() {
        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (sys && sys->HasPendingWork()) {
            ANI_LOG_WARN("[PreviewView] ApplyModeToLibrary deferred: task in flight");
            return;
        }
        DiffusionCallbackUtils::SetPreviewMode(
            PreviewModeToInt(m_mode), m_interval);
    }

    void PreviewView::Init() {
        LoadModeFromSettings();
        ApplyModeToLibrary();
        m_lastSequence = DiffusionCallbackUtils::GetPreviewSequence();
        m_lastBatchSequence = DiffusionCallbackUtils::GetBatchSequence();
        m_lastBatchFrameSeq = 0;

        m_textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
        m_videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();

        m_previewEntity = m_entityManager.AddNewEntity();
        if (m_entityManager.IsEntityValid(m_previewEntity)) {
            if (!m_entityManager.HasComponent<ECS::PreviewImageComponent>(m_previewEntity)) {
                m_entityManager.AddComponent<ECS::PreviewImageComponent>(m_previewEntity);
            }
            if (!m_entityManager.HasComponent<ECS::TextureComponent>(m_previewEntity)) {
                m_entityManager.AddComponent<ECS::TextureComponent>(m_previewEntity);
            }
            m_previewEntityHasComponent = true;
        }

        m_videoPreviewEntity = m_entityManager.AddNewEntity();
        if (m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            m_entityManager.AddComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            m_entityManager.AddComponent<ECS::TextureComponent>(m_videoPreviewEntity);
            m_entityManager.AddComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
            auto& st = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
            st.mode = ECS::PlaybackMode::Streaming;
            m_videoPreviewEntityHasComponent = true;
        }

        m_showVideoCallback = [this](const std::any& data) {
            OnShowVideoInPreview(data);
            };
        ANI::Events::Ref().RegisterEventWithData(
            "ShowVideoInPreview", m_showVideoCallback);

        if (auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>()) {
            sys->RegisterTaskCompletedCallback(
                this,
                [this](ECS::SDCPPSystem::TaskType type,
                    ECS::EntityID entityID,
                    const std::string& fullPath) {
                        OnSdcppTaskCompleted(type, entityID, fullPath);
                });
        }

        if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) {
            av->RegisterTrackStateCallback(
                [this](ECS::EntityID entity, ECS::PlaybackState state) {
                    if (entity != m_videoPreviewEntity) return;
                    if (state != ECS::PlaybackState::EndOfStream) return;
                    if (!m_entityManager.IsEntityValid(entity)) return;
                    if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) return;
                    auto& st = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
                    if (st.looping) {
                        SeekVideo(entity, 0.0);
                        PlayVideo(entity);
                    }
                    else if (m_videoAutoplay) {
                        SeekVideo(entity, 0.0);
                        PlayVideo(entity);
                    }
                    else {
                        m_playbackProgress = 0.0f;
                    }
                });
        }
    }

    void PreviewView::OnShowVideoInPreview(const std::any& data) {
        try {
            auto m = std::any_cast<
                std::unordered_map<std::string, std::any>>(data);
            std::string path = std::any_cast<std::string>(m.at("filePath"));
            std::string label = m.count("label")
                ? std::any_cast<std::string>(m.at("label")) : std::string("preview");
            bool loop = m.count("loop")
                ? std::any_cast<bool>(m.at("loop")) : true;
            ANI_LOG_INFO("[PreviewView] ShowVideoInPreview path=%s label=%s",
                path.c_str(), label.c_str());
            ShowVideo(path, label, loop);
        }
        catch (const std::exception& ex) {
            ANI_LOG_WARN("[PreviewView] ShowVideoInPreview: %s", ex.what());
        }
    }

    void PreviewView::OnSdcppTaskCompleted(ECS::SDCPPSystem::TaskType type,
        ECS::EntityID entityID,
        const std::string& fullPath) {
        using TT = ECS::SDCPPSystem::TaskType;
        if (type != TT::Img2Vid && type != TT::Edit) {
            return;
        }
        ANI_LOG_INFO("[PreviewView] SDCPP video task completed: entity=%u path=%s",
            entityID, fullPath.c_str());
        ShowVideo(fullPath, std::string("diffusion"), true);
    }

    void PreviewView::Update(const float deltaT) {
        (void)deltaT;

        if (m_videoAttached &&
            m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity) &&
            m_entityManager.HasComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity)) {
            auto& st = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
            if (st.duration > 0.0) {
                m_playbackProgress = static_cast<float>(st.currentTime / st.duration);
                if (m_playbackProgress < 0) m_playbackProgress = 0;
                if (m_playbackProgress > 1) m_playbackProgress = 1;
            }
        }

        if (m_videoAttached &&
            m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            auto av = m_entityManager.GetSystem<ECS::AVSystem>();
            if (av) {
                const auto state = av->GetState(m_videoPreviewEntity);
                const bool ready = av->IsMediaReady(m_videoPreviewEntity);
                if (ready && state != ECS::PlaybackState::Playing &&
                    state != ECS::PlaybackState::Paused &&
                    state != ECS::PlaybackState::EndOfStream) {
                    av->SetLooping(m_videoPreviewEntity, m_videoLoops);
                    av->Play(m_videoPreviewEntity);
                }
            }
        }
    }

    void PreviewView::LoadModeFromSettings() {
        auto settingsSys = m_entityManager.GetSystem<ECS::SettingsSystem>();
        if (!settingsSys) return;
        ECS::EntityID e = settingsSys->GetSettingsEntity();
        if (!m_entityManager.IsEntityValid(e) ||
            !m_entityManager.HasComponent<ECS::SDCPPSettingsComponent>(e)) return;
        auto& sdcpp = m_entityManager.GetComponent<ECS::SDCPPSettingsComponent>(e);
        if (sdcpp.preview_mode >= 0 && sdcpp.preview_mode <= 3) {
            m_mode = static_cast<PreviewMode>(sdcpp.preview_mode);
        }
        m_interval = sdcpp.preview_interval;
        m_modeLoaded = true;
    }

    void PreviewView::SaveModeToSettings() {
        auto settingsSys = m_entityManager.GetSystem<ECS::SettingsSystem>();
        if (!settingsSys) return;
        ECS::EntityID e = settingsSys->GetSettingsEntity();
        if (!m_entityManager.IsEntityValid(e) ||
            !m_entityManager.HasComponent<ECS::SDCPPSettingsComponent>(e)) return;
        auto& sdcpp = m_entityManager.GetComponent<ECS::SDCPPSettingsComponent>(e);
        sdcpp.preview_mode = PreviewModeToInt(m_mode);
        sdcpp.preview_interval = m_interval;
    }

    void PreviewView::PushPreviewFrameToComponent(const PreviewFrame& frame) {
        if (!m_previewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_previewEntity)) return;
        if (!frame.valid()) return;

        if (!m_entityManager.HasComponent<ECS::PreviewImageComponent>(m_previewEntity)) {
            m_entityManager.AddComponent<ECS::PreviewImageComponent>(m_previewEntity);
        }

        auto& preview = m_entityManager.GetComponent<ECS::PreviewImageComponent>(m_previewEntity);

        const size_t byteCount =
            static_cast<size_t>(frame.width) *
            static_cast<size_t>(frame.height) *
            static_cast<size_t>(frame.channels);

        auto buf = std::shared_ptr<unsigned char[]>(new unsigned char[byteCount]);
        std::memcpy(buf.get(), frame.data.get(), byteCount);

        preview.imageDataPtr = std::move(buf);
        preview.imageData = preview.imageDataPtr.get();
        preview.width = frame.width;
        preview.height = frame.height;
        preview.channels = frame.channels;
        preview.sourceName = "diffusion";

        if (!m_textureSystem) {
            m_textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
        }
        if (m_textureSystem) {
            unsigned char* upload = static_cast<unsigned char*>(std::malloc(byteCount));
            if (upload) {
                std::memcpy(upload, preview.imageData, byteCount);
                m_textureSystem->QueueTextureCreation(
                    m_previewEntity, upload,
                    preview.width, preview.height, preview.channels);
            }
        }
    }

    void PreviewView::RefreshPreviewTextureIfNeeded(float deltaT) {
        if (!m_previewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_previewEntity)) return;

        PreviewBatch batch = DiffusionCallbackUtils::GetLatestBatch();
        const uint64_t batchSeq = DiffusionCallbackUtils::GetBatchSequence();

        if (batch.valid() && !batch.frames.empty()) {
            if (batchSeq != m_lastBatchSequence) {
                m_lastBatchSequence = batchSeq;
                m_liveFrameIndex = 0;
                m_liveFrameTimer = 0.0;
                m_livePaused = false;
            }

            if (m_liveFrameIndex < 0 ||
                m_liveFrameIndex >= static_cast<int>(batch.frames.size())) {
                m_liveFrameIndex = 0;
            }

            if (!m_livePaused) {
                m_liveFrameTimer += deltaT;
                if (m_liveFrameTimer >= m_liveFrameDuration) {
                    m_liveFrameTimer = 0.0;
                    m_liveFrameIndex = (m_liveFrameIndex + 1) % static_cast<int>(batch.frames.size());
                }
            }

            if (m_liveFrameIndex < 0 ||
                m_liveFrameIndex >= static_cast<int>(batch.frames.size())) {
                m_liveFrameIndex = 0;
            }

            const PreviewFrame& frame = batch.frames[m_liveFrameIndex];
            if (!frame.valid()) return;
            if (frame.sequence == m_lastBatchFrameSeq) return;
            m_lastBatchFrameSeq = frame.sequence;
            PushPreviewFrameToComponent(frame);
            return;
        }

        const uint64_t seq = DiffusionCallbackUtils::GetPreviewSequence();
        if (seq == m_lastSequence) return;
        PreviewFrame frame = DiffusionCallbackUtils::GetLatestPreview();
        m_lastSequence = seq;
        if (!frame.valid()) return;
        if (frame.channels != 1 && frame.channels != 3 && frame.channels != 4) return;
        PushPreviewFrameToComponent(frame);
    }

    void PreviewView::RefreshVideoTextureIfNeeded() {
        if (!m_videoAttached) return;
        if (!m_videoPreviewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;

        if (!m_videoSystem) {
            m_videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
            if (!m_videoSystem) return;
        }

        std::vector<uint8_t> rgba;
        int w = 0, h = 0;
        if (!m_videoSystem->GetCurrentFrame(m_videoPreviewEntity, rgba, w, h)) return;
        if (rgba.empty() || w <= 0 || h <= 0) return;

        const size_t fpBytes = std::min<size_t>(rgba.size(), 64);
        uint64_t fp = 1469598103934665603ull;
        for (size_t i = 0; i < fpBytes; ++i) {
            fp = (fp ^ rgba[i]) * 1099511628211ull;
        }

        if (w == m_lastVideoWidth && h == m_lastVideoHeight &&
            fp == m_lastVideoFingerprint) {
            return;
        }
        m_lastVideoWidth = w;
        m_lastVideoHeight = h;
        m_lastVideoFingerprint = fp;

        PushVideoFrameToComponent(rgba, w, h);
    }

    void PreviewView::PushVideoFrameToComponent(const std::vector<uint8_t>& rgba,
        int w, int h) {
        if (!m_videoPreviewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;
        if (!m_textureSystem) {
            m_textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
        }
        if (!m_textureSystem) return;

        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            pvc.width = w;
            pvc.height = h;
        }

        const size_t byteCount = rgba.size();
        unsigned char* upload = static_cast<unsigned char*>(std::malloc(byteCount));
        if (!upload) return;
        std::memcpy(upload, rgba.data(), byteCount);

        m_textureSystem->QueueTextureCreation(
            m_videoPreviewEntity, upload, w, h, 4);
    }

    void PreviewView::ShowVideo(const std::string& path,
        const std::string& label,
        bool loop) {
        if (!m_videoPreviewEntityHasComponent ||
            !m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            ANI_LOG_WARN("[PreviewView] ShowVideo: video preview entity unavailable");
            return;
        }

        auto av = m_entityManager.GetSystem<ECS::AVSystem>();
        if (!av) {
            ANI_LOG_WARN("[PreviewView] ShowVideo: AVSystem unavailable");
            return;
        }

        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            pvc.sourceName = label;
        }

        if (!av->AttachMedia(m_videoPreviewEntity, path,
            ECS::TrackType::Video,
            ECS::PlaybackMode::Streaming)) {
            ANI_LOG_WARN("[PreviewView] ShowVideo: AttachMedia failed for %s", path.c_str());
            m_videoAttached = false;
            return;
        }

        m_videoLoops = loop;
        av->SetLooping(m_videoPreviewEntity, loop);

        if (av->IsMediaReady(m_videoPreviewEntity)) {
            av->Play(m_videoPreviewEntity);
        }

        m_videoAttached = true;
        m_lastVideoWidth = 0;
        m_lastVideoHeight = 0;
        m_lastVideoFingerprint = 0;
        m_playbackProgress = 0.0f;

        ANI_LOG_INFO("[PreviewView] ShowVideo: attached '%s' (loop=%d)",
            path.c_str(), loop ? 1 : 0);
    }

    void PreviewView::ClearVideo() {
        if (!m_videoPreviewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;

        auto av = m_entityManager.GetSystem<ECS::AVSystem>();
        if (av) av->Stop(m_videoPreviewEntity);
        if (m_textureSystem) m_textureSystem->RemoveTexture(m_videoPreviewEntity);
        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            pvc.Unload();
        }
        m_videoAttached = false;
        m_lastVideoWidth = 0;
        m_lastVideoHeight = 0;
        m_lastVideoFingerprint = 0;
        m_playbackProgress = 0.0f;
    }

    bool PreviewView::HasAudioTrack(ECS::EntityID entity) const {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return false;
        return m_entityManager.HasComponent<ECS::AudioComponent>(entity);
    }

    void PreviewView::SeekVideo(ECS::EntityID entity, double time) {
        if (entity == 0 || !m_entityManager.IsEntityValid(entity)) return;
        if (m_isSeeking) {
            m_pendingSeek = true;
            m_pendingSeekTime = time;
            m_pendingSeekEntity = entity;
            return;
        }
        m_isSeeking = true;
        try {
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
                auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
                state.currentTime = time;
            }
            if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) {
                av->Seek(entity, time);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[PreviewView] Seek error: %s", e.what());
        }
        m_isSeeking = false;
        if (m_pendingSeek) {
            m_pendingSeek = false;
            SeekVideo(m_pendingSeekEntity, m_pendingSeekTime);
        }
    }

    void PreviewView::PlayVideo(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) return;
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) return;
        if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) av->Play(entity);
    }
    void PreviewView::PauseVideo(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) return;
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) return;
        if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) av->Pause(entity);
    }
    void PreviewView::StopVideo(ECS::EntityID entity) {
        if (!m_entityManager.IsEntityValid(entity)) return;
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) return;
        if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) av->Stop(entity);
        m_playbackProgress = 0.0f;
    }
    void PreviewView::SetVideoSpeed(ECS::EntityID entity, float speed) {
        if (!m_entityManager.IsEntityValid(entity)) return;
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) return;
        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.speed = speed;
        if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) av->SetSpeed(entity, speed);
    }
    void PreviewView::SetVideoVolume(ECS::EntityID entity, float volume) {
        if (!m_entityManager.IsEntityValid(entity)) return;
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) return;
        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity);
        state.volume = volume;
        if (auto av = m_entityManager.GetSystem<ECS::AVSystem>()) av->SetVolume(entity, volume);
    }

    std::string PreviewView::FormatTimecode(double seconds) const {
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

    void PreviewView::RenderAudioControls(ECS::EntityID entity) {
        if (!HasAudioTrack(entity)) return;
        float volume = 1.0f;
        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(entity)) {
            volume = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(entity).volume;
        }
        if (volume <= 0.0f)       ImGui::Text("%s", Icon::VolumeOff().c_str());
        else if (volume < 0.33f)  ImGui::Text("%s", Icon::VolumeLow().c_str());
        else if (volume < 0.66f)  ImGui::Text("%s", Icon::VolumeMedium().c_str());
        else                      ImGui::Text("%s", Icon::VolumeHigh().c_str());
        ImGui::SameLine();
        bool audioEnabled = (volume > 0.0f);
        ImGui::PushID(400);
        if (ImGui::Checkbox("##AudioEnabled", &audioEnabled)) {
            SetVideoVolume(entity, audioEnabled ? 1.0f : 0.0f);
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushItemWidth(70.0f);
        float volumePercent = volume * 100.0f;
        ImGui::PushID(401);
        if (ImGui::SliderFloat("##AudioVolume", &volumePercent, 0.0f, 100.0f,
            "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
            SetVideoVolume(entity, volumePercent / 100.0f);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();
    }

    void PreviewView::RenderTimelineControls() {
        if (!m_videoPreviewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;
        if (!m_entityManager.HasComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity)) return;

        auto& state = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
        std::string currentStr = FormatTimecode(state.currentTime);
        std::string totalStr = FormatTimecode(state.duration);

        float availWidth = ImGui::GetContentRegionAvail().x;
        float sliderWidth = availWidth - 20.0f;
        if (sliderWidth < 50.0f) sliderWidth = 50.0f;
        float startX = (availWidth - sliderWidth) * 0.5f;
        if (startX < 0) startX = 0;
        ImGui::SetCursorPosX(startX);

        ImGui::PushItemWidth(sliderWidth);
        ImGui::PushID("PreviewTimeline");
        if (ImGui::SliderFloat("##Timeline", &m_playbackProgress, 0.0f, 1.0f, "")) {
            m_pendingSeek = true;
            m_pendingSeekEntity = m_videoPreviewEntity;
            m_pendingSeekTime = static_cast<double>(m_playbackProgress) * state.duration;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && m_pendingSeek) {
            m_pendingSeek = false;
            SeekVideo(m_pendingSeekEntity, m_pendingSeekTime);
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

    void PreviewView::DrawMenuBar() {
        if (!ImGui::BeginMenuBar()) return;
        if (ImGui::BeginMenu("Preview")) {
            if (ImGui::MenuItem("None", nullptr, m_mode == PreviewMode::None)) {
                m_mode = PreviewMode::None;
                SaveModeToSettings();
                ApplyModeToLibrary();
                DiffusionCallbackUtils::ClearPreview();
                DiffusionCallbackUtils::ClearBatch();
                m_lastSequence = DiffusionCallbackUtils::GetPreviewSequence();
                m_lastBatchSequence = DiffusionCallbackUtils::GetBatchSequence();
            }
            if (ImGui::MenuItem("Proj (latent)", nullptr, m_mode == PreviewMode::Proj)) {
                m_mode = PreviewMode::Proj; SaveModeToSettings(); ApplyModeToLibrary();
            }
            if (ImGui::MenuItem("TAE (fast VAE)", nullptr, m_mode == PreviewMode::Tae)) {
                m_mode = PreviewMode::Tae; SaveModeToSettings(); ApplyModeToLibrary();
            }
            if (ImGui::MenuItem("VAE (full)", nullptr, m_mode == PreviewMode::Vae)) {
                m_mode = PreviewMode::Vae; SaveModeToSettings(); ApplyModeToLibrary();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear")) {
            DiffusionCallbackUtils::ClearPreview();
            DiffusionCallbackUtils::ClearBatch();
            m_lastSequence = DiffusionCallbackUtils::GetPreviewSequence();
            m_lastBatchSequence = DiffusionCallbackUtils::GetBatchSequence();
            if (m_previewEntityHasComponent &&
                m_entityManager.IsEntityValid(m_previewEntity)) {
                if (m_entityManager.HasComponent<ECS::PreviewImageComponent>(m_previewEntity)) {
                    auto& c = m_entityManager.GetComponent<ECS::PreviewImageComponent>(m_previewEntity);
                    c.ClearImageData();
                }
                if (m_textureSystem) m_textureSystem->RemoveTexture(m_previewEntity);
            }
            ClearVideo();
        }

        const auto prog = DiffusionCallbackUtils::GetProgressSnapshot();
        std::string status;
        if (prog.isProcessing && prog.totalSteps > 0) {
            status = "Step " + std::to_string(prog.currentStep) + "/" +
                std::to_string(prog.totalSteps);
            if (prog.currentTime > 0.0f) {
                char buf[64];
                snprintf(buf, sizeof(buf), " | %.2fs/step", prog.currentTime);
                status += buf;
            }
        }
        else {
            status = "Idle";
        }
        float avail = ImGui::GetContentRegionAvail().x;
        float tw = ImGui::CalcTextSize(status.c_str()).x;
        if (avail > tw) ImGui::SameLine(ImGui::GetWindowWidth() - tw - 16.0f);
        else ImGui::SameLine();
        ImGui::TextUnformatted(status.c_str());
        ImGui::EndMenuBar();
    }

    void PreviewView::DrawLivePreviewControls() {
        const auto prog = DiffusionCallbackUtils::GetProgressSnapshot();
        const uint64_t batchSeq = DiffusionCallbackUtils::GetBatchSequence();
        PreviewBatch batch = DiffusionCallbackUtils::GetLatestBatch();

        const bool isVideoBatch = batch.valid() && batch.frames.size() > 1;

        ImGui::TextUnformatted(isVideoBatch ? "Live video preview"
            : "Live image preview");
        ImGui::SameLine();
        if (prog.isProcessing) {
            ImGui::TextDisabled("(generating: step %d/%d)",
                prog.currentStep, prog.totalSteps);
        }
        else {
            ImGui::TextDisabled("(idle)");
        }

        if (batch.valid() && !batch.frames.empty()) {
            const int frameCount = static_cast<int>(batch.frames.size());
            const int curFrame = (frameCount > 0) ? (m_liveFrameIndex % frameCount) : 0;
            ImGui::Text("Batch %llu  |  Step %d  |  Frame %d / %d",
                static_cast<unsigned long long>(batchSeq),
                batch.step, curFrame + 1, frameCount);
        }
        else {
            ImGui::TextDisabled("Waiting for first preview frame...");
        }

        if (!batch.valid() || batch.frames.empty()) {
            if (prog.isProcessing && prog.totalSteps > 0) {
                float frac = static_cast<float>(prog.currentStep) /
                    static_cast<float>(prog.totalSteps);
                frac = std::clamp(frac, 0.0f, 1.0f);
                ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f));
            }
            return;
        }

        ImGui::BeginDisabled(batch.frames.size() <= 1);

        ImGui::PushID(600);
        if (ImGui::Button(Icon::ArrowFirst().c_str())) {
            m_liveFrameIndex = 0;
            m_liveFrameTimer = 0.0;
            m_livePaused = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("First frame of current step");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(601);
        if (ImGui::Button(Icon::ArrowLeft().c_str())) {
            m_liveFrameIndex--;
            if (m_liveFrameIndex < 0)
                m_liveFrameIndex = static_cast<int>(batch.frames.size()) - 1;
            m_liveFrameTimer = 0.0;
            m_livePaused = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Previous frame");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(602);
        if (ImGui::Button(m_livePaused ? Icon::Play().c_str() : Icon::Pause().c_str())) {
            m_livePaused = !m_livePaused;
            m_liveFrameTimer = 0.0;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(m_livePaused
            ? "Resume cycling through this step"
            : "Pause cycling");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(603);
        if (ImGui::Button(Icon::ArrowRight().c_str())) {
            m_liveFrameIndex = (m_liveFrameIndex + 1) %
                static_cast<int>(batch.frames.size());
            m_liveFrameTimer = 0.0;
            m_livePaused = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Next frame");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(604);
        if (ImGui::Button(Icon::ArrowLast().c_str())) {
            m_liveFrameIndex = static_cast<int>(batch.frames.size()) - 1;
            m_liveFrameTimer = 0.0;
            m_livePaused = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Last frame of current step");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::Text("%s", Icon::ClockRotateLeft().c_str());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Preview cycle speed");
        ImGui::SameLine();
        ImGui::PushItemWidth(90.0f);
        float fps = (m_liveFrameDuration > 0.0)
            ? static_cast<float>(1.0 / m_liveFrameDuration) : 16.0f;
        ImGui::PushID(605);
        if (ImGui::DragFloat("##LiveFPS", &fps, 0.5f, 1.0f, 60.0f, "%.0f fps")) {
            fps = std::clamp(fps, 1.0f, 60.0f);
            m_liveFrameDuration = 1.0 / static_cast<double>(fps);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::EndDisabled();

        if (batch.frames.size() > 1) {
            float maxFrame = static_cast<float>(batch.frames.size() - 1);
            float framePos = static_cast<float>(m_liveFrameIndex);
            ImGui::PushItemWidth(-1.0f);
            ImGui::PushID(606);
            if (ImGui::SliderFloat("##LiveBatchScrub", &framePos,
                0.0f, maxFrame, "frame %.0f")) {
                m_liveFrameIndex = static_cast<int>(framePos + 0.5f);
                if (m_liveFrameIndex < 0) m_liveFrameIndex = 0;
                if (m_liveFrameIndex >= (int)batch.frames.size())
                    m_liveFrameIndex = static_cast<int>(batch.frames.size()) - 1;
                m_liveFrameTimer = 0.0;
                m_livePaused = true;
                const PreviewFrame& f = batch.frames[m_liveFrameIndex];
                if (f.valid()) {
                    m_lastBatchFrameSeq = f.sequence;
                    PushPreviewFrameToComponent(f);
                }
            }
            ImGui::PopID();
            ImGui::PopItemWidth();
        }

        if (prog.isProcessing && prog.totalSteps > 0) {
            float frac = static_cast<float>(prog.currentStep) /
                static_cast<float>(prog.totalSteps);
            frac = std::clamp(frac, 0.0f, 1.0f);
            ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f));
        }
    }

    void PreviewView::DrawVideoControls() {
        if (!m_videoPreviewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;

        auto av = m_entityManager.GetSystem<ECS::AVSystem>();
        if (!av) return;

        const bool ready = av->IsMediaReady(m_videoPreviewEntity);
        const auto state = av->GetState(m_videoPreviewEntity);
        const bool playing = (state == ECS::PlaybackState::Playing);
        const bool paused = (state == ECS::PlaybackState::Paused);

        bool hasVideo = false;
        double fps = 30.0;
        if (m_entityManager.HasComponent<ECS::VideoComponent>(m_videoPreviewEntity)) {
            auto& vc = m_entityManager.GetComponent<ECS::VideoComponent>(m_videoPreviewEntity);
            hasVideo = vc.width > 0 && vc.height > 0;
            if (vc.fps > 0.0) fps = vc.fps;
        }

        std::string label = "no video";
        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            if (!pvc.sourceName.empty()) label = pvc.sourceName;
        }

        ImGui::Text("%s", label.c_str());
        ImGui::SameLine();
        if (!m_videoAttached)   ImGui::TextDisabled("(no video loaded)");
        else if (!ready)        ImGui::TextDisabled("(loading...)");
        else if (playing)       ImGui::TextDisabled("(playing)");
        else if (paused)        ImGui::TextDisabled("(paused)");
        else                    ImGui::TextDisabled("(stopped)");

        ImGui::BeginDisabled(!ready || !hasVideo);

        float speed = 1.0f;
        if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity)) {
            speed = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity).speed;
        }
        ImGui::Text("%s", Icon::ClockRotateLeft().c_str());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Playback Speed");
        ImGui::SameLine();
        ImGui::PushItemWidth(70.0f);
        ImGui::PushID(305);
        if (ImGui::DragFloat("##Speed", &speed, 0.05f, 0.1f, 4.0f, "%.1fx")) {
            speed = std::clamp(speed, 0.1f, 4.0f);
            SetVideoSpeed(m_videoPreviewEntity, speed);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();
        ImGui::SameLine();

        if (HasAudioTrack(m_videoPreviewEntity)) {
            RenderAudioControls(m_videoPreviewEntity);
            ImGui::SameLine();
        }

        ImGui::PushID(308);
        if (ImGui::Button(Icon::ArrowFirst().c_str())) SeekVideo(m_videoPreviewEntity, 0.0);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("First frame");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(302);
        bool held = false;
        ImGui::Button(Icon::ArrowLeft().c_str());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Previous frame (hold for repeat)");
        ImGuiID prevID = ImGui::GetItemID();
        if (m_repeatButtonHandler.Process(prevID, 0.4, 0.1, held)) {
            float step = 1.0f / (float)fps;
            auto& states = m_repeatButtonHandler.GetAllStates();
            auto it = states.find(prevID);
            if (it != states.end()) {
                int count = it->second.repeatCount;
                if (count > 10) step *= 30.0f;
                else if (count > 5) step *= 10.0f;
            }
            double current = 0.0;
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity)) {
                current = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity).currentTime;
            }
            double newTime = current - step;
            if (newTime < 0.0) newTime = 0.0;
            SeekVideo(m_videoPreviewEntity, newTime);
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(300);
        if (ImGui::Button(playing ? Icon::Pause().c_str() : Icon::Play().c_str())) {
            if (playing) PauseVideo(m_videoPreviewEntity);
            else         PlayVideo(m_videoPreviewEntity);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(playing ? "Pause" : "Play");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(301);
        if (ImGui::Button(Icon::Stop().c_str())) StopVideo(m_videoPreviewEntity);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(303);
        ImGui::Button(Icon::ArrowRight().c_str());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Next frame (hold for repeat)");
        ImGuiID nextID = ImGui::GetItemID();
        if (m_repeatButtonHandler.Process(nextID, 0.4, 0.1, held)) {
            float step = 1.0f / (float)fps;
            auto& states = m_repeatButtonHandler.GetAllStates();
            auto it = states.find(nextID);
            if (it != states.end()) {
                int count = it->second.repeatCount;
                if (count > 10) step *= 30.0f;
                else if (count > 5) step *= 10.0f;
            }
            double current = 0.0, duration = 0.0;
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity)) {
                auto& state2 = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
                current = state2.currentTime;
                duration = state2.duration;
            }
            double newTime = current + step;
            if (newTime > duration) newTime = duration;
            SeekVideo(m_videoPreviewEntity, newTime);
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(309);
        if (ImGui::Button(Icon::ArrowLast().c_str())) {
            if (m_entityManager.HasComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity)) {
                auto& state2 = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
                SeekVideo(m_videoPreviewEntity, state2.duration - 0.001);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Last frame");
        ImGui::PopID();
        ImGui::SameLine();

        bool loop = m_videoLoops;
        ImGui::PushID(310);
        if (ImGui::Checkbox((Icon::Loop() + " Loop").c_str(), &loop)) {
            m_videoLoops = loop;
            av->SetLooping(m_videoPreviewEntity, loop);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Loop video");
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(311);
        ImGui::Checkbox((Icon::Play() + " Autoplay").c_str(), &m_videoAutoplay);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto restart when video ends");
        ImGui::PopID();

        ImGui::EndDisabled();
        RenderTimelineControls();
    }

    void PreviewView::Render() {
        if (!m_modeLoaded) {
            LoadModeFromSettings();
            ApplyModeToLibrary();
        }

        auto sys = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        const bool generating = sys && sys->HasPendingWork();

        const bool showVideo = !generating &&
            m_videoAttached &&
            m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity);

        const bool livePreviewActive =
            generating &&
            m_mode != PreviewMode::None &&
            m_previewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_previewEntity);

        if (livePreviewActive) {
            RefreshPreviewTextureIfNeeded(ImGui::GetIO().DeltaTime);
        }
        else if (!showVideo && m_mode != PreviewMode::None) {
            RefreshPreviewTextureIfNeeded(ImGui::GetIO().DeltaTime);
        }
        if (showVideo) {
            RefreshVideoTextureIfNeeded();
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 150), ImVec2(FLT_MAX, FLT_MAX));
        if (!ImGui::Begin(GetWindowTitle().c_str(), &windowOpen,
            ImGuiWindowFlags_MenuBar)) {
            ImGui::End();
            return;
        }

        DrawMenuBar();

        if (!m_textureSystem) {
            m_textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
        }

        ECS::EntityID texEntity = showVideo ? m_videoPreviewEntity : m_previewEntity;

        if (generating) {
            DrawLivePreviewControls();
            ImGui::Separator();
        }
        else if (m_videoAttached &&
            m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            DrawVideoControls();
            ImGui::Separator();
        }

        if (generating && m_mode == PreviewMode::None) {
            ImGui::TextDisabled("Preview disabled (generating...).");
            ImGui::End();
            return;
        }

        int tw = 0, th = 0;
        if (m_entityManager.IsEntityValid(texEntity)) {
            if (showVideo &&
                m_entityManager.HasComponent<ECS::PreviewVideoComponent>(texEntity)) {
                auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(texEntity);
                tw = pvc.width;
                th = pvc.height;
            }
            else if (m_entityManager.HasComponent<ECS::PreviewImageComponent>(texEntity)) {
                auto& c = m_entityManager.GetComponent<ECS::PreviewImageComponent>(texEntity);
                tw = c.width;
                th = c.height;
            }
        }

        GLuint tex = m_textureSystem ? m_textureSystem->GetTextureID(texEntity) : 0;
        if (!m_textureSystem || !m_textureSystem->HasValidTexture(texEntity) ||
            tex == 0 || tw <= 0 || th <= 0 || !glIsTexture(tex)) {
            if (showVideo) {
                ImGui::TextUnformatted("Waiting for first video frame...");
            }
            else if (generating && m_mode != PreviewMode::None) {
                ImGui::TextUnformatted("Waiting for first preview frame...");
            }
            else if (m_videoAttached) {
                ImGui::TextDisabled("No video frame available yet.");
            }
            else if (m_mode == PreviewMode::None) {
                ImGui::TextDisabled("Preview disabled.");
            }
            else {
                ImGui::TextDisabled("No preview available.");
            }
            ImGui::End();
            return;
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x <= 0.0f || avail.y <= 0.0f) {
            ImGui::End();
            return;
        }

        const float srcAspect = (float)tw / (float)th;
        float drawW = avail.x;
        float drawH = drawW / srcAspect;
        if (drawH > avail.y) {
            drawH = avail.y;
            drawW = drawH * srcAspect;
        }

        ImVec2 pos = ImGui::GetCursorScreenPos();
        float offsetX = (avail.x - drawW) * 0.5f;
        float offsetY = (avail.y - drawH) * 0.5f;
        if (offsetX < 0.0f) offsetX = 0.0f;
        if (offsetY < 0.0f) offsetY = 0.0f;

        ImVec2 topLeft(pos.x + offsetX, pos.y + offsetY);
        ImVec2 bottomRight(pos.x + offsetX + drawW, pos.y + offsetY + drawH);

        if (tex != 0 && glIsTexture(tex)) {
            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)(intptr_t)tex,
                topLeft,
                bottomRight,
                ImVec2(0, 0), ImVec2(1, 1));
        }

        ImGui::Dummy(ImVec2(avail.x, avail.y));
        ImGui::End();
    }

    nlohmann::json PreviewView::Serialize() const {
        nlohmann::json j = BaseView::Serialize();
        j["previewMode"] = PreviewModeToInt(m_mode);
        j["previewInterval"] = m_interval;
        return j;
    }

    void PreviewView::Deserialize(const nlohmann::json& j) {
        BaseView::Deserialize(j);
        if (j.contains("previewMode")) {
            int m = j["previewMode"].get<int>();
            if (m >= 0 && m <= 3) m_mode = static_cast<PreviewMode>(m);
        }
        if (j.contains("previewInterval")) {
            m_interval = j["previewInterval"].get<int>();
        }
    }

}