// ConvertView.cpp
#include "ConvertView.hpp"
#include "Events.hpp"
#include "DiffusionOptions.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "UISchema.hpp"
#include "SDcppSystem.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <map>

using namespace ECS;
using namespace ANI;

namespace GUI {

    ConvertView::ConvertView(ECS::EntityManager& entityMgr, ImGuiContext* mainContext) : BaseView(entityMgr) {
        viewName = "ConvertView";
        windowOpen = true;
    }

    void ConvertView::Init() {
        GUI::DiffusionCallbackUtils::InitializeCallbacks();
        InitializeEntity();
    }

    void ConvertView::InitializeEntity() {
        if (convertEntity != 0 && mgr.IsEntityValid(convertEntity)) {
            mgr.DestroyEntity(convertEntity);
        }

        convertEntity = mgr.AddNewEntity();

        mgr.AddComponent<CheckpointComponent>(convertEntity);
        mgr.AddComponent<VaeComponent>(convertEntity);
        mgr.AddComponent<SamplerComponent>(convertEntity);
        mgr.AddComponent<ConversionComponent>(convertEntity);

        if (mgr.HasComponent<CheckpointComponent>(convertEntity)) {
            auto& checkpoint = mgr.GetComponent<CheckpointComponent>(convertEntity);
            checkpoint.modelPath = "";
            checkpoint.modelName = "";
        }

        if (mgr.HasComponent<VaeComponent>(convertEntity)) {
            auto& vae = mgr.GetComponent<VaeComponent>(convertEntity);
            vae.modelPath = "";
            vae.modelName = "";
        }

        if (mgr.HasComponent<SamplerComponent>(convertEntity)) {
            auto& sampler = mgr.GetComponent<SamplerComponent>(convertEntity);
            sampler.current_type_method = sd_type_t::SD_TYPE_COUNT;
        }

        if (mgr.HasComponent<ConversionComponent>(convertEntity)) {
            auto& conversion = mgr.GetComponent<ConversionComponent>(convertEntity);
            conversion.tensorTypeRules = "";
            conversion.convertName = true;
        }

        auto componentIds = mgr.GetEntityComponents(convertEntity);
        for (ComponentTypeID compId : componentIds) {
            auto* component = mgr.GetComponentById(convertEntity, compId);
            if (component) {
                component->RefreshSchema();
            }
        }
    }

    std::vector<std::string> ConvertView::GetCategoryRenderOrder() const {
        return {
            "Models",
            "Sampling",
            "Tools"
        };
    }

    void ConvertView::RenderEntityComponents() {
        if (convertEntity == 0 || !mgr.IsEntityValid(convertEntity)) {
            InitializeEntity();
            return;
        }

        auto componentIds = mgr.GetEntityComponents(convertEntity);
        std::map<std::string, std::vector<std::pair<ComponentTypeID, std::string>>> categorizedComponents;

        for (ComponentTypeID compId : componentIds) {
            std::string componentName = mgr.GetComponentNameById(compId);
            auto* component = mgr.GetComponentByIdConst(convertEntity, compId);
            if (!component) continue;

            std::string category = std::string(component->compCategory).empty() ? "Uncategorized" : component->compCategory;
            categorizedComponents[category].emplace_back(compId, componentName);
        }

        std::vector<std::string> renderOrder = GetCategoryRenderOrder();

        for (const auto& category : renderOrder) {
            auto it = categorizedComponents.find(category);
            if (it != categorizedComponents.end()) {
                if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (const auto& [compId, componentName] : it->second) {
                        RenderComponent(compId, componentName);
                    }
                }
                categorizedComponents.erase(it);
            }
        }

        for (const auto& [category, components] : categorizedComponents) {
            if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const auto& [compId, componentName] : components) {
                    RenderComponent(compId, componentName);
                }
            }
        }
    }

    void ConvertView::RenderComponent(ComponentTypeID compId, const std::string& componentName) {
        if (componentVisibility.find(componentName) == componentVisibility.end()) {
            componentVisibility[componentName] = true;
        }

        bool isVisible = componentVisibility[componentName];
        if (ImGui::Checkbox(componentName.c_str(), &isVisible)) {
            componentVisibility[componentName] = isVisible;
        }

        if (!isVisible) return;

        ImGui::Indent();

        auto* component = mgr.GetComponentById(convertEntity, compId);
        if (component && !component->schema.empty()) {
            try {
                auto properties = component->GetPropertyMap();
                UISchema::RenderSchema(component->schema, properties);

                if (componentName == "Sampler") {
                    if (mgr.HasComponent<SamplerComponent>(convertEntity)) {
                        auto& sampler = mgr.GetComponent<SamplerComponent>(convertEntity);
                        const char* typeName = sd_type_name(static_cast<sd_type_t>(sampler.current_type_method));
                        ImGui::Text("Quantization: %s", typeName);
                    }
                }
                else if (componentName == "Conversion") {
                    if (mgr.HasComponent<ConversionComponent>(convertEntity)) {
                        auto& conversion = mgr.GetComponent<ConversionComponent>(convertEntity);
                        ImGui::TextDisabled("Tensor rules: %s",
                            conversion.tensorTypeRules.empty() ? "None" : conversion.tensorTypeRules.c_str());
                        ImGui::TextDisabled("Convert names: %s",
                            conversion.convertName ? "Yes" : "No");
                    }
                }
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering %s: %s",
                    componentName.c_str(), e.what());
            }
        }

        ImGui::Unindent();
    }

    void ConvertView::Render() {
        RenderQueueList();

        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
            RenderEntityComponents();

            ImGui::Separator();

            if (ImGui::Button("Convert to GGUF", ImVec2(-FLT_MIN, 40))) {
                Convert();
            }
        }
        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void ConvertView::Convert() {
        auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
        if (!sdSystem) {
            mgr.RegisterSystem<ECS::SDCPPSystem>();
            sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
        }

        if (!sdSystem) {
            std::cerr << "Failed to create SDCPPSystem!" << std::endl;
            return;
        }

        nlohmann::json entityData = mgr.SerializeEntity(convertEntity);
        EntityID newEntity = mgr.DeserializeEntity(entityData);

        if (newEntity == 0) {
            std::cerr << "Failed to create conversion entity!" << std::endl;
            return;
        }

        std::cout << "Successfully cloned entity " << convertEntity << " to " << newEntity << " via serialization" << std::endl;

        auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Conversion);
        ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
    }

    void ConvertView::RenderQueueList() {
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Convert Queue")) {
            const auto& progressData = DiffusionCallbackUtils::GetProgressData();
            int currentStep = progressData.currentStep;
            int totalSteps = progressData.totalSteps;
            float time = progressData.currentTime;
            bool isProcessing = progressData.isProcessing;

            if (isProcessing && totalSteps > 0) {
                float progress = static_cast<float>(currentStep) / totalSteps;
                std::ostringstream ss;
                ss << "Processing: " << currentStep << "/" << totalSteps << " steps ("
                    << std::fixed << std::setprecision(1) << time << "s)";
                ImGui::Text("%s", ss.str().c_str());
                ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
            }
            else {
                ImGui::Text("Waiting...");
                ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
            }

            ImGui::Separator();

            ImGui::SetNextItemWidth(100);
            if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
                if (numQueues < 1) {
                    numQueues = 1;
                }
            }

            if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
                for (int i = 0; i < numQueues; i++) {
                    Convert();
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

            auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
            if (sdSystem && ImGui::BeginTable("Queue", 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {

                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

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
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

}