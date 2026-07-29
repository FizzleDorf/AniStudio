// DiffusionView.cpp
#include "DiffusionView.hpp"
#include "Events.hpp"
#include "DiffusionOptions.hpp"
#include "UISchema.hpp"
#include "PngMetadataUtils.hpp"
#include "ContextMenuUtils.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "utils.h"
#include "SDcppSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include <png.h>
#include <exiv2/exiv2.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace ECS;
using namespace ANI;

namespace GUI {

    void DiffusionView::Init() {
        GUI::DiffusionCallbackUtils::InitializeCallbacks();
        ResetEntities();
        QuickLoad();
    }

    void DiffusionView::ResetEntities() {
        if (txt2imgEntity != 0) m_entityManager.DestroyEntity(txt2imgEntity);
        if (img2imgEntity != 0) m_entityManager.DestroyEntity(img2imgEntity);
        if (editEntity != 0) m_entityManager.DestroyEntity(editEntity);

        txt2imgEntity = CreateEntityWithComponents(false);
        img2imgEntity = CreateEntityWithComponents(true);
        editEntity = CreateEntityWithComponents(true);

        if (m_entityManager.HasComponent<SamplerComponent>(img2imgEntity)) {
            m_entityManager.GetComponent<SamplerComponent>(img2imgEntity).denoise = 0.6f;
        }
        if (m_entityManager.HasComponent<SamplerComponent>(editEntity)) {
            m_entityManager.GetComponent<SamplerComponent>(editEntity).denoise = 0.6f;
        }
    }

    ECS::EntityID DiffusionView::CreateEntityWithComponents(bool includeInputImage) {
        EntityID entity = m_entityManager.AddNewEntity();

        m_entityManager.AddComponent<CheckpointComponent>(entity);
        m_entityManager.AddComponent<LatentComponent>(entity);
        m_entityManager.AddComponent<SamplerComponent>(entity);
        m_entityManager.AddComponent<GuidanceComponent>(entity);
        m_entityManager.AddComponent<PromptComponent>(entity);
        m_entityManager.AddComponent<OutputImageComponent>(entity);

        if (includeInputImage) {
            m_entityManager.AddComponent<InputImageComponent>(entity);
        }

        return entity;
    }

    bool DiffusionView::IsEntitySafeToUse(ECS::EntityID entity) const {
        return m_entityManager.IsEntityValid(entity);
    }

    void DiffusionView::ToggleComponent(EntityID entity, const std::string& name) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;
        if (name == "InputImage") {
            if (m_entityManager.HasComponent<InputImageComponent>(entity))
                m_entityManager.RemoveComponent<InputImageComponent>(entity);
            else
                m_entityManager.AddComponent<InputImageComponent>(entity);
            return;
        }
        if (name == "CheckpointComponent") {
            if (m_entityManager.HasComponent<CheckpointComponent>(entity))
                m_entityManager.RemoveComponent<CheckpointComponent>(entity);
            else
                m_entityManager.AddComponent<CheckpointComponent>(entity);
        }
        else if (name == "DiffusionModelComponent") {
            if (m_entityManager.HasComponent<DiffusionModelComponent>(entity))
                m_entityManager.RemoveComponent<DiffusionModelComponent>(entity);
            else
                m_entityManager.AddComponent<DiffusionModelComponent>(entity);
        }
        else if (name == "ClipLComponent") {
            if (m_entityManager.HasComponent<ClipLComponent>(entity))
                m_entityManager.RemoveComponent<ClipLComponent>(entity);
            else
                m_entityManager.AddComponent<ClipLComponent>(entity);
        }
        else if (name == "ClipGComponent") {
            if (m_entityManager.HasComponent<ClipGComponent>(entity))
                m_entityManager.RemoveComponent<ClipGComponent>(entity);
            else
                m_entityManager.AddComponent<ClipGComponent>(entity);
        }
        else if (name == "T5XXLComponent") {
            if (m_entityManager.HasComponent<T5XXLComponent>(entity))
                m_entityManager.RemoveComponent<T5XXLComponent>(entity);
            else
                m_entityManager.AddComponent<T5XXLComponent>(entity);
        }
        else if (name == "ClipVisionComponent") {
            if (m_entityManager.HasComponent<ClipVisionComponent>(entity))
                m_entityManager.RemoveComponent<ClipVisionComponent>(entity);
            else
                m_entityManager.AddComponent<ClipVisionComponent>(entity);
        }
        else if (name == "LlmEncoderComponent") {
            if (m_entityManager.HasComponent<LlmEncoderComponent>(entity))
                m_entityManager.RemoveComponent<LlmEncoderComponent>(entity);
            else
                m_entityManager.AddComponent<LlmEncoderComponent>(entity);
        }
        else if (name == "LlmVisionComponent") {
            if (m_entityManager.HasComponent<LlmVisionComponent>(entity))
                m_entityManager.RemoveComponent<LlmVisionComponent>(entity);
            else
                m_entityManager.AddComponent<LlmVisionComponent>(entity);
        }
        else if (name == "VaeComponent") {
            if (m_entityManager.HasComponent<VaeComponent>(entity))
                m_entityManager.RemoveComponent<VaeComponent>(entity);
            else
                m_entityManager.AddComponent<VaeComponent>(entity);
        }
        else if (name == "TaesdComponent") {
            if (m_entityManager.HasComponent<TaesdComponent>(entity))
                m_entityManager.RemoveComponent<TaesdComponent>(entity);
            else
                m_entityManager.AddComponent<TaesdComponent>(entity);
        }
        else if (name == "LoraComponent") {
            if (m_entityManager.HasComponent<LoraComponent>(entity))
                m_entityManager.RemoveComponent<LoraComponent>(entity);
            else
                m_entityManager.AddComponent<LoraComponent>(entity);
        }
        else if (name == "ControlNetComponent") {
            if (m_entityManager.HasComponent<ControlNetComponent>(entity))
                m_entityManager.RemoveComponent<ControlNetComponent>(entity);
            else
                m_entityManager.AddComponent<ControlNetComponent>(entity);
        }
        else if (name == "EmbeddingComponent") {
            if (m_entityManager.HasComponent<EmbeddingComponent>(entity))
                m_entityManager.RemoveComponent<EmbeddingComponent>(entity);
            else
                m_entityManager.AddComponent<EmbeddingComponent>(entity);
        }
        else if (name == "PhotoMakerComponent") {
            if (m_entityManager.HasComponent<PhotoMakerComponent>(entity))
                m_entityManager.RemoveComponent<PhotoMakerComponent>(entity);
            else
                m_entityManager.AddComponent<PhotoMakerComponent>(entity);
        }
        else if (name == "StackedIdEmbedComponent") {
            if (m_entityManager.HasComponent<StackedIdEmbedComponent>(entity))
                m_entityManager.RemoveComponent<StackedIdEmbedComponent>(entity);
            else
                m_entityManager.AddComponent<StackedIdEmbedComponent>(entity);
        }
        else if (name == "LatentComponent") {
            if (m_entityManager.HasComponent<LatentComponent>(entity))
                m_entityManager.RemoveComponent<LatentComponent>(entity);
            else
                m_entityManager.AddComponent<LatentComponent>(entity);
        }
        else if (name == "SamplerComponent") {
            if (m_entityManager.HasComponent<SamplerComponent>(entity))
                m_entityManager.RemoveComponent<SamplerComponent>(entity);
            else
                m_entityManager.AddComponent<SamplerComponent>(entity);
        }
        else if (name == "GuidanceComponent") {
            if (m_entityManager.HasComponent<GuidanceComponent>(entity))
                m_entityManager.RemoveComponent<GuidanceComponent>(entity);
            else
                m_entityManager.AddComponent<GuidanceComponent>(entity);
        }
        else if (name == "PromptComponent") {
            if (m_entityManager.HasComponent<PromptComponent>(entity))
                m_entityManager.RemoveComponent<PromptComponent>(entity);
            else
                m_entityManager.AddComponent<PromptComponent>(entity);
        }
        else if (name == "ConversionComponent") {
            if (m_entityManager.HasComponent<ConversionComponent>(entity))
                m_entityManager.RemoveComponent<ConversionComponent>(entity);
            else
                m_entityManager.AddComponent<ConversionComponent>(entity);
        }
    }

    bool DiffusionView::IsComponentPresent(EntityID entity, const std::string& name) const {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return false;
        if (name == "InputImage")
            return m_entityManager.HasComponent<InputImageComponent>(entity);
        if (name == "CheckpointComponent")
            return m_entityManager.HasComponent<CheckpointComponent>(entity);
        if (name == "DiffusionModelComponent")
            return m_entityManager.HasComponent<DiffusionModelComponent>(entity);
        if (name == "ClipLComponent")
            return m_entityManager.HasComponent<ClipLComponent>(entity);
        if (name == "ClipGComponent")
            return m_entityManager.HasComponent<ClipGComponent>(entity);
        if (name == "T5XXLComponent")
            return m_entityManager.HasComponent<T5XXLComponent>(entity);
        if (name == "ClipVisionComponent")
            return m_entityManager.HasComponent<ClipVisionComponent>(entity);
        if (name == "LlmEncoderComponent")
            return m_entityManager.HasComponent<LlmEncoderComponent>(entity);
        if (name == "LlmVisionComponent")
            return m_entityManager.HasComponent<LlmVisionComponent>(entity);
        if (name == "VaeComponent")
            return m_entityManager.HasComponent<VaeComponent>(entity);
        if (name == "TaesdComponent")
            return m_entityManager.HasComponent<TaesdComponent>(entity);
        if (name == "LoraComponent")
            return m_entityManager.HasComponent<LoraComponent>(entity);
        if (name == "ControlNetComponent")
            return m_entityManager.HasComponent<ControlNetComponent>(entity);
        if (name == "EmbeddingComponent")
            return m_entityManager.HasComponent<EmbeddingComponent>(entity);
        if (name == "PhotoMakerComponent")
            return m_entityManager.HasComponent<PhotoMakerComponent>(entity);
        if (name == "StackedIdEmbedComponent")
            return m_entityManager.HasComponent<StackedIdEmbedComponent>(entity);
        if (name == "LatentComponent")
            return m_entityManager.HasComponent<LatentComponent>(entity);
        if (name == "SamplerComponent")
            return m_entityManager.HasComponent<SamplerComponent>(entity);
        if (name == "GuidanceComponent")
            return m_entityManager.HasComponent<GuidanceComponent>(entity);
        if (name == "PromptComponent")
            return m_entityManager.HasComponent<PromptComponent>(entity);
        if (name == "ConversionComponent")
            return m_entityManager.HasComponent<ConversionComponent>(entity);
        return false;
    }

    void DiffusionView::SetModelMode(EntityID entity, bool useCheckpoint) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;
        if (useCheckpoint) {
            if (!m_entityManager.HasComponent<CheckpointComponent>(entity))
                m_entityManager.AddComponent<CheckpointComponent>(entity);
            m_entityManager.RemoveComponent<DiffusionModelComponent>(entity);
            m_entityManager.RemoveComponent<ClipLComponent>(entity);
            m_entityManager.RemoveComponent<ClipGComponent>(entity);
            m_entityManager.RemoveComponent<T5XXLComponent>(entity);
            m_entityManager.RemoveComponent<ClipVisionComponent>(entity);
            m_entityManager.RemoveComponent<LlmEncoderComponent>(entity);
            m_entityManager.RemoveComponent<LlmVisionComponent>(entity);
            m_entityManager.RemoveComponent<VaeComponent>(entity);
            m_entityManager.RemoveComponent<TaesdComponent>(entity);
        }
        else {
            m_entityManager.RemoveComponent<CheckpointComponent>(entity);
            if (!m_entityManager.HasComponent<DiffusionModelComponent>(entity))
                m_entityManager.AddComponent<DiffusionModelComponent>(entity);
            if (!m_entityManager.HasComponent<ClipLComponent>(entity))
                m_entityManager.AddComponent<ClipLComponent>(entity);
            if (!m_entityManager.HasComponent<ClipGComponent>(entity))
                m_entityManager.AddComponent<ClipGComponent>(entity);
            if (!m_entityManager.HasComponent<T5XXLComponent>(entity))
                m_entityManager.AddComponent<T5XXLComponent>(entity);
            if (!m_entityManager.HasComponent<ClipVisionComponent>(entity))
                m_entityManager.AddComponent<ClipVisionComponent>(entity);
            if (!m_entityManager.HasComponent<LlmEncoderComponent>(entity))
                m_entityManager.AddComponent<LlmEncoderComponent>(entity);
            if (!m_entityManager.HasComponent<LlmVisionComponent>(entity))
                m_entityManager.AddComponent<LlmVisionComponent>(entity);
            if (!m_entityManager.HasComponent<VaeComponent>(entity))
                m_entityManager.AddComponent<VaeComponent>(entity);
            if (!m_entityManager.HasComponent<TaesdComponent>(entity))
                m_entityManager.AddComponent<TaesdComponent>(entity);
        }
    }

    bool DiffusionView::IsCheckpointMode(EntityID entity) const {
        return m_entityManager.HasComponent<CheckpointComponent>(entity);
    }

    void DiffusionView::ResetToDefaultComponents(EntityID entity) {
        std::vector<std::string> all = m_entityManager.GetAllRegisteredComponentNames();
        for (const std::string& name : all) {
            if (name == "InputImage") continue;
            if (name == "CheckpointComponent" || name == "LatentComponent" ||
                name == "SamplerComponent" || name == "GuidanceComponent" ||
                name == "PromptComponent" || name == "OutputImageComponent") {
                if (!IsComponentPresent(entity, name))
                    ToggleComponent(entity, name);
            }
            else {
                if (IsComponentPresent(entity, name))
                    ToggleComponent(entity, name);
            }
        }
    }

    void DiffusionView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save Metadata...")) {
                    std::string defaultPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
                    std::string defaultFilename = viewName + "_" + std::to_string(GetID()) + ".json";
                    std::string selectedFile;
                    if (FileDialog::SaveFile("Save Metadata", FileDialog::FilterType::METADATA_FILE, defaultFilename, selectedFile, defaultPath)) {
                        SaveMetadataToJson(selectedFile);
                    }
                }
                if (ImGui::MenuItem("Load Metadata...")) {
                    std::string defaultPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
                    std::string selectedFile;
                    if (FileDialog::OpenFile("Load Metadata", FileDialog::FilterType::METADATA_FILE, selectedFile, defaultPath)) {
                        std::string ext = std::filesystem::path(selectedFile).extension().string();
                        if (ext == ".json")
                            LoadMetadataFromJson(selectedFile);
                        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                            LoadMetadataFromPNG(selectedFile);
                    }
                }
                if (ImGui::MenuItem("Quick Save"))
                    QuickSave();
                if (ImGui::MenuItem("Quick Load"))
                    QuickLoad();
                ImGui::Separator();
                if (ImGui::MenuItem("Reset View"))
                    ResetEntities();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Model Mode")) {
                EntityID current = GetCurrentEntity();
                bool cp = IsCheckpointMode(current);
                if (ImGui::MenuItem("Checkpoint", nullptr, cp))
                    SetModelMode(current, true);
                if (ImGui::MenuItem("Split", nullptr, !cp))
                    SetModelMode(current, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Components")) {
                EntityID current = GetCurrentEntity();
                if (ImGui::MenuItem("Reset to Defaults"))
                    ResetToDefaultComponents(current);
                ImGui::Separator();

                std::vector<std::string> all = m_entityManager.GetAllRegisteredComponentNames();
                std::sort(all.begin(), all.end());
                for (const std::string& name : all) {
                    if (name == "BaseModelComponent" || name == "BaseComponent") continue;
                    if (name.find("Esrgan") != std::string::npos ||
                        name.find("Video") != std::string::npos ||
                        name.find("Upscale") != std::string::npos ||
                        name.find("Audio") != std::string::npos ||
                        name == "HighNoiseDiffusionModel" ||
                        name == "UncondDiffusionModel") continue;
                    bool present = IsComponentPresent(current, name);
                    if (ImGui::MenuItem(name.c_str(), nullptr, present))
                        ToggleComponent(current, name);
                }
                if (currentMode != 0) {
                    bool hasInput = m_entityManager.HasComponent<InputImageComponent>(current);
                    if (ImGui::MenuItem("InputImage", nullptr, hasInput))
                        ToggleComponent(current, "InputImage");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void DiffusionView::RenderComponent(EntityID entity, ComponentTypeID compId, const std::string& name) {
        auto* comp = m_entityManager.GetComponentById(entity, compId);
        if (!comp || comp->schema.empty()) return;
        try {
            auto props = comp->GetPropertyMap();
            if (name == "Latent") {
                UISchema::RenderSchema(comp->schema, props);
                if (m_entityManager.HasComponent<LatentComponent>(entity)) {
                    auto& l = m_entityManager.GetComponent<LatentComponent>(entity);
                    int w = std::max(64, ((l.latentWidth + 32) / 64) * 64);
                    int h = std::max(64, ((l.latentHeight + 32) / 64) * 64);
                    if (w != l.latentWidth || h != l.latentHeight) {
                        l.latentWidth = w;
                        l.latentHeight = h;
                    }
                    ImGui::Text("Current: %dx%d", l.latentWidth, l.latentHeight);
                    if (ImGui::Button("Swap", ImVec2(-1.0f, 0))) {
                        int t = l.latentWidth;
                        l.latentWidth = l.latentHeight;
                        l.latentHeight = t;
                    }
                }
            }
            else if (name == "InputImage") {
                UISchema::RenderSchema(comp->schema, props);
                if (m_entityManager.HasComponent<InputImageComponent>(entity)) {
                    auto& in = m_entityManager.GetComponent<InputImageComponent>(entity);
                    if (!in.filePath.empty()) {
                        ImGui::Text("Selected: %s", in.filePath.c_str());
                        if (in.width <= 0 || in.height <= 0) {
                            int w, h, ch;
                            unsigned char* data = stbi_load(in.filePath.c_str(), &w, &h, &ch, 0);
                            if (data) {
                                in.width = w; in.height = h; in.channels = ch;
                                stbi_image_free(data);
                            }
                        }
                        if (in.width > 0 && in.height > 0)
                            ImGui::Text("Dimensions: %dx%d", in.width, in.height);
                        if (ImGui::Button("Set Latent to Image Size", ImVec2(-1.0f, 0))) {
                            if (m_entityManager.HasComponent<LatentComponent>(entity)) {
                                auto& l = m_entityManager.GetComponent<LatentComponent>(entity);
                                if (in.width > 0 && in.height > 0) {
                                    l.latentWidth = in.width;
                                    l.latentHeight = in.height;
                                }
                            }
                        }
                    }
                    else {
                        ImGui::TextDisabled("No image selected");
                    }
                }
            }
            else {
                UISchema::RenderSchema(comp->schema, props);
            }
        }
        catch (...) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error rendering %s", name.c_str());
        }
    }

    void DiffusionView::RenderEntityComponents(const EntityID entity) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;

        auto componentIds = m_entityManager.GetEntityComponents(entity);
        for (ComponentTypeID compId : componentIds) {
            std::string name = m_entityManager.GetComponentNameById(compId);
            if (name == "InputImage" && currentMode == 0) continue;
            if (name == "BaseComponent" || name == "BaseModelComponent") continue;

            if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                RenderComponent(entity, compId, name);
                ImGui::Unindent();
            }
        }
    }

    void DiffusionView::RenderMainContextMenu() {
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("DiffusionMainContext");
        if (ImGui::BeginPopup("DiffusionMainContext")) {
            ImGui::Text("Diffusion View");
            ImGui::Separator();
            EntityID current = GetCurrentEntity();
            if (ImGui::MenuItem("Copy Current Entity"))
                contextMenuUtils->CopyEntity(current);
            ImGui::Separator();
            if (contextMenuUtils->HasClipboardEntity()) {
                ImGui::TextDisabled("Clipboard: %s", contextMenuUtils->GetClipboardPreview().c_str());
                ImGui::Separator();
                contextMenuUtils->RenderPasteMenu(current);
            }
            else {
                ImGui::TextDisabled("Nothing to paste");
            }
            ImGui::EndPopup();
        }
    }

    void DiffusionView::HandleT2IEvent() {
        EntityID newEntity = m_entityManager.CloneEntity(txt2imgEntity);
        if (newEntity == 0) return;
        auto sdSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
        if (sdSystem)
            sdSystem->QueueTask(newEntity, ECS::SDCPPSystem::TaskType::Inference);
        else
            m_entityManager.DestroyEntity(newEntity);
    }

    void DiffusionView::HandleI2IEvent() {
        EntityID newEntity = m_entityManager.CloneEntity(img2imgEntity);
        if (newEntity == 0) return;
        if (m_entityManager.HasComponent<OutputImageComponent>(newEntity)) {
            auto& out = m_entityManager.GetComponent<OutputImageComponent>(newEntity);
            if (out.filePath.empty())
                out.filePath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
            if (out.fileName.empty())
                out.fileName = "AniStudio.png";
            std::filesystem::create_directories(out.filePath);
        }
        auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Img2Img);
        ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
    }

    void DiffusionView::HandleEditEvent() {
        EntityID newEntity = m_entityManager.CloneEntity(editEntity);
        if (newEntity == 0) return;
        if (m_entityManager.HasComponent<OutputImageComponent>(newEntity)) {
            auto& out = m_entityManager.GetComponent<OutputImageComponent>(newEntity);
            if (out.filePath.empty())
                out.filePath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
            if (out.fileName.empty())
                out.fileName = "AniStudio_edit.png";
            std::filesystem::create_directories(out.filePath);
        }
        auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Edit);
        ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
    }

    ECS::EntityID DiffusionView::GetCurrentEntity() const {
        switch (currentMode) {
        case 0: return txt2imgEntity;
        case 1: return img2imgEntity;
        case 2: return editEntity;
        default: return 0;
        }
    }

    void DiffusionView::RenderQueueList() {
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Queue")) {
            const auto& prog = DiffusionCallbackUtils::GetProgressData();
            int step = prog.currentStep;
            int total = prog.totalSteps;
            float time = prog.currentTime;
            bool processing = prog.isProcessing;
            if (processing && total > 0) {
                float p = static_cast<float>(step) / total;
                std::ostringstream ss;
                ss << "Processing: " << step << "/" << total << " steps (" << std::fixed << std::setprecision(1) << time << "s)";
                ImGui::Text("%s", ss.str().c_str());
                ImGui::ProgressBar(p, ImVec2(-FLT_MIN, 0));
            }
            else {
                ImGui::Text("Waiting...");
                ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
            }
            ImGui::Separator();
            if (ImGui::InputInt("Queue #", &numQueues, 1, 4))
                if (numQueues < 1) numQueues = 1;
            if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
                EntityID target = GetCurrentEntity();
                if (m_entityManager.HasComponent<LoraComponent>(target)) {
                    auto& lora = m_entityManager.GetComponent<LoraComponent>(target);
                    if (lora.modelPath.empty())
                        lora.modelPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("Lora") : "";
                }
                for (int i = 0; i < numQueues; ++i) {
                    switch (currentMode) {
                    case 0: HandleT2IEvent(); break;
                    case 1: HandleI2IEvent(); break;
                    case 2: HandleEditEvent(); break;
                    }
                }
            }
            ImGui::Separator();

            auto sdSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
            if (sdSystem) {
                isPaused = sdSystem->IsPaused();
            }

            if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("CancelCurrentDiffusionTask");
            }

            if (isPaused) {
                if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 0))) {
                    ANI::Events::Ref().QueueEvent("ResumeDiffusionWorker");
                    isPaused = false;
                }
            }
            else {
                if (ImGui::Button("Pause", ImVec2(-FLT_MIN, 0))) {
                    ANI::Events::Ref().QueueEvent("PauseDiffusionWorker");
                    isPaused = true;
                }
            }

            if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("StopCurrentDiffusionTask");
            }

            if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("ClearDiffusionQueue");
            }

            if (ImGui::Button("Clear All", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("ClearAllDiffusionTasks");
            }

            ImGui::Separator();

            if (ImGui::BeginTable("Queue", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                if (sdSystem) {
                    auto items = sdSystem->GetQueueSnapshot();
                    for (size_t i = 0; i < items.size(); ++i) {
                        const auto& item = items[i];
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(item.entityID));
                        ImGui::TableNextColumn();
                        if (item.processing)
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
                        else
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
                        ImGui::TableNextColumn();
                        if (!item.processing) {
                            if (i > 0) {
                                if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
                                    auto moveData = std::make_pair(i, i - 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }
                            if (i < items.size() - 1) {
                                if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
                                    auto moveData = std::make_pair(i, i + 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }
                            if (i > 0) {
                                if (ImGui::Button(("Top##Top" + std::to_string(i)).c_str())) {
                                    size_t target = items[0].processing ? 1 : 0;
                                    auto moveData = std::make_pair(i, target);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }
                            if (i < items.size() - 1) {
                                if (ImGui::Button(("Bottom##Bottom" + std::to_string(i)).c_str())) {
                                    auto moveData = std::make_pair(i, items.size() - 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }
                            if (ImGui::Button(("X##Remove" + std::to_string(i)).c_str()))
                                ANI::Events::Ref().QueueEventWithData("RemoveFromDiffusionQueue", i);
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void DiffusionView::Render() {
        RenderQueueList();
        ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            RenderMenuBar();
            if (contextMenuUtils->HasClipboardEntity()) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s",
                    contextMenuUtils->GetClipboardPreview().c_str());
                ImGui::Separator();
            }
            if (ImGui::BeginTabBar("Diffusion")) {
                if (ImGui::BeginTabItem("Txt2Img")) {
                    currentMode = 0;
                    RenderEntityComponents(txt2imgEntity);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Img2Img")) {
                    currentMode = 1;
                    RenderEntityComponents(img2imgEntity);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Edit")) {
                    currentMode = 2;
                    RenderEntityComponents(editEntity);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            RenderMainContextMenu();
        }
        ImGui::End();
        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    nlohmann::json DiffusionView::Serialize() const {
        nlohmann::json j;
        j["txt2imgEntity"] = m_entityManager.SerializeEntity(txt2imgEntity);
        j["img2imgEntity"] = m_entityManager.SerializeEntity(img2imgEntity);
        j["editEntity"] = m_entityManager.SerializeEntity(editEntity);
        j["componentVisibility"] = componentVisibility;
        j["currentMode"] = currentMode;
        j["numQueues"] = numQueues;
        j["isPaused"] = isPaused;
        return j;
    }

    void DiffusionView::Deserialize(const nlohmann::json& j) {
        try {
            if (txt2imgEntity != 0) m_entityManager.DestroyEntity(txt2imgEntity);
            if (img2imgEntity != 0) m_entityManager.DestroyEntity(img2imgEntity);
            if (editEntity != 0) m_entityManager.DestroyEntity(editEntity);

            if (j.contains("txt2imgEntity"))
                txt2imgEntity = m_entityManager.DeserializeEntity(j["txt2imgEntity"]);
            if (j.contains("img2imgEntity"))
                img2imgEntity = m_entityManager.DeserializeEntity(j["img2imgEntity"]);
            if (j.contains("editEntity"))
                editEntity = m_entityManager.DeserializeEntity(j["editEntity"]);

            if (j.contains("componentVisibility"))
                componentVisibility = j["componentVisibility"];
            if (j.contains("currentMode"))
                currentMode = j["currentMode"];
            if (j.contains("numQueues"))
                numQueues = j["numQueues"];
            if (j.contains("isPaused"))
                isPaused = j["isPaused"];
        }
        catch (...) {}
    }

    void DiffusionView::LoadMetadataFromPNG(const std::string& imagePath) {
        FILE* fp = fopen(imagePath.c_str(), "rb");
        if (!fp) return;
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) { fclose(fp); return; }
        png_infop info = png_create_info_struct(png);
        if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return; }
        if (setjmp(png_jmpbuf(png))) { png_destroy_read_struct(&png, &info, nullptr); fclose(fp); return; }
        png_init_io(png, fp);
        png_read_info(png, info);
        png_textp text_ptr;
        int num_text;
        if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
            for (int i = 0; i < num_text; ++i) {
                if (strcmp(text_ptr[i].key, "parameters") == 0) {
                    try {
                        nlohmann::json metadata = nlohmann::json::parse(text_ptr[i].text);
                        nlohmann::json converted;
                        converted["ID"] = metadata.value("ID", 0);
                        if (metadata.contains("software")) converted["software"] = metadata["software"];
                        if (metadata.contains("timestamp")) converted["timestamp"] = metadata["timestamp"];
                        if (metadata.contains("version")) converted["version"] = metadata["version"];
                        converted["components"] = nlohmann::json::array();
                        if (metadata.contains("components") && metadata["components"].is_object()) {
                            auto& comps = metadata["components"];
                            for (auto it = comps.begin(); it != comps.end(); ++it) {
                                std::string key = it.key();
                                if (key == "Base_Component") {
                                    nlohmann::json base; base["compName"] = "Base_Component";
                                    converted["components"].push_back(base);
                                    continue;
                                }
                                nlohmann::json elem;
                                elem[key] = it.value();
                                converted["components"].push_back(elem);
                            }
                        }
                        else if (metadata.contains("components") && metadata["components"].is_array()) {
                            converted["components"] = metadata["components"];
                        }
                        DeserializeAllEntities(converted);
                    }
                    catch (...) {}
                    break;
                }
            }
        }
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
    }

    void DiffusionView::LoadMetadataFromJson(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json meta;
                file >> meta;
                DeserializeAllEntities(meta);
            }
        }
        catch (...) {}
    }

    void DiffusionView::SaveMetadataToJson(const std::string& filepath) {
        try {
            nlohmann::json meta = Serialize();
            std::ofstream file(filepath);
            if (file.is_open())
                file << meta.dump(4);
        }
        catch (...) {}
    }

    void DiffusionView::QuickSave() {
        try {
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!filePathSys) return;

            std::string dataPath = filePathSys->GetPath("ProjectDataPath");
            if (dataPath.empty())
                dataPath = filePathSys->GetPath("DefaultProject");
            if (dataPath.empty()) return;

            std::filesystem::create_directories(dataPath);
            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            SaveMetadataToJson(filepath);
        }
        catch (...) {}
    }

    void DiffusionView::QuickLoad() {
        try {
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!filePathSys) return;

            std::string dataPath = filePathSys->GetPath("ProjectDataPath");
            if (dataPath.empty())
                dataPath = filePathSys->GetPath("DefaultProject");
            if (dataPath.empty()) return;

            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            if (!std::filesystem::exists(filepath)) return;
            LoadMetadataFromJson(filepath);
        }
        catch (...) {}
    }

    void DiffusionView::DeserializeAllEntities(const nlohmann::json& j) {
        if (j.contains("txt2imgEntity") && j.contains("img2imgEntity") && j.contains("editEntity")) {
            Deserialize(j);
            return;
        }
        try {
            EntityID target = GetCurrentEntity();
            if (target != 0) {
                m_entityManager.DeserializeEntity(j, txt2imgEntity);
                if (j.contains("components")) {
                    nlohmann::json data = m_entityManager.SerializeEntity(txt2imgEntity);
                    float imgDenoise = 0.6f, editDenoise = 0.6f;
                    if (m_entityManager.HasComponent<SamplerComponent>(img2imgEntity))
                        imgDenoise = m_entityManager.GetComponent<SamplerComponent>(img2imgEntity).denoise;
                    if (m_entityManager.HasComponent<SamplerComponent>(editEntity))
                        editDenoise = m_entityManager.GetComponent<SamplerComponent>(editEntity).denoise;
                    m_entityManager.DeserializeEntity(data, img2imgEntity);
                    m_entityManager.DeserializeEntity(data, editEntity);
                    if (m_entityManager.HasComponent<SamplerComponent>(img2imgEntity))
                        m_entityManager.GetComponent<SamplerComponent>(img2imgEntity).denoise = imgDenoise;
                    if (m_entityManager.HasComponent<SamplerComponent>(editEntity))
                        m_entityManager.GetComponent<SamplerComponent>(editEntity).denoise = editDenoise;
                }
            }
            if (j.contains("componentVisibility"))
                componentVisibility = j["componentVisibility"];
            if (j.contains("currentMode"))
                currentMode = j["currentMode"];
        }
        catch (...) {}
    }

}