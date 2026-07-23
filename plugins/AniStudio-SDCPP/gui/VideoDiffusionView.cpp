// VideoDiffusionView.cpp
#include "VideoDiffusionView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "Events.hpp"
#include "DiffusionOptions.hpp"
#include "UISchema.hpp"
#include "PngMetadataUtils.hpp"
#include "utils.h"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace ECS;
using namespace ANI;

namespace GUI {

    void VideoDiffusionView::InitializeComponentVisibility() {
        componentVisibility["ModelComponent"] = true;
        componentVisibility["ClipLComponent"] = true;
        componentVisibility["ClipGComponent"] = true;
        componentVisibility["ClipVisionComponent"] = true;
        componentVisibility["T5XXLComponent"] = true;
        componentVisibility["DiffusionModelComponent"] = true;
        componentVisibility["HighNoiseDiffusionModelComponent"] = true;
        componentVisibility["VaeComponent"] = true;
        componentVisibility["LoraComponent"] = true;
        componentVisibility["TaesdComponent"] = true;
        componentVisibility["LatentComponent"] = true;
        componentVisibility["SamplerComponent"] = true;
        componentVisibility["HighNoiseSamplerComponent"] = true;
        componentVisibility["VideoParamsComponent"] = true;
        componentVisibility["GuidanceComponent"] = true;
        componentVisibility["PromptComponent"] = true;
        componentVisibility["OutputImageComponent"] = true;
        componentVisibility["InputImageComponent"] = true;
        componentVisibility["EndImageComponent"] = true;
        componentVisibility["ControlNetComponent"] = true;
        componentVisibility["EmbeddingComponent"] = true;
        componentVisibility["EsrganComponent"] = true;
    }

    void VideoDiffusionView::Init() {
        GUI::DiffusionCallbackUtils::InitializeCallbacks();
        ResetEntities();
    }

    void VideoDiffusionView::Update(float deltaT) {
    }

    void VideoDiffusionView::ResetEntities() {
        if (img2vidEntity != 0) {
            m_entityManager.DestroyEntity(img2vidEntity);
            img2vidEntity = 0;
        }

        img2vidEntity = m_entityManager.AddNewEntity();

        m_entityManager.AddComponent<ClipLComponent>(img2vidEntity);
        m_entityManager.AddComponent<ClipGComponent>(img2vidEntity);
        m_entityManager.AddComponent<ClipVisionComponent>(img2vidEntity);
        m_entityManager.AddComponent<T5XXLComponent>(img2vidEntity);
        m_entityManager.AddComponent<DiffusionModelComponent>(img2vidEntity);
        m_entityManager.AddComponent<HighNoiseDiffusionModelComponent>(img2vidEntity);
        m_entityManager.AddComponent<VaeComponent>(img2vidEntity);
        m_entityManager.AddComponent<LoraComponent>(img2vidEntity);
        m_entityManager.AddComponent<TaesdComponent>(img2vidEntity);
        m_entityManager.AddComponent<LatentComponent>(img2vidEntity);
        m_entityManager.AddComponent<SamplerComponent>(img2vidEntity);
        m_entityManager.AddComponent<HighNoiseSamplerComponent>(img2vidEntity);
        m_entityManager.AddComponent<VideoParamsComponent>(img2vidEntity);
        m_entityManager.AddComponent<GuidanceComponent>(img2vidEntity);
        m_entityManager.AddComponent<PromptComponent>(img2vidEntity);
        m_entityManager.AddComponent<OutputImageComponent>(img2vidEntity);
        m_entityManager.AddComponent<InputImageComponent>(img2vidEntity);

        m_entityManager.GetComponent<SamplerComponent>(img2vidEntity).denoise = 0.6f;
    }

    bool VideoDiffusionView::IsEntitySafeToUse(EntityID entity) const {
        return m_entityManager.IsEntityValid(entity);
    }

    void VideoDiffusionView::RenderComponentWithCheckbox(const EntityID entity, const std::string& componentName, const std::string& displayName, const std::function<void()>& renderFunc) {
        if (!componentVisibility.count(componentName)) {
            componentVisibility[componentName] = true;
        }

        bool isVisible = componentVisibility[componentName];
        if (ImGui::Checkbox(displayName.c_str(), &isVisible)) {
            componentVisibility[componentName] = isVisible;
        }

        if (isVisible) {
            ImGui::Indent();
            renderFunc();
            ImGui::Unindent();
        }
    }

    void VideoDiffusionView::RenderEntityComponents(const EntityID entity) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;

        if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTabBar("ModelTabs")) {
                if (ImGui::BeginTabItem("Full")) {
                    RenderComponentWithCheckbox(entity, "CheckpointComponent", "Checkpoint", [&]() {
                        if (m_entityManager.HasComponent<CheckpointComponent>(entity)) {
                            auto& comp = m_entityManager.GetComponent<CheckpointComponent>(entity);
                            if (!comp.schema.empty()) {
                                try {
                                    auto properties = comp.GetPropertyMap();
                                    UISchema::RenderSchema(comp.schema, properties);
                                }
                                catch (const std::exception& e) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering CheckpointComponent: %s", e.what());
                                }
                            }
                        }
                        });

                    RenderComponentWithCheckbox(entity, "VaeComponent", "VAE", [&]() {
                        if (m_entityManager.HasComponent<VaeComponent>(entity)) {
                            auto& comp = m_entityManager.GetComponent<VaeComponent>(entity);
                            if (!comp.schema.empty()) {
                                try {
                                    auto properties = comp.GetPropertyMap();
                                    UISchema::RenderSchema(comp.schema, properties);
                                }
                                catch (const std::exception& e) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering VaeComponent: %s", e.what());
                                }
                            }
                        }
                        });

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Split")) {
                    RenderComponentWithCheckbox(entity, "DiffusionModelComponent", "Diffusion Model", [&]() {
                        if (m_entityManager.HasComponent<DiffusionModelComponent>(entity)) {
                            auto& comp = m_entityManager.GetComponent<DiffusionModelComponent>(entity);
                            if (!comp.schema.empty()) {
                                try {
                                    auto properties = comp.GetPropertyMap();
                                    UISchema::RenderSchema(comp.schema, properties);
                                }
                                catch (const std::exception& e) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering DiffusionModelComponent: %s", e.what());
                                }
                            }
                        }
                        });

                    RenderComponentWithCheckbox(entity, "HighNoiseDiffusionModelComponent", "High Noise Model", [&]() {
                        if (m_entityManager.HasComponent<HighNoiseDiffusionModelComponent>(entity)) {
                            auto& comp = m_entityManager.GetComponent<HighNoiseDiffusionModelComponent>(entity);
                            if (!comp.schema.empty()) {
                                try {
                                    auto properties = comp.GetPropertyMap();
                                    UISchema::RenderSchema(comp.schema, properties);
                                }
                                catch (const std::exception& e) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering HighNoiseDiffusionModelComponent: %s", e.what());
                                }
                            }
                        }
                        });

                    if (ImGui::CollapsingHeader("Text Encoders", ImGuiTreeNodeFlags_DefaultOpen)) {
                        RenderComponentWithCheckbox(entity, "ClipLComponent", "CLIP-L", [&]() {
                            if (m_entityManager.HasComponent<ClipLComponent>(entity)) {
                                auto& comp = m_entityManager.GetComponent<ClipLComponent>(entity);
                                if (!comp.schema.empty()) {
                                    try {
                                        auto properties = comp.GetPropertyMap();
                                        UISchema::RenderSchema(comp.schema, properties);
                                    }
                                    catch (const std::exception& e) {
                                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipLComponent: %s", e.what());
                                    }
                                }
                            }
                            });

                        RenderComponentWithCheckbox(entity, "ClipGComponent", "CLIP-G", [&]() {
                            if (m_entityManager.HasComponent<ClipGComponent>(entity)) {
                                auto& comp = m_entityManager.GetComponent<ClipGComponent>(entity);
                                if (!comp.schema.empty()) {
                                    try {
                                        auto properties = comp.GetPropertyMap();
                                        UISchema::RenderSchema(comp.schema, properties);
                                    }
                                    catch (const std::exception& e) {
                                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipGComponent: %s", e.what());
                                    }
                                }
                            }
                            });

                        RenderComponentWithCheckbox(entity, "ClipVisionComponent", "CLIP Vision", [&]() {
                            if (m_entityManager.HasComponent<ClipVisionComponent>(entity)) {
                                auto& comp = m_entityManager.GetComponent<ClipVisionComponent>(entity);
                                if (!comp.schema.empty()) {
                                    try {
                                        auto properties = comp.GetPropertyMap();
                                        UISchema::RenderSchema(comp.schema, properties);
                                    }
                                    catch (const std::exception& e) {
                                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipVisionComponent: %s", e.what());
                                    }
                                }
                            }
                            });

                        RenderComponentWithCheckbox(entity, "T5XXLComponent", "T5-XXL", [&]() {
                            if (m_entityManager.HasComponent<T5XXLComponent>(entity)) {
                                auto& comp = m_entityManager.GetComponent<T5XXLComponent>(entity);
                                if (!comp.schema.empty()) {
                                    try {
                                        auto properties = comp.GetPropertyMap();
                                        UISchema::RenderSchema(comp.schema, properties);
                                    }
                                    catch (const std::exception& e) {
                                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering T5XXLComponent: %s", e.what());
                                    }
                                }
                            }
                            });
                    }

                    RenderComponentWithCheckbox(entity, "VaeComponent", "VAE", [&]() {
                        if (m_entityManager.HasComponent<VaeComponent>(entity)) {
                            auto& comp = m_entityManager.GetComponent<VaeComponent>(entity);
                            if (!comp.schema.empty()) {
                                try {
                                    auto properties = comp.GetPropertyMap();
                                    UISchema::RenderSchema(comp.schema, properties);
                                }
                                catch (const std::exception& e) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering VaeComponent: %s", e.what());
                                }
                            }
                        }
                        });

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }

        if (ImGui::CollapsingHeader("Video Generation Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderComponentWithCheckbox(entity, "LatentComponent", "Video Dimensions", [&]() {
                if (m_entityManager.HasComponent<LatentComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<LatentComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);

                            if (ImGui::Button("Swap Width/Height", ImVec2(-1.0f, 0))) {
                                int temp = comp.latentWidth;
                                comp.latentWidth = comp.latentHeight;
                                comp.latentHeight = temp;
                            }
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering LatentComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "VideoParamsComponent", "Video Parameters", [&]() {
                if (m_entityManager.HasComponent<VideoParamsComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<VideoParamsComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);

                            ImGui::Separator();
                            float duration = static_cast<float>(comp.video_frames) / comp.fps;
                            ImGui::Text("Video Duration: %.2f seconds", duration);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering VideoParamsComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "SamplerComponent", "Main Sampler", [&]() {
                if (m_entityManager.HasComponent<SamplerComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<SamplerComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering SamplerComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "HighNoiseSamplerComponent", "High Noise Sampler", [&]() {
                if (m_entityManager.HasComponent<HighNoiseSamplerComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<HighNoiseSamplerComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering HighNoiseSamplerComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "GuidanceComponent", "Guidance", [&]() {
                if (m_entityManager.HasComponent<GuidanceComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<GuidanceComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering GuidanceComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "PromptComponent", "Prompts", [&]() {
                if (m_entityManager.HasComponent<PromptComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<PromptComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering PromptComponent: %s", e.what());
                        }
                    }
                }
                });
        }

        if (ImGui::CollapsingHeader("Input/Output Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderComponentWithCheckbox(entity, "InputImageComponent", "Input Image", [&]() {
                if (m_entityManager.HasComponent<InputImageComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<InputImageComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering InputImageComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "OutputImageComponent", "Output Settings", [&]() {
                if (m_entityManager.HasComponent<OutputImageComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<OutputImageComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering OutputImageComponent: %s", e.what());
                        }
                    }
                }
                });
        }

        if (ImGui::CollapsingHeader("Advanced Options")) {
            RenderComponentWithCheckbox(entity, "LoraComponent", "LoRA", [&]() {
                if (m_entityManager.HasComponent<LoraComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<LoraComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering LoraComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "TaesdComponent", "TAESD", [&]() {
                if (m_entityManager.HasComponent<TaesdComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<TaesdComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering TaesdComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "ControlNetComponent", "ControlNet", [&]() {
                if (m_entityManager.HasComponent<ControlNetComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<ControlNetComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ControlNetComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "EmbeddingComponent", "Embeddings", [&]() {
                if (m_entityManager.HasComponent<EmbeddingComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<EmbeddingComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering EmbeddingComponent: %s", e.what());
                        }
                    }
                }
                });

            RenderComponentWithCheckbox(entity, "EsrganComponent", "ESRGAN Upscaler", [&]() {
                if (m_entityManager.HasComponent<EsrganComponent>(entity)) {
                    auto& comp = m_entityManager.GetComponent<EsrganComponent>(entity);
                    if (!comp.schema.empty()) {
                        try {
                            auto properties = comp.GetPropertyMap();
                            UISchema::RenderSchema(comp.schema, properties);
                        }
                        catch (const std::exception& e) {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering EsrganComponent: %s", e.what());
                        }
                    }
                }
                });
        }
    }

    void VideoDiffusionView::RenderComponentSchema(const EntityID entity, const std::string& componentName, BaseComponent* component) {
        if (!component || component->schema.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No schema available for %s", componentName.c_str());
            return;
        }

        try {
            auto properties = component->GetPropertyMap();
            UISchema::RenderSchema(component->schema, properties);
        }
        catch (const std::exception& e) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                "Error rendering %s: %s", componentName.c_str(), e.what());
        }
    }

    void VideoDiffusionView::UpdateModelPath(const EntityID entity, const std::string& componentName) {
    }

    void VideoDiffusionView::HandleImg2VidEvent() {
        std::cout << "Adding new Img2Vid entity..." << std::endl;

        EntityID newEntity = m_entityManager.CloneEntity(img2vidEntity);
        if (newEntity == 0) {
            std::cerr << "Failed to create new entity!" << std::endl;
            return;
        }

        if (m_entityManager.HasComponent<OutputImageComponent>(newEntity)) {
            auto& outputComp = m_entityManager.GetComponent<OutputImageComponent>(newEntity);
            if (outputComp.filePath.empty()) {
                outputComp.filePath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
            }
            if (outputComp.fileName.empty()) {
                outputComp.fileName = "AniStudio_video.mp4";
            }

            std::string filename = outputComp.fileName;
            size_t lastDot = filename.find_last_of('.');
            if (lastDot != std::string::npos) {
                filename = filename.substr(0, lastDot);
            }
            outputComp.fileName = filename + ".mp4";

            std::filesystem::create_directories(outputComp.filePath);
        }

        auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Img2Vid);
        ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
    }

    void VideoDiffusionView::RenderQueueList() {
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Video Queue")) {
            const ProgressData& progressData = DiffusionCallbackUtils::GetProgressData();
            int currentStep = progressData.currentStep;
            int totalSteps = progressData.totalSteps;
            float time = progressData.currentTime;
            bool isProcessing = progressData.isProcessing;

            if (isProcessing && totalSteps > 0) {
                float progress = static_cast<float>(currentStep) / totalSteps;
                std::ostringstream ss;
                ss << "Processing: " << currentStep << "/" << totalSteps << " steps (" << std::fixed << std::setprecision(1)
                    << time << "s)";
                ImGui::Text("%s", ss.str().c_str());
                ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
            }
            else {
                ImGui::Text("Waiting...");
                ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
            }
            ImGui::Separator();

            if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
                if (m_entityManager.HasComponent<LoraComponent>(img2vidEntity)) {
                    auto& loraComp = m_entityManager.GetComponent<LoraComponent>(img2vidEntity);
                    loraComp.modelPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("Lora") : "";
                }

                for (int i = 0; i < numQueues; i++) {
                    HandleImg2VidEvent();
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

            if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("StopCurrentDiffusionTask");
            }

            if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("ClearDiffusionQueue");
            }

            ImGui::Separator();

            if (ImGui::BeginTable("Queue", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                auto sdSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
                if (sdSystem) {
                    auto queueItems = sdSystem->GetQueueSnapshot();
                    for (size_t i = 0; i < queueItems.size(); i++) {
                        const auto& item = queueItems[i];

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(item.entityID));

                        ImGui::TableNextColumn();
                        if (item.processing) {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
                        }
                        else {
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
                        }

                        ImGui::TableNextColumn();
                        if (!item.processing) {
                            if (i > 0) {
                                if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
                                    auto moveData = std::make_pair(i, i - 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (i < queueItems.size() - 1) {
                                if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
                                    auto moveData = std::make_pair(i, i + 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (i > 0) {
                                if (ImGui::Button(("Top##Top" + std::to_string(i)).c_str())) {
                                    size_t targetIndex = queueItems[0].processing ? 1 : 0;
                                    auto moveData = std::make_pair(i, targetIndex);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (i < queueItems.size() - 1) {
                                if (ImGui::Button(("Bottom##Bottom" + std::to_string(i)).c_str())) {
                                    auto moveData = std::make_pair(i, queueItems.size() - 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (ImGui::Button(("X##Remove" + std::to_string(i)).c_str())) {
                                ANI::Events::Ref().QueueEventWithData("RemoveFromDiffusionQueue", i);
                            }
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void VideoDiffusionView::Render() {
        RenderQueueList();

        ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {

            if (ImGui::CollapsingHeader("Metadata Controls")) {
                RenderMetadataControls();
            }

            ImGui::Text("Image-to-Video Generation");
            ImGui::Separator();
            RenderEntityComponents(img2vidEntity);
        }
        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    nlohmann::json VideoDiffusionView::Serialize() const {
        nlohmann::json j = m_entityManager.SerializeEntity(img2vidEntity);
        j["componentVisibility"] = componentVisibility;
        return j;
    }

    void VideoDiffusionView::Deserialize(const nlohmann::json& j) {
        if (img2vidEntity == 0) {
            std::cerr << "Error: Invalid target entity for deserialization" << std::endl;
            return;
        }

        try {
            m_entityManager.DeserializeEntity(j, img2vidEntity);

            if (j.contains("componentVisibility")) {
                componentVisibility = j["componentVisibility"];
            }

            std::cout << "Successfully deserialized data to entity " << img2vidEntity << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during deserialization: " << e.what() << std::endl;
        }
    }

    void VideoDiffusionView::SaveMetadataToJson(const std::string& filepath) {
        try {
            nlohmann::json metadata = Serialize();
            std::ofstream file(filepath);
            if (file.is_open()) {
                file << metadata.dump(4);
                file.close();
                std::cout << "Video metadata saved to: " << filepath << std::endl;
            }
            else {
                std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving video metadata: " << e.what() << std::endl;
        }
    }

    void VideoDiffusionView::LoadMetadataFromJson(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json metadata;
                file >> metadata;
                Deserialize(metadata);
                file.close();
                std::cout << "Video metadata loaded from: " << filepath << std::endl;
            }
            else {
                std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading video metadata: " << e.what() << std::endl;
        }
    }

    void VideoDiffusionView::LoadMetadataFromVideo(const std::string& videoPath) {
        std::cout << "Attempting to load metadata from video: " << videoPath << std::endl;
    }

    void VideoDiffusionView::RenderMetadataControls() {
        std::string defaultPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";

        if (ImGui::Button("Save Video Metadata", ImVec2(-FLT_MIN, 0))) {
            std::string selectedFile;
            if (FileDialog::SaveFile("Save Video Metadata", FileDialog::FilterType::METADATA_FILE, "video_metadata.json", selectedFile, defaultPath)) {
                SaveMetadataToJson(selectedFile);
            }
        }

        if (ImGui::Button("Load Video Metadata", ImVec2(-FLT_MIN, 0))) {
            std::string selectedFile;
            if (FileDialog::OpenFile("Load Video Metadata", FileDialog::FilterType::METADATA_FILE, selectedFile, defaultPath)) {
                std::string extension = std::filesystem::path(selectedFile).extension().string();
                if (extension == ".json") {
                    LoadMetadataFromJson(selectedFile);
                }
                else if (extension == ".mp4" || extension == ".avi" || extension == ".mkv") {
                    LoadMetadataFromVideo(selectedFile);
                }
            }
        }
    }

}