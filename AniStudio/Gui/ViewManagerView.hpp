#pragma once
#include "Base/BaseView.hpp"
#include "ViewManager.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>

namespace GUI {

class ViewManagerView : public BaseView {
public:
    ViewManagerView(ECS::EntityManager &entityMgr, ViewManager &vMgr) : BaseView(entityMgr), viewManager(vMgr) {
        viewName = "View Manager";
    }

    void Init() override { LoadState(); }

    void Render() override {
        ImGui::Begin("View Manager");

        if (ImGui::BeginTable("ViewLists", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("View List ID");
            ImGui::TableSetupColumn("Active Views");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();

            auto viewLists = viewManager.GetAllViews();
            for (ViewListID listId : viewLists) {
                ImGui::TableNextRow();

                // View List ID
                ImGui::TableNextColumn();
                ImGui::Text("%zu", listId);

                // Active Views
                ImGui::TableNextColumn();
                RenderViewTypeControls(listId);

                // Actions
                ImGui::TableNextColumn();
                if (ImGui::Button(("Delete##" + std::to_string(listId)).c_str())) {
                    viewManager.DestroyView(listId);
                }
            }
            ImGui::EndTable();
        }

        if (ImGui::Button("Create New View List")) {
            viewManager.CreateView();
        }

        ImGui::End();
    }

private:
    void RenderViewTypeControls(ViewListID listId) {
        // Add DiffusionView button
        if (!viewManager.HasView<DiffusionView>(listId)) {
            if (ImGui::Button(("Add Diffusion View##" + std::to_string(listId)).c_str())) {
                viewManager.AddView<DiffusionView>(listId, DiffusionView(mgr));
            }
        }
        ImGui::SameLine();

        // Add ImageView button
        if (!viewManager.HasView<ImageView>(listId)) {
            if (ImGui::Button(("Add Image View##" + std::to_string(listId)).c_str())) {
                viewManager.AddView<ImageView>(listId, ImageView(mgr));
            }
        }
        ImGui::SameLine();

        // Add DebugView button
        if (!viewManager.HasView<DebugView>(listId)) {
            if (ImGui::Button(("Add Debug View##" + std::to_string(listId)).c_str())) {
                viewManager.AddView<DebugView>(listId, DebugView(mgr));
            }
        }
        ImGui::SameLine();

        // Add SettingsView button
        if (!viewManager.HasView<SettingsView>(listId)) {
            if (ImGui::Button(("Add Settings View##" + std::to_string(listId)).c_str())) {
                viewManager.AddView<SettingsView>(listId, SettingsView(mgr));
            }
        }
    }

    void SaveState() {
        nlohmann::json j;
        auto viewLists = viewManager.GetAllViews();
        for (ViewListID listId : viewLists) {
            nlohmann::json viewListData;
            viewListData["id"] = listId;

            // Save view type information
            viewListData["has_diffusion_view"] = viewManager.HasView<DiffusionView>(listId);
            viewListData["has_image_view"] = viewManager.HasView<ImageView>(listId);
            viewListData["has_debug_view"] = viewManager.HasView<DebugView>(listId);
            viewListData["has_settings_view"] = viewManager.HasView<SettingsView>(listId);

            j["viewLists"].push_back(viewListData);
        }

        std::filesystem::path configPath = "../data/view_manager_state.json";
        std::filesystem::create_directories(configPath.parent_path());

        std::ofstream file(configPath);
        if (file.is_open()) {
            file << j.dump(4);
        }
    }

    void LoadState() {
        std::filesystem::path configPath = "../data/view_manager_state.json";
        if (!std::filesystem::exists(configPath)) {
            return;
        }

        std::ifstream file(configPath);
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;

                for (const auto &viewListData : j["viewLists"]) {
                    ViewListID listId = viewManager.CreateView();

                    // Restore views based on saved state
                    if (viewListData["has_diffusion_view"]) {
                        viewManager.AddView<DiffusionView>(listId, DiffusionView(mgr));
                    }
                    if (viewListData["has_image_view"]) {
                        viewManager.AddView<ImageView>(listId, ImageView(mgr));
                    }
                    if (viewListData["has_debug_view"]) {
                        viewManager.AddView<DebugView>(listId, DebugView(mgr));
                    }
                    if (viewListData["has_settings_view"]) {
                        viewManager.AddView<SettingsView>(listId, SettingsView(mgr));
                    }
                }
            } catch (const std::exception &e) {
                std::cerr << "Error loading view manager state: " << e.what() << std::endl;
            }
        }
    }

private:
    ViewManager &viewManager;
};

} // namespace GUI