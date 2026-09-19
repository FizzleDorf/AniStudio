// PreviewView.cpp
#include "PreviewView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "OpenGLUtils.hpp"
#include "SDCPPSettingsComponent.hpp"
#include "SettingsSystem.hpp"
#include "ImageSystem.hpp"
#include "ImageUtils.hpp"
#include "TextureSystem.hpp"
#include "Log.hpp"
#include "ECS.h"
#include "OpenGLWrapper.hpp"
#include <filesystem>
#include <cstdint>
#include <cfloat>

namespace GUI {

    PreviewView::PreviewView(ECS::EntityManager& entityMgr, ViewManager& viewMgr)
        : BaseView(entityMgr, viewMgr) {
        viewName = "PreviewView";
    }

    PreviewView::~PreviewView() {
        DeleteLastTempFile();

        if (m_previewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_previewEntity)) {
            m_entityManager.DestroyEntity(m_previewEntity);
        }
        m_previewEntity = 0;
        m_previewEntityHasComponent = false;
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

        m_previewEntity = m_entityManager.AddNewEntity();
        if (m_entityManager.IsEntityValid(m_previewEntity)) {
            if (!m_entityManager.HasComponent<ECS::ImageComponent>(m_previewEntity)) {
                m_entityManager.AddComponent<ECS::ImageComponent>(m_previewEntity);
            }
            m_previewEntityHasComponent = true;
        }
    }

    void PreviewView::Update(const float deltaT) {
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

    void PreviewView::DeleteLastTempFile() {
        if (m_lastTempPath.empty()) return;
        std::error_code ec;
        std::filesystem::remove(m_lastTempPath, ec);
        m_lastTempPath.clear();
    }

    void PreviewView::PushPreviewFrameToComponent(const PreviewFrame& frame) {
        if (!m_previewEntityHasComponent) return;
        if (!m_entityManager.IsEntityValid(m_previewEntity)) return;

        if (!m_entityManager.HasComponent<ECS::ImageComponent>(m_previewEntity)) {
            m_entityManager.AddComponent<ECS::ImageComponent>(m_previewEntity);
        }

        std::filesystem::path tmpDir = std::filesystem::temp_directory_path();
        std::filesystem::path tmpFile = tmpDir /
            ("anistudio_preview_" + std::to_string(frame.sequence) + ".png");
        std::string tmp = tmpFile.string();

        std::string mutablePath = tmp;
        if (!Utils::ImageUtils::SaveImage(mutablePath, frame.width, frame.height,
            frame.channels, frame.data.get())) {
            ANI_LOG_ERROR("[PreviewView] SaveImage failed for preview frame seq=%llu size=%dx%d ch=%d",
                static_cast<unsigned long long>(frame.sequence),
                frame.width, frame.height, frame.channels);
            return;
        }

        if (auto imgSys = m_entityManager.GetSystem<ECS::ImageSystem>()) {
            imgSys->SetImage(m_previewEntity, tmp);
        }

        DeleteLastTempFile();
        m_lastTempPath = tmp;
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
                m_entityManager.IsEntityValid(m_previewEntity) &&
                m_entityManager.HasComponent<ECS::ImageComponent>(m_previewEntity)) {
                auto& c = m_entityManager.GetComponent<ECS::ImageComponent>(m_previewEntity);
                c.ClearImageData();
            }
            if (m_previewEntityHasComponent &&
                m_entityManager.IsEntityValid(m_previewEntity) &&
                m_entityManager.HasComponent<ECS::TextureComponent>(m_previewEntity)) {
                if (auto texSys = m_entityManager.GetSystem<ECS::TextureSystem>()) {
                    texSys->RemoveTexture(m_previewEntity);
                }
            }
            DeleteLastTempFile();
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

    void PreviewView::Render() {
        if (!m_modeLoaded) {
            LoadModeFromSettings();
            ApplyModeToLibrary();
        }

        if (m_mode != PreviewMode::None) {
            RefreshTextureIfNeeded();
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 150), ImVec2(FLT_MAX, FLT_MAX));
        if (!ImGui::Begin(GetWindowTitle().c_str(), &windowOpen,
            ImGuiWindowFlags_MenuBar)) {
            ImGui::End();
            return;
        }

        DrawMenuBar();

        if (m_mode == PreviewMode::None) {
            ImGui::TextDisabled("Preview disabled.");
            ImGui::End();
            return;
        }

        GLuint tex = 0;
        int tw = 0;
        int th = 0;

        if (m_previewEntityHasComponent &&
            m_entityManager.IsEntityValid(m_previewEntity)) {
            if (m_entityManager.HasComponent<ECS::TextureComponent>(m_previewEntity)) {
                tex = m_entityManager.GetComponent<ECS::TextureComponent>(m_previewEntity).textureID;
            }
            if (m_entityManager.HasComponent<ECS::ImageComponent>(m_previewEntity)) {
                auto& c = m_entityManager.GetComponent<ECS::ImageComponent>(m_previewEntity);
                tw = c.width;
                th = c.height;
            }
        }

        if (tex == 0 || tw <= 0 || th <= 0) {
            const auto& prog = DiffusionCallbackUtils::GetProgressData();
            if (prog.isProcessing.load()) {
                ImGui::TextUnformatted("Waiting for first preview frame...");
            }
            else {
                ImGui::TextDisabled("No preview available.");
            }
            ImGui::End();
            return;
        }

        if (!glIsTexture(tex)) {
            ImGui::TextDisabled("No preview available.");
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