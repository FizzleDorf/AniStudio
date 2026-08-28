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
            , selectedContextKey("")
            , showConfirmDialog(false)
            , confirmAction(ConfirmAction::None)
            , m_loadEntityId(0)
            , showMemoryErrorDialog(false)
            , memoryErrorRetryPending(false) {
            viewName = "Model Cache";
        }

        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Model Cache",
            "category": "Diffusion",
            "description": "View loaded model contexts and their memory usage."
        })";
        }

        void Init() override {
            RefreshContextList();
        }

        void Update(float deltaT) override {
            static float refreshTimer = 0.0f;
            refreshTimer += deltaT;
            if (refreshTimer >= 2.0f) {
                RefreshContextList();
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

            if (showMemoryErrorDialog) {
                RenderMemoryErrorDialog();
            }
        }

        nlohmann::json Serialize() const override {
            auto j = BaseView::Serialize();
            j["selectedContextKey"] = selectedContextKey;
            j["loadEntityId"] = m_loadEntityId;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseView::Deserialize(j);
            if (j.contains("selectedContextKey")) {
                selectedContextKey = j["selectedContextKey"].get<std::string>();
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

        std::vector<ECS::ContextDetail> contextDetails;
        std::string selectedContextKey;
        bool showConfirmDialog;
        ConfirmAction confirmAction;
        std::string confirmMessage;
        ECS::EntityID m_loadEntityId = 0;

        bool showMemoryErrorDialog = false;
        std::string memoryErrorMessage;
        bool memoryErrorRetryPending = false;
        std::string memoryErrorFailedKey;

        void RefreshContextList() {
            auto cacheSystem = m_entityManager.GetSystem<ECS::ModelCacheSystem>();
            if (cacheSystem) {
                contextDetails = cacheSystem->GetContextDetails();
                if (!selectedContextKey.empty()) {
                    auto it = std::find_if(contextDetails.begin(), contextDetails.end(),
                        [this](const ECS::ContextDetail& d) { return d.key == selectedContextKey; });
                    if (it == contextDetails.end()) {
                        selectedContextKey.clear();
                    }
                }
            }
        }

        void RenderContent();
        void RenderLoadFromEntity();
        void RenderContextTable();
        void RenderCacheActions();
        void RenderConfirmationDialog();
        void RenderMemoryErrorDialog();
        void ShowMemoryErrorDialog(const std::string& error, const std::string& failedKey = "");
        void ShowConfirmationDialog(ConfirmAction action, const std::string& message);
        void ExecuteConfirmedAction();
        std::string ExtractDisplayName(const std::string& key) const;
        std::string FormatMemory(size_t bytes) const;
        std::string GetModelType(const ECS::ContextDetail& detail) const;
        void HelpMarker(const char* desc);
    };

} // namespace GUI