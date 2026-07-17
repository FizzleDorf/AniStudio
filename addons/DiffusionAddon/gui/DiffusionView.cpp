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

    DiffusionView::DiffusionView(EntityManager& entityMgr, ImGuiContext* mainContext) : BaseView(entityMgr) {
        viewName = "DiffusionView";
        windowOpen = true;
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(entityMgr);
    }

    DiffusionView::~DiffusionView() {
        QuickSave();
        if (txt2imgEntity != 0) mgr.DestroyEntity(txt2imgEntity);
        if (img2imgEntity != 0) mgr.DestroyEntity(img2imgEntity);
        if (editEntity != 0) mgr.DestroyEntity(editEntity);
    }

    void DiffusionView::Init() {
        GUI::DiffusionCallbackUtils::InitializeCallbacks();
        ResetEntities();
        QuickLoad();
    }

    void DiffusionView::ResetEntities() {
        if (txt2imgEntity != 0) mgr.DestroyEntity(txt2imgEntity);
        if (img2imgEntity != 0) mgr.DestroyEntity(img2imgEntity);
        if (editEntity != 0) mgr.DestroyEntity(editEntity);

        txt2imgEntity = CreateEntityWithComponents(false);
        img2imgEntity = CreateEntityWithComponents(true);
        editEntity = CreateEntityWithComponents(true);

        if (mgr.HasComponent<SamplerComponent>(img2imgEntity)) {
            mgr.GetComponent<SamplerComponent>(img2imgEntity).denoise = 0.6f;
        }
        if (mgr.HasComponent<SamplerComponent>(editEntity)) {
            mgr.GetComponent<SamplerComponent>(editEntity).denoise = 0.6f;
        }
    }

    ECS::EntityID DiffusionView::CreateEntityWithComponents(bool includeInputImage) {
        EntityID entity = mgr.AddNewEntity();

        mgr.AddComponent<CheckpointComponent>(entity);
        mgr.AddComponent<LatentComponent>(entity);
        mgr.AddComponent<SamplerComponent>(entity);
        mgr.AddComponent<GuidanceComponent>(entity);
        mgr.AddComponent<ClipSkipComponent>(entity);
        mgr.AddComponent<PromptComponent>(entity);
        mgr.AddComponent<OutputImageComponent>(entity);

        if (includeInputImage) {
            mgr.AddComponent<InputImageComponent>(entity);
        }

        return entity;
    }

    bool DiffusionView::IsEntitySafeToUse(ECS::EntityID entity) const {
        return mgr.IsEntityValid(entity);
    }

    void DiffusionView::ToggleComponent(EntityID entity, const std::string& name) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;
        if (name == "InputImage") {
            if (mgr.HasComponent<InputImageComponent>(entity))
                mgr.RemoveComponent<InputImageComponent>(entity);
            else
                mgr.AddComponent<InputImageComponent>(entity);
            return;
        }
        if (name == "CheckpointComponent") {
            if (mgr.HasComponent<CheckpointComponent>(entity))
                mgr.RemoveComponent<CheckpointComponent>(entity);
            else
                mgr.AddComponent<CheckpointComponent>(entity);
        }
        else if (name == "DiffusionModelComponent") {
            if (mgr.HasComponent<DiffusionModelComponent>(entity))
                mgr.RemoveComponent<DiffusionModelComponent>(entity);
            else
                mgr.AddComponent<DiffusionModelComponent>(entity);
        }
        else if (name == "ClipLComponent") {
            if (mgr.HasComponent<ClipLComponent>(entity))
                mgr.RemoveComponent<ClipLComponent>(entity);
            else
                mgr.AddComponent<ClipLComponent>(entity);
        }
        else if (name == "ClipGComponent") {
            if (mgr.HasComponent<ClipGComponent>(entity))
                mgr.RemoveComponent<ClipGComponent>(entity);
            else
                mgr.AddComponent<ClipGComponent>(entity);
        }
        else if (name == "T5XXLComponent") {
            if (mgr.HasComponent<T5XXLComponent>(entity))
                mgr.RemoveComponent<T5XXLComponent>(entity);
            else
                mgr.AddComponent<T5XXLComponent>(entity);
        }
        else if (name == "ClipVisionComponent") {
            if (mgr.HasComponent<ClipVisionComponent>(entity))
                mgr.RemoveComponent<ClipVisionComponent>(entity);
            else
                mgr.AddComponent<ClipVisionComponent>(entity);
        }
        else if (name == "LlmEncoderComponent") {
            if (mgr.HasComponent<LlmEncoderComponent>(entity))
                mgr.RemoveComponent<LlmEncoderComponent>(entity);
            else
                mgr.AddComponent<LlmEncoderComponent>(entity);
        }
        else if (name == "LlmVisionComponent") {
            if (mgr.HasComponent<LlmVisionComponent>(entity))
                mgr.RemoveComponent<LlmVisionComponent>(entity);
            else
                mgr.AddComponent<LlmVisionComponent>(entity);
        }
        else if (name == "VaeComponent") {
            if (mgr.HasComponent<VaeComponent>(entity))
                mgr.RemoveComponent<VaeComponent>(entity);
            else
                mgr.AddComponent<VaeComponent>(entity);
        }
        else if (name == "TaesdComponent") {
            if (mgr.HasComponent<TaesdComponent>(entity))
                mgr.RemoveComponent<TaesdComponent>(entity);
            else
                mgr.AddComponent<TaesdComponent>(entity);
        }
        else if (name == "LoraComponent") {
            if (mgr.HasComponent<LoraComponent>(entity))
                mgr.RemoveComponent<LoraComponent>(entity);
            else
                mgr.AddComponent<LoraComponent>(entity);
        }
        else if (name == "ControlNetComponent") {
            if (mgr.HasComponent<ControlNetComponent>(entity))
                mgr.RemoveComponent<ControlNetComponent>(entity);
            else
                mgr.AddComponent<ControlNetComponent>(entity);
        }
        else if (name == "EmbeddingComponent") {
            if (mgr.HasComponent<EmbeddingComponent>(entity))
                mgr.RemoveComponent<EmbeddingComponent>(entity);
            else
                mgr.AddComponent<EmbeddingComponent>(entity);
        }
        else if (name == "PhotoMakerComponent") {
            if (mgr.HasComponent<PhotoMakerComponent>(entity))
                mgr.RemoveComponent<PhotoMakerComponent>(entity);
            else
                mgr.AddComponent<PhotoMakerComponent>(entity);
        }
        else if (name == "StackedIdEmbedComponent") {
            if (mgr.HasComponent<StackedIdEmbedComponent>(entity))
                mgr.RemoveComponent<StackedIdEmbedComponent>(entity);
            else
                mgr.AddComponent<StackedIdEmbedComponent>(entity);
        }
        else if (name == "LatentComponent") {
            if (mgr.HasComponent<LatentComponent>(entity))
                mgr.RemoveComponent<LatentComponent>(entity);
            else
                mgr.AddComponent<LatentComponent>(entity);
        }
        else if (name == "SamplerComponent") {
            if (mgr.HasComponent<SamplerComponent>(entity))
                mgr.RemoveComponent<SamplerComponent>(entity);
            else
                mgr.AddComponent<SamplerComponent>(entity);
        }
        else if (name == "GuidanceComponent") {
            if (mgr.HasComponent<GuidanceComponent>(entity))
                mgr.RemoveComponent<GuidanceComponent>(entity);
            else
                mgr.AddComponent<GuidanceComponent>(entity);
        }
        else if (name == "ClipSkipComponent") {
            if (mgr.HasComponent<ClipSkipComponent>(entity))
                mgr.RemoveComponent<ClipSkipComponent>(entity);
            else
                mgr.AddComponent<ClipSkipComponent>(entity);
        }
        else if (name == "PromptComponent") {
            if (mgr.HasComponent<PromptComponent>(entity))
                mgr.RemoveComponent<PromptComponent>(entity);
            else
                mgr.AddComponent<PromptComponent>(entity);
        }
        else if (name == "LayerSkipComponent") {
            if (mgr.HasComponent<LayerSkipComponent>(entity))
                mgr.RemoveComponent<LayerSkipComponent>(entity);
            else
                mgr.AddComponent<LayerSkipComponent>(entity);
        }
        else if (name == "ChromaComponent") {
            if (mgr.HasComponent<ChromaComponent>(entity))
                mgr.RemoveComponent<ChromaComponent>(entity);
            else
                mgr.AddComponent<ChromaComponent>(entity);
        }
        else if (name == "SLGComponent") {
            if (mgr.HasComponent<SLGComponent>(entity))
                mgr.RemoveComponent<SLGComponent>(entity);
            else
                mgr.AddComponent<SLGComponent>(entity);
        }
        else if (name == "EasyCacheComponent") {
            if (mgr.HasComponent<EasyCacheComponent>(entity))
                mgr.RemoveComponent<EasyCacheComponent>(entity);
            else
                mgr.AddComponent<EasyCacheComponent>(entity);
        }
        else if (name == "ConversionComponent") {
            if (mgr.HasComponent<ConversionComponent>(entity))
                mgr.RemoveComponent<ConversionComponent>(entity);
            else
                mgr.AddComponent<ConversionComponent>(entity);
        }
    }

    bool DiffusionView::IsComponentPresent(EntityID entity, const std::string& name) const {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return false;
        if (name == "InputImage")
            return mgr.HasComponent<InputImageComponent>(entity);
        if (name == "CheckpointComponent")
            return mgr.HasComponent<CheckpointComponent>(entity);
        if (name == "DiffusionModelComponent")
            return mgr.HasComponent<DiffusionModelComponent>(entity);
        if (name == "ClipLComponent")
            return mgr.HasComponent<ClipLComponent>(entity);
        if (name == "ClipGComponent")
            return mgr.HasComponent<ClipGComponent>(entity);
        if (name == "T5XXLComponent")
            return mgr.HasComponent<T5XXLComponent>(entity);
        if (name == "ClipVisionComponent")
            return mgr.HasComponent<ClipVisionComponent>(entity);
        if (name == "LlmEncoderComponent")
            return mgr.HasComponent<LlmEncoderComponent>(entity);
        if (name == "LlmVisionComponent")
            return mgr.HasComponent<LlmVisionComponent>(entity);
        if (name == "VaeComponent")
            return mgr.HasComponent<VaeComponent>(entity);
        if (name == "TaesdComponent")
            return mgr.HasComponent<TaesdComponent>(entity);
        if (name == "LoraComponent")
            return mgr.HasComponent<LoraComponent>(entity);
        if (name == "ControlNetComponent")
            return mgr.HasComponent<ControlNetComponent>(entity);
        if (name == "EmbeddingComponent")
            return mgr.HasComponent<EmbeddingComponent>(entity);
        if (name == "PhotoMakerComponent")
            return mgr.HasComponent<PhotoMakerComponent>(entity);
        if (name == "StackedIdEmbedComponent")
            return mgr.HasComponent<StackedIdEmbedComponent>(entity);
        if (name == "LatentComponent")
            return mgr.HasComponent<LatentComponent>(entity);
        if (name == "SamplerComponent")
            return mgr.HasComponent<SamplerComponent>(entity);
        if (name == "GuidanceComponent")
            return mgr.HasComponent<GuidanceComponent>(entity);
        if (name == "ClipSkipComponent")
            return mgr.HasComponent<ClipSkipComponent>(entity);
        if (name == "PromptComponent")
            return mgr.HasComponent<PromptComponent>(entity);
        if (name == "LayerSkipComponent")
            return mgr.HasComponent<LayerSkipComponent>(entity);
        if (name == "ChromaComponent")
            return mgr.HasComponent<ChromaComponent>(entity);
        if (name == "SLGComponent")
            return mgr.HasComponent<SLGComponent>(entity);
        if (name == "EasyCacheComponent")
            return mgr.HasComponent<EasyCacheComponent>(entity);
        if (name == "ConversionComponent")
            return mgr.HasComponent<ConversionComponent>(entity);
        return false;
    }

    void DiffusionView::SetModelMode(EntityID entity, bool useCheckpoint) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;
        if (useCheckpoint) {
            if (!mgr.HasComponent<CheckpointComponent>(entity))
                mgr.AddComponent<CheckpointComponent>(entity);
            mgr.RemoveComponent<DiffusionModelComponent>(entity);
            mgr.RemoveComponent<ClipLComponent>(entity);
            mgr.RemoveComponent<ClipGComponent>(entity);
            mgr.RemoveComponent<T5XXLComponent>(entity);
            mgr.RemoveComponent<ClipVisionComponent>(entity);
            mgr.RemoveComponent<LlmEncoderComponent>(entity);
            mgr.RemoveComponent<LlmVisionComponent>(entity);
            mgr.RemoveComponent<VaeComponent>(entity);
            mgr.RemoveComponent<TaesdComponent>(entity);
        }
        else {
            mgr.RemoveComponent<CheckpointComponent>(entity);
            if (!mgr.HasComponent<DiffusionModelComponent>(entity))
                mgr.AddComponent<DiffusionModelComponent>(entity);
            if (!mgr.HasComponent<ClipLComponent>(entity))
                mgr.AddComponent<ClipLComponent>(entity);
            if (!mgr.HasComponent<ClipGComponent>(entity))
                mgr.AddComponent<ClipGComponent>(entity);
            if (!mgr.HasComponent<T5XXLComponent>(entity))
                mgr.AddComponent<T5XXLComponent>(entity);
            if (!mgr.HasComponent<ClipVisionComponent>(entity))
                mgr.AddComponent<ClipVisionComponent>(entity);
            if (!mgr.HasComponent<LlmEncoderComponent>(entity))
                mgr.AddComponent<LlmEncoderComponent>(entity);
            if (!mgr.HasComponent<LlmVisionComponent>(entity))
                mgr.AddComponent<LlmVisionComponent>(entity);
            if (!mgr.HasComponent<VaeComponent>(entity))
                mgr.AddComponent<VaeComponent>(entity);
            if (!mgr.HasComponent<TaesdComponent>(entity))
                mgr.AddComponent<TaesdComponent>(entity);
        }
    }

    bool DiffusionView::IsCheckpointMode(EntityID entity) const {
        return mgr.HasComponent<CheckpointComponent>(entity);
    }

    void DiffusionView::ResetToDefaultComponents(EntityID entity) {
        std::vector<std::string> all = mgr.GetAllRegisteredComponentNames();
        for (const std::string& name : all) {
            if (name == "InputImage") continue;
            if (name == "CheckpointComponent" || name == "LatentComponent" ||
                name == "SamplerComponent" || name == "GuidanceComponent" ||
                name == "ClipSkipComponent" || name == "PromptComponent" ||
                name == "OutputImageComponent") {
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

                std::vector<std::string> all = mgr.GetAllRegisteredComponentNames();
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
                    bool hasInput = mgr.HasComponent<InputImageComponent>(current);
                    if (ImGui::MenuItem("InputImage", nullptr, hasInput))
                        ToggleComponent(current, "InputImage");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void DiffusionView::RenderComponent(EntityID entity, ComponentTypeID compId, const std::string& name) {
        auto* comp = mgr.GetComponentById(entity, compId);
        if (!comp || comp->schema.empty()) return;
        try {
            auto props = comp->GetPropertyMap();
            if (name == "Latent") {
                UISchema::RenderSchema(comp->schema, props);
                if (mgr.HasComponent<LatentComponent>(entity)) {
                    auto& l = mgr.GetComponent<LatentComponent>(entity);
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
                if (mgr.HasComponent<InputImageComponent>(entity)) {
                    auto& in = mgr.GetComponent<InputImageComponent>(entity);
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
                            if (mgr.HasComponent<LatentComponent>(entity)) {
                                auto& l = mgr.GetComponent<LatentComponent>(entity);
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

        auto componentIds = mgr.GetEntityComponents(entity);
        for (ComponentTypeID compId : componentIds) {
            std::string name = mgr.GetComponentNameById(compId);
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
        EntityID newEntity = mgr.CloneEntity(txt2imgEntity);
        if (newEntity == 0) return;
        auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
        if (sdSystem)
            sdSystem->QueueTask(newEntity, ECS::SDCPPSystem::TaskType::Inference);
        else
            mgr.DestroyEntity(newEntity);
    }

    void DiffusionView::HandleI2IEvent() {
        EntityID newEntity = mgr.CloneEntity(img2imgEntity);
        if (newEntity == 0) return;
        if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
            auto& out = mgr.GetComponent<OutputImageComponent>(newEntity);
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
        EntityID newEntity = mgr.CloneEntity(editEntity);
        if (newEntity == 0) return;
        if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
            auto& out = mgr.GetComponent<OutputImageComponent>(newEntity);
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
                if (mgr.HasComponent<LoraComponent>(target)) {
                    auto& lora = mgr.GetComponent<LoraComponent>(target);
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
            if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0)))
                ANI::Events::Ref().QueueEvent("StopCurrentDiffusionTask");
            if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0)))
                ANI::Events::Ref().QueueEvent("ClearDiffusionQueue");
            ImGui::Separator();

            if (ImGui::BeginTable("Queue", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
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
        j["txt2imgEntity"] = mgr.SerializeEntity(txt2imgEntity);
        j["img2imgEntity"] = mgr.SerializeEntity(img2imgEntity);
        j["editEntity"] = mgr.SerializeEntity(editEntity);
        j["componentVisibility"] = componentVisibility;
        j["currentMode"] = currentMode;
        j["numQueues"] = numQueues;
        j["isPaused"] = isPaused;
        return j;
    }

    void DiffusionView::Deserialize(const nlohmann::json& j) {
        try {
            if (j.contains("txt2imgEntity"))
                mgr.DeserializeEntity(j["txt2imgEntity"], txt2imgEntity);
            if (j.contains("img2imgEntity"))
                mgr.DeserializeEntity(j["img2imgEntity"], img2imgEntity);
            if (j.contains("editEntity"))
                mgr.DeserializeEntity(j["editEntity"], editEntity);
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
            std::string dataPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("ProjectDataPath") : "";
            if (dataPath.empty())
                dataPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
            if (dataPath.empty()) return;
            std::filesystem::create_directories(dataPath);
            std::string filename = viewName + "_" + std::to_string(GetID()) + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            SaveMetadataToJson(filepath);
        }
        catch (...) {}
    }

    void DiffusionView::QuickLoad() {
        try {
            std::string dataPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("ProjectDataPath") : "";
            if (dataPath.empty())
                dataPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
            if (dataPath.empty()) return;
            std::string filename = viewName + "_" + std::to_string(GetID()) + ".json";
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
                mgr.DeserializeEntity(j, txt2imgEntity);
                if (j.contains("components")) {
                    nlohmann::json data = mgr.SerializeEntity(txt2imgEntity);
                    float imgDenoise = 0.6f, editDenoise = 0.6f;
                    if (mgr.HasComponent<SamplerComponent>(img2imgEntity))
                        imgDenoise = mgr.GetComponent<SamplerComponent>(img2imgEntity).denoise;
                    if (mgr.HasComponent<SamplerComponent>(editEntity))
                        editDenoise = mgr.GetComponent<SamplerComponent>(editEntity).denoise;
                    mgr.DeserializeEntity(data, img2imgEntity);
                    mgr.DeserializeEntity(data, editEntity);
                    if (mgr.HasComponent<SamplerComponent>(img2imgEntity))
                        mgr.GetComponent<SamplerComponent>(img2imgEntity).denoise = imgDenoise;
                    if (mgr.HasComponent<SamplerComponent>(editEntity))
                        mgr.GetComponent<SamplerComponent>(editEntity).denoise = editDenoise;
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