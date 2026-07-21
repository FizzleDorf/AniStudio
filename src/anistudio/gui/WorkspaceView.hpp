#pragma once
#include "GUI.h"
#include "pch.h"
#include "imgui.h"
#include "ViewTypes.hpp"

namespace GUI {
    class ViewManager;
}

namespace GUI {

    class WorkspaceView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Workspace Manager",
            "category": "Views",
            "description": "Manage and debug active workspaces and views."
        })";
        }

        WorkspaceView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm), selectedWorkspace(-1) {
            viewName = "WorkspaceView";
        }

        void Init() override { RefreshWorkspaces(); }

        void RefreshWorkspaces() {
            workspaces = GetViewManager().GetAllWorkspaces();
            selectedWorkspace = workspaces.empty() ? -1 : 0;
            selectedActiveViews.clear();
            selectedAvailableViews.clear();
        }

        void Render() override {
            ImGui::Begin("Workspace Manager");

            if (ImGui::Button("New Workspace")) {
                WorkspaceID newWorkspace = GetViewManager().CreateView();
                RefreshWorkspaces();
                selectedWorkspace = static_cast<int>(workspaces.size()) - 1;
            }

            ImGui::SameLine();
            if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size()) &&
                ImGui::Button("Remove Selected Workspace")) {
                GetViewManager().DestroyView(workspaces[selectedWorkspace]);
                RefreshWorkspaces();
                if (selectedWorkspace >= static_cast<int>(workspaces.size())) {
                    selectedWorkspace = workspaces.empty() ? -1 : static_cast<int>(workspaces.size()) - 1;
                }
            }

            ImGui::Separator();

            if (ImGui::BeginChild("Workspaces", ImVec2(150, 0), true)) {
                ImGui::Text("Workspaces");
                ImGui::Separator();

                for (size_t i = 0; i < workspaces.size(); i++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "Workspace %zu", workspaces[i]);
                    if (ImGui::Selectable(buf, selectedWorkspace == static_cast<int>(i))) {
                        selectedWorkspace = static_cast<int>(i);
                        selectedActiveViews.clear();
                        selectedAvailableViews.clear();
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("Active Views", ImVec2(200, 0), true)) {
                ImGui::Text("Active Views");
                ImGui::Separator();

                if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                    WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                    auto activeViews = GetActiveViews(currentWorkspace);

                    for (size_t i = 0; i < activeViews.size(); i++) {
                        bool isSelected = selectedActiveViews.count(i) > 0;
                        if (ImGui::Selectable(activeViews[i].c_str(), isSelected)) {
                            if (ImGui::GetIO().KeyShift && lastSelectedActiveView != static_cast<size_t>(-1)) {
                                size_t start = std::min(lastSelectedActiveView, i);
                                size_t end = std::max(lastSelectedActiveView, i);
                                for (size_t j = start; j <= end; j++) {
                                    selectedActiveViews.insert(j);
                                }
                            }
                            else if (ImGui::GetIO().KeyCtrl) {
                                if (isSelected) {
                                    selectedActiveViews.erase(i);
                                }
                                else {
                                    selectedActiveViews.insert(i);
                                }
                            }
                            else {
                                selectedActiveViews.clear();
                                selectedActiveViews.insert(i);
                            }
                            lastSelectedActiveView = i;
                            selectedAvailableViews.clear();
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("Controls", ImVec2(60, 0), false)) {
                bool canAdd = !selectedAvailableViews.empty();
                bool canRemove = !selectedActiveViews.empty();

                ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);

                if (ImGui::Button("Add All", ImVec2(50, 25))) {
                    MoveAllToActive();
                }

                if (ImGui::Button("Add >>", ImVec2(50, 25))) {
                    if (canAdd) {
                        MoveSelectedToActive();
                    }
                }

                if (ImGui::Button("<< Remove", ImVec2(50, 25))) {
                    if (canRemove) {
                        MoveSelectedToInactive();
                    }
                }

                if (ImGui::Button("Remove All", ImVec2(50, 25))) {
                    MoveAllToInactive();
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("Available Views", ImVec2(200, 0), true)) {
                ImGui::Text("Available Views");
                ImGui::Separator();

                if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                    WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                    auto availableViews = GetAvailableViews(currentWorkspace);

                    for (size_t i = 0; i < availableViews.size(); i++) {
                        bool isSelected = selectedAvailableViews.count(i) > 0;
                        if (ImGui::Selectable(availableViews[i].c_str(), isSelected)) {
                            if (ImGui::GetIO().KeyCtrl) {
                                if (isSelected) {
                                    selectedAvailableViews.erase(i);
                                }
                                else {
                                    selectedAvailableViews.insert(i);
                                }
                            }
                            else {
                                selectedAvailableViews.clear();
                                selectedAvailableViews.insert(i);
                            }
                            selectedActiveViews.clear();
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                auto activeViews = GetActiveViews(currentWorkspace);
                auto availableViews = GetAvailableViews(currentWorkspace);

                ImGui::Text("Workspace ID: %zu", currentWorkspace);
                ImGui::SameLine();
                ImGui::Text("Active Views: %zu", activeViews.size());
                ImGui::SameLine();
                ImGui::Text("Available Views: %zu", availableViews.size());
            }

            ImGui::End();
        }

    private:
        std::vector<std::string> GetAvailableViews(WorkspaceID workspaceId) {
            std::vector<std::string> availableViews;
            const auto& signatures = GetViewManager().GetWorkspaceSignatures();
            auto it = signatures.find(workspaceId);

            if (it != signatures.end()) {
                for (const auto& [name, typeId] : GetViewManager().GetRegisteredViews()) {
                    if (it->second->count(typeId) == 0) {
                        availableViews.push_back(name);
                    }
                }
            }
            else {
                for (const auto& [name, typeId] : GetViewManager().GetRegisteredViews()) {
                    availableViews.push_back(name);
                }
            }
            return availableViews;
        }

        std::vector<std::string> GetActiveViews(WorkspaceID workspaceId) {
            std::vector<std::string> activeViews;
            const auto& signatures = GetViewManager().GetWorkspaceSignatures();
            auto it = signatures.find(workspaceId);

            if (it != signatures.end()) {
                for (const auto& [name, typeId] : GetViewManager().GetRegisteredViews()) {
                    if (it->second->count(typeId) > 0) {
                        activeViews.push_back(name);
                    }
                }
            }
            return activeViews;
        }

        void MoveSelectedToActive() {
            if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                auto availableViews = GetAvailableViews(currentWorkspace);

                for (size_t index : selectedAvailableViews) {
                    if (index < availableViews.size()) {
                        ViewTypeID typeId = GetViewManager().GetViewType(availableViews[index]);
                        GetViewManager().AddViewByType(currentWorkspace, typeId);
                    }
                }
                selectedAvailableViews.clear();
            }
        }

        void MoveSelectedToInactive() {
            if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                auto activeViews = GetActiveViews(currentWorkspace);

                for (size_t index : selectedActiveViews) {
                    if (index < activeViews.size()) {
                        ViewTypeID typeId = GetViewManager().GetViewType(activeViews[index]);
                        GetViewManager().RemoveViewByType(currentWorkspace, typeId);
                    }
                }
                selectedActiveViews.clear();
            }
        }

        void MoveAllToActive() {
            if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                auto availableViews = GetAvailableViews(currentWorkspace);

                for (const auto& viewName : availableViews) {
                    ViewTypeID typeId = GetViewManager().GetViewType(viewName);
                    GetViewManager().AddViewByType(currentWorkspace, typeId);
                }
                selectedAvailableViews.clear();
                selectedActiveViews.clear();
            }
        }

        void MoveAllToInactive() {
            if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
                WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
                auto activeViews = GetActiveViews(currentWorkspace);

                for (const auto& viewName : activeViews) {
                    ViewTypeID typeId = GetViewManager().GetViewType(viewName);
                    GetViewManager().RemoveViewByType(currentWorkspace, typeId);
                }
                selectedAvailableViews.clear();
                selectedActiveViews.clear();
            }
        }

    private:
        std::vector<WorkspaceID> workspaces;
        int selectedWorkspace;
        size_t lastSelectedActiveView = static_cast<size_t>(-1);
        std::unordered_set<size_t> selectedActiveViews;
        std::unordered_set<size_t> selectedAvailableViews;
    };

} // namespace GUI