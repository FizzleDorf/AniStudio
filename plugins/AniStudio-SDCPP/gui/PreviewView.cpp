// PreviewView.cpp
#include "PreviewView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "SDCPPSettingsComponent.hpp"
#include "SettingsSystem.hpp"
#include "TextureSystem.hpp"
#include "Log.hpp"
#include "ECS.h"
#include <cstdint>
#include <cfloat>
#include <cstdlib>
#include <cstring>

namespace GUI {

    PreviewView::PreviewView(ECS::EntityManager& entityMgr, ViewManager& viewMgr)
        : BaseView(entityMgr, viewMgr) {
        viewName = "PreviewView";
    }

    PreviewView::~PreviewView() {
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
        DiffusionCallbackUtils::SetPreviewMode(
            PreviewModeToInt(m_mode), m_interval);
    }

    void PreviewView::Init() {
        LoadModeFromSettings();
        ApplyModeToLibrary();
        m_lastSequence = DiffusionCallbackUtils::GetPreviewSequence();

        m_textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
        m_videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();

        // ---- image preview entity (unchanged shape) ----
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

        // ---- video preview entity (new; never persisted, never enumerated
        //      as media because it carries PreviewVideoComponent, not a
        //      plain VideoComponent, and PlaybackStateComponent is set to
        //      Streaming mode) ----
        m_videoPreviewEntity = m_entityManager.AddNewEntity();
        if (m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            m_entityManager.AddComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            m_entityManager.AddComponent<ECS::TextureComponent>(m_videoPreviewEntity);

            // Pre-seed the playback state so VideoSystem uses the 2-frame
            // streaming buffer instead of the 24-frame cached ring.
            m_entityManager.AddComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
            auto& st = m_entityManager.GetComponent<ECS::PlaybackStateComponent>(m_videoPreviewEntity);
            st.mode = ECS::PlaybackMode::Streaming;

            m_videoPreviewEntityHasComponent = true;
        }
    }

    void PreviewView::Update(const float deltaT) {
        (void)deltaT;

        // Autoplay once the decoder is ready. AVSystem::Play is idempotent
        // enough for our purposes; we only call it while we're still Stopped.
        if (m_videoAttached && m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity)) {
            auto* av = m_entityManager.GetSystem<ECS::AVSystem>();
            if (av && av->GetState(m_videoPreviewEntity) == ECS::PlaybackState::Stopped &&
                av->IsMediaReady(m_videoPreviewEntity)) {
                av->SetLooping(m_videoPreviewEntity, m_videoLoops);
                av->Play(m_videoPreviewEntity);
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

    // ---- image preview (unchanged) ----

    void PreviewView::PushPreviewFrameToComponent(const PreviewFrame& frame) {
        if (!m_previewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_previewEntity)) return;

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

    void PreviewView::RefreshTextureIfNeeded() {
        const uint64_t seq = DiffusionCallbackUtils::GetPreviewSequence();
        if (seq == m_lastSequence) return;

        PreviewFrame frame = DiffusionCallbackUtils::GetLatestPreview();
        if (!frame.valid()) {
            m_lastSequence = seq;
            return;
        }

        if (frame.channels != 1 && frame.channels != 3 && frame.channels != 4) {
            m_lastSequence = seq;
            return;
        }

        m_lastSequence = seq;
        PushPreviewFrameToComponent(frame);
    }

    // ---- video preview (new; same shape as image, sourced from VideoSystem) ----

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

        // Cheap "changed?" check: dimensions plus a byte-length compare.
        // VideoSystem publishes the same RGBA buffer until the next frame
        // lands, so we skip re-uploading when nothing new arrived.
        if (w == m_lastVideoWidth && h == m_lastVideoHeight) {
            // Nothing size-wise changed; we still upload because the contents
            // of currentFrameRGBA are swapped in-place. If you want a strict
            // change check, add a frame counter to the Track and expose it.
        }

        m_lastVideoWidth = w;
        m_lastVideoHeight = h;
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

        // Reflect size on the component for the draw path.
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
            !m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;

        auto* av = m_entityManager.GetSystem<ECS::AVSystem>();
        if (!av) {
            ANI_LOG_WARN("[PreviewView] ShowVideo: AVSystem unavailable");
            return;
        }

        // Label for tooltips / debugging.
        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            pvc.sourceName = label;
        }

        // Load INTO the preview entity. AttachMedia sees PreviewVideoComponent
        // (which is-a VideoComponent) and uses it directly, so no plain
        // VideoComponent is added. The entity remains a preview entity and
        // is skipped by media-history enumeration.
        if (!av->AttachMedia(m_videoPreviewEntity, path,
            ECS::TrackType::Both,
            ECS::PlaybackMode::Streaming)) {
            ANI_LOG_WARN("[PreviewView] ShowVideo: AttachMedia failed for %s", path.c_str());
            m_videoAttached = false;
            return;
        }

        m_videoLoops = loop;
        av->SetLooping(m_videoPreviewEntity, loop);
        m_videoAttached = true;
        m_lastVideoWidth = 0;
        m_lastVideoHeight = 0;
    }

    void PreviewView::ClearVideo() {
        if (!m_videoPreviewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return;

        auto* av = m_entityManager.GetSystem<ECS::AVSystem>();
        if (av) av->Stop(m_videoPreviewEntity);

        if (m_textureSystem) m_textureSystem->RemoveTexture(m_videoPreviewEntity);

        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            auto& pvc = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity);
            pvc.Unload();
        }

        m_videoAttached = false;
        m_lastVideoWidth = 0;
        m_lastVideoHeight = 0;
    }

    // ---- UI ----

    void PreviewView::DrawMenuBar() {
        if (!ImGui::BeginMenuBar()) return;

        if (ImGui::BeginMenu("Preview")) {
            if (ImGui::MenuItem("None", nullptr, m_mode == PreviewMode::None)) {
                m_mode = PreviewMode::None;
                SaveModeToSettings();
                ApplyModeToLibrary();
                DiffusionCallbackUtils::ClearPreview();
                m_lastSequence = DiffusionCallbackUtils::GetPreviewSequence();
            }
            if (ImGui::MenuItem("Proj (latent)", nullptr, m_mode == PreviewMode::Proj)) {
                m_mode = PreviewMode::Proj;
                SaveModeToSettings();
                ApplyModeToLibrary();
            }
            if (ImGui::MenuItem("TAE (fast VAE)", nullptr, m_mode == PreviewMode::Tae)) {
                m_mode = PreviewMode::Tae;
                SaveModeToSettings();
                ApplyModeToLibrary();
            }
            if (ImGui::MenuItem("VAE (full)", nullptr, m_mode == PreviewMode::Vae)) {
                m_mode = PreviewMode::Vae;
                SaveModeToSettings();
                ApplyModeToLibrary();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Clear")) {
            DiffusionCallbackUtils::ClearPreview();
            m_lastSequence = DiffusionCallbackUtils::GetPreviewSequence();

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

        const auto& prog = DiffusionCallbackUtils::GetProgressData();
        int step = prog.currentStep.load();
        int total = prog.totalSteps.load();
        float t = prog.currentTime.load();
        bool active = prog.isProcessing.load();

        std::string status;
        if (active && total > 0) {
            status = "Step " + std::to_string(step) + "/" + std::to_string(total);
            if (t > 0.0f) {
                char buf[64];
                snprintf(buf, sizeof(buf), " | %.2fs/step", t);
                status += buf;
            }
        }
        else {
            status = "Idle";
        }

        float avail = ImGui::GetContentRegionAvail().x;
        float tw = ImGui::CalcTextSize(status.c_str()).x;
        if (avail > tw) {
            ImGui::SameLine(ImGui::GetWindowWidth() - tw - 16.0f);
        }
        else {
            ImGui::SameLine();
        }
        ImGui::TextUnformatted(status.c_str());

        ImGui::EndMenuBar();
    }

    bool PreviewView::DrawVideoControls() {
        if (!m_videoAttached) return false;
        if (!m_entityManager.IsEntityValid(m_videoPreviewEntity)) return false;

        auto* av = m_entityManager.GetSystem<ECS::AVSystem>();
        if (!av) return false;

        std::string label = "preview";
        if (m_entityManager.HasComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity)) {
            label = m_entityManager.GetComponent<ECS::PreviewVideoComponent>(m_videoPreviewEntity).sourceName;
        }

        ImGui::Text("%s", label.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(streaming)");

        const bool ready = av->IsMediaReady(m_videoPreviewEntity);
        const bool playing = (av->GetState(m_videoPreviewEntity) == ECS::PlaybackState::Playing);
        const bool paused = (av->GetState(m_videoPreviewEntity) == ECS::PlaybackState::Paused);

        ImGui::BeginDisabled(!ready);

        if (ImGui::Button(playing ? "Pause" : "Play")) {
            if (playing) av->Pause(m_videoPreviewEntity);
            else         av->Play(m_videoPreviewEntity);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) av->Stop(m_videoPreviewEntity);
        ImGui::SameLine();

        bool loop = m_videoLoops;
        if (ImGui::Checkbox("Loop", &loop)) {
            m_videoLoops = loop;
            av->SetLooping(m_videoPreviewEntity, loop);
        }

        double pos = av->GetPosition(m_videoPreviewEntity);
        double dur = av->GetDuration(m_videoPreviewEntity);
        if (dur > 0.0) {
            float fpos = (float)pos;
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##pos", &fpos, 0.0f, (float)dur, "%.2fs / %.2fs")) {
                av->Seek(m_videoPreviewEntity, (double)fpos);
            }
        }

        ImGui::EndDisabled();
        return true;
    }

    void PreviewView::Render() {
        if (!m_modeLoaded) {
            LoadModeFromSettings();
            ApplyModeToLibrary();
        }

        const bool showVideo = m_videoAttached &&
            m_videoPreviewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_videoPreviewEntity);

        // Image polling only when we're not showing video.
        if (!showVideo && m_mode != PreviewMode::None) {
            RefreshTextureIfNeeded();
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

        // Pick which entity to draw.
        ECS::EntityID texEntity = showVideo ? m_videoPreviewEntity : m_previewEntity;

        // Video transport bar goes above the image.
        if (showVideo) {
            DrawVideoControls();
            ImGui::Separator();
        }

        if (!showVideo && m_mode == PreviewMode::None) {
            ImGui::TextDisabled("Preview disabled.");
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
            tex == 0 || tw <= 0 || th <= 0) {
            if (showVideo) {
                ImGui::TextUnformatted("Waiting for first video frame...");
            }
            else if (DiffusionCallbackUtils::GetProgressData().isProcessing.load()) {
                ImGui::TextUnformatted("Waiting for first preview frame...");
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

        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)(intptr_t)tex,
            topLeft,
            bottomRight,
            ImVec2(0, 0), ImVec2(1, 1));

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
            if (m >= 0 && m <= 3) {
                m_mode = static_cast<PreviewMode>(m);
            }
        }
        if (j.contains("previewInterval")) {
            m_interval = j["previewInterval"].get<int>();
        }
    }

} // namespace GUI