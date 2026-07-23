#pragma once

#include "BaseView.hpp"
#include "ModelCacheSystem.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <algorithm>

namespace GUI {

    class ModelCacheView : public BaseView {
    public:
        ModelCacheView(ECS::EntityManager& m_entityManager, ViewManager& vm)
            : BaseView(m_entityManager, vm)
            , selectedModelIndex(-1)
            , showConfirmDialog(false)
            , confirmAction(ConfirmAction::None)
            , m_loadEntityId(0) {
            viewName = "Model Cache";
        }

        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Model Cache",
            "category": "System",
            "description": "Manage loaded models and cache settings."
        })";
        }

        void Init() override {
            RefreshModelList();
        }

        void Update(float deltaT) override {
            static float refreshTimer = 0.0f;
            refreshTimer += deltaT;
            if (refreshTimer >= 2.0f) {
                RefreshModelList();
                refreshTimer = 0.0f;
            }
        }

        void Render() override {
            if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
                RenderContent();
            }
            ImGui::End();

            if (showConfirmDialog) {
                RenderConfirmationDialog();
            }
        }

        nlohmann::json Serialize() const override {
            auto j = BaseView::Serialize();
            j["selectedModelIndex"] = selectedModelIndex;
            j["loadEntityId"] = m_loadEntityId;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseView::Deserialize(j);
            if (j.contains("selectedModelIndex")) {
                selectedModelIndex = j["selectedModelIndex"];
            }
            if (j.contains("loadEntityId")) {
                m_loadEntityId = j["loadEntityId"];
            }
        }

    private:
        enum class ConfirmAction {
            None,
            ClearAll,
            ClearSelected,
            UnloadAll
        };

        std::vector<std::string> loadedModels;
        int selectedModelIndex;
        bool showConfirmDialog;
        ConfirmAction confirmAction;
        std::string confirmMessage;
        ECS::EntityID m_loadEntityId = 0;

        void RefreshModelList() {
            auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
            if (cacheSystem) {
                loadedModels = cacheSystem->GetLoadedModels();
                if (selectedModelIndex >= static_cast<int>(loadedModels.size())) {
                    selectedModelIndex = -1;
                }
            }
        }

        void RenderContent() {
            auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
            if (!cacheSystem) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ModelCacheSystem not available!");
                return;
            }

            ImGui::Separator();

            ImGui::Text("Load Model from Entity");
            ImGui::InputInt("Entity ID", reinterpret_cast<int*>(&m_loadEntityId));
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                if (m_loadEntityId != 0 && m_entityManager.IsEntityValid(m_loadEntityId)) {
                    nlohmann::json metadata = m_entityManager.SerializeEntity(m_loadEntityId);
                    cacheSystem->loadModelFromMetadata(metadata);
                    RefreshModelList();
                }
            }
            ImGui::SameLine();
            HelpMarker("Enter the Entity ID of a Diffusion view or any entity with model components, then click Load.");

            ImGui::Separator();
            RenderLoadedModels(cacheSystem);
            ImGui::Separator();
            RenderCacheActions(cacheSystem);
        }

        void RenderLoadedModels(std::shared_ptr<ECS::ModelCacheSystem> cacheSystem) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Loaded Models");
            if (loadedModels.empty()) {
                ImGui::Text("No models currently loaded");
                return;
            }

            ImGui::BeginChild("ModelList", ImVec2(0, 200), true);
            for (size_t i = 0; i < loadedModels.size(); ++i) {
                bool isSelected = (selectedModelIndex == static_cast<int>(i));
                std::string displayText = loadedModels[i];
                ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                std::string statusPrefix = "  ";
                if (displayText.find("[In Use]") != std::string::npos || displayText.find("processing") != std::string::npos) {
                    color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                    statusPrefix = "> ";
                }
                else if (displayText.find("[Loading]") != std::string::npos) {
                    color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);
                    statusPrefix = "~ ";
                }
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                if (ImGui::Selectable((statusPrefix + displayText).c_str(), isSelected)) {
                    selectedModelIndex = static_cast<int>(i);
                }
                ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Unload Model")) {
                        std::string modelPath = ExtractModelPath(loadedModels[i]);
                        if (!modelPath.empty()) {
                            cacheSystem->UnloadModel(modelPath);
                            RefreshModelList();
                        }
                    }
                    if (ImGui::MenuItem("Reload Model")) {
                        std::string modelPath = ExtractModelPath(loadedModels[i]);
                        if (!modelPath.empty()) {
                            cacheSystem->reloadModel(modelPath);
                            RefreshModelList();
                        }
                    }
                    if (ImGui::MenuItem("Force Reload")) {
                        std::string modelPath = ExtractModelPath(loadedModels[i]);
                        if (!modelPath.empty()) {
                            cacheSystem->reloadModel(modelPath);
                            RefreshModelList();
                        }
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Model: %s", loadedModels[i].c_str());
                    ImGui::Text("Status: %s", color.x == 0.4f ? "Active" : (color.x == 1.0f ? "Idle" : "Loading"));
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndChild();

            if (selectedModelIndex >= 0 && selectedModelIndex < static_cast<int>(loadedModels.size())) {
                ImGui::Text("Selected: %s", loadedModels[selectedModelIndex].c_str());
                ImGui::SameLine();
                if (ImGui::Button("Reload Selected")) {
                    std::string modelPath = ExtractModelPath(loadedModels[selectedModelIndex]);
                    if (!modelPath.empty()) {
                        cacheSystem->reloadModel(modelPath);
                        RefreshModelList();
                    }
                }
            }
            else {
                ImGui::Text("No model selected");
            }
        }

        void RenderCacheActions(std::shared_ptr<ECS::ModelCacheSystem> cacheSystem) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Cache Actions");

            if (ImGui::Button("Unload Selected Model")) {
                if (selectedModelIndex >= 0 && selectedModelIndex < static_cast<int>(loadedModels.size())) {
                    std::string modelPath = ExtractModelPath(loadedModels[selectedModelIndex]);
                    if (!modelPath.empty()) {
                        cacheSystem->UnloadModel(modelPath);
                        RefreshModelList();
                        selectedModelIndex = -1;
                    }
                }
            }
            ImGui::SameLine(); HelpMarker("Unloads the selected model if not currently in use");

            ImGui::SameLine();
            if (ImGui::Button("Unload All Models")) {
                ShowConfirmationDialog(ConfirmAction::UnloadAll,
                    "Are you sure you want to unload ALL models?\n\n"
                    "This will remove all cached models from memory.\n"
                    "Any running tasks will continue with their current model.");
            }
            ImGui::SameLine(); HelpMarker("Unloads all models that are not currently in use");

            if (ImGui::Button("Force Unload Idle Models")) {
                ShowConfirmationDialog(ConfirmAction::ClearAll,
                    "Are you sure you want to force unload ALL idle models?\n\n"
                    "This will remove ALL cached contexts, even if they're\n"
                    "marked as in use (except for currently running tasks).\n"
                    "Use with caution!");
            }
            ImGui::SameLine(); HelpMarker("Forcefully unloads all idle models (aggressive cleanup)");

            ImGui::SameLine();
            if (ImGui::Button("List Contexts")) {
                cacheSystem->ListSDContexts();
            }
            ImGui::SameLine(); HelpMarker("Prints detailed cache info to console");

            if (ImGui::Button("Refresh List")) {
                RefreshModelList();
            }
        }

        void RenderConfirmationDialog() {
            if (!showConfirmDialog) return;
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
            if (ImGui::Begin("Confirm Action", &showConfirmDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                ImGui::TextWrapped("%s", confirmMessage.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 100);
                if (ImGui::Button("Confirm", ImVec2(100, 0))) {
                    ExecuteConfirmedAction();
                    showConfirmDialog = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                    showConfirmDialog = false;
                }
                ImGui::End();
            }
        }

        void ShowConfirmationDialog(ConfirmAction action, const std::string& message) {
            confirmAction = action;
            confirmMessage = message;
            showConfirmDialog = true;
        }

        void ExecuteConfirmedAction() {
            auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
            if (!cacheSystem) return;
            switch (confirmAction) {
            case ConfirmAction::ClearAll:
                cacheSystem->ForceUnloadIdleModels();
                break;
            case ConfirmAction::UnloadAll:
                cacheSystem->UnloadAllModels();
                break;
            case ConfirmAction::ClearSelected:
                if (selectedModelIndex >= 0 && selectedModelIndex < static_cast<int>(loadedModels.size())) {
                    std::string modelPath = ExtractModelPath(loadedModels[selectedModelIndex]);
                    if (!modelPath.empty()) cacheSystem->UnloadModel(modelPath);
                }
                break;
            default: break;
            }
            RefreshModelList();
        }

        std::string ExtractModelPath(const std::string& displayText) {
            size_t cachePos = displayText.find(" [Cache:");
            if (cachePos != std::string::npos) return displayText.substr(0, cachePos);
            return displayText;
        }

        void HelpMarker(const char* desc) {
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(desc);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    };

} // namespace GUI