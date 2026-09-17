#pragma once

#include "BaseView.hpp"
#include "ModelCacheSystem.hpp"
#include "SDCPPParamFill.hpp"
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
            , selectedUpscalerKey("")
            , showConfirmDialog(false)
            , confirmAction(ConfirmAction::None)
            , m_loadEntityId(0)
            , showDetailsDialog(false) {
            viewName = "Model Cache";
        }

        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Model Cache",
            "category": "Diffusion",
            "description": "View loaded model contexts and upscalers as a tree."
        })";
        }

        void Init() override {
            RefreshLists();
        }

        void Update(float deltaT) override {
            static float refreshTimer = 0.0f;
            refreshTimer += deltaT;
            if (refreshTimer >= 2.0f) {
                RefreshLists();
                refreshTimer = 0.0f;
            }
        }

        void Render() override {
            if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
                RenderContent();
            }
            ImGui::End();

            if (showConfirmDialog) RenderConfirmationDialog();
            if (showDetailsDialog) RenderDetailsDialog();
        }

        nlohmann::json Serialize() const override {
            auto j = BaseView::Serialize();
            j["selectedContextKey"] = selectedContextKey;
            j["selectedUpscalerKey"] = selectedUpscalerKey;
            j["loadEntityId"] = m_loadEntityId;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseView::Deserialize(j);
            if (j.contains("selectedContextKey"))
                selectedContextKey = j["selectedContextKey"].get<std::string>();
            if (j.contains("selectedUpscalerKey"))
                selectedUpscalerKey = j["selectedUpscalerKey"].get<std::string>();
            if (j.contains("loadEntityId"))
                m_loadEntityId = j["loadEntityId"];
        }

    private:
        enum class ConfirmAction {
            None,
            ClearAll,
            UnloadAll,
            UnloadSelectedContext,
            UnloadSelectedUpscaler
        };

        std::vector<ECS::ContextDetail>  contextDetails;
        std::vector<ECS::UpscalerDetail> upscalerDetails;

        std::string selectedContextKey;
        std::string selectedUpscalerKey;

        bool showConfirmDialog;
        ConfirmAction confirmAction;
        std::string confirmMessage;

        ECS::EntityID m_loadEntityId = 0;

        // Details dialog state
        bool showDetailsDialog = false;
        std::string detailsTitle;
        std::vector<std::pair<std::string, std::string>> detailsRows;

        void RefreshLists();
        void RenderContent();
        void RenderTree();
        void RenderContextLeaf(const ECS::ContextDetail& d);
        void RenderUpscalerLeaf(const ECS::UpscalerDetail& d);
        void RenderControls();
        void RenderConfirmationDialog();
        void RenderDetailsDialog();
        void ShowConfirm(ConfirmAction a, const std::string& msg);
        void ExecuteConfirmedAction();
        void ShowDetailsForContext(const std::string& key);
        void ShowDetailsForUpscaler(const std::string& key);

        std::string ExtractDisplayName(const std::string& key) const;
        std::string FormatMemory(size_t bytes) const;
        std::string GetModelType(const ECS::ContextDetail& d) const;
        void HelpMarker(const char* desc);
    };

} // namespace GUI