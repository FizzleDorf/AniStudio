#pragma once
#include "Base/BaseView.hpp"
#include "Base/ViewManager.hpp"
#include "imgui.h"
#include <string>
#include <vector>

namespace GUI {

class ViewListManagerView : public BaseView {
public:
    ViewListManagerView(ECS::EntityManager &entityMgr, ViewManager &viewMgr)
        : BaseView(entityMgr), viewManager(viewMgr), selectedViewList(-1), selectedActiveView(-1),
          selectedInactiveView(-1) {
        viewName = "ViewList Manager";
    }

    void Init() override { RefreshViewLists(); }

    void RefreshViewLists() {
        // Get all ViewLists
        viewLists = viewManager.GetAllViews();

        // Reset selection indexes when refreshing
        selectedViewList = viewLists.empty() ? -1 : 0;
        selectedActiveView = -1;
        selectedInactiveView = -1;
    }

    void Render() override {
        ImGui::Begin("ViewList Manager");

        // Left column: ViewList selection
        if (ImGui::BeginChild("ViewLists", ImVec2(150, 0), true)) {
            if (ImGui::Button("New ViewList")) {
                ViewListID newList = viewManager.CreateView();
                RefreshViewLists();
                selectedViewList = viewLists.size() - 1;
            }

            ImGui::Separator();

            for (size_t i = 0; i < viewLists.size(); i++) {
                char buf[32];
                snprintf(buf, sizeof(buf), "ViewList %zu", viewLists[i]);
                if (ImGui::Selectable(buf, selectedViewList == static_cast<int>(i))) {
                    selectedViewList = i;
                    selectedActiveView = -1;
                    selectedInactiveView = -1;
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Middle column: Active views
        if (ImGui::BeginChild("Active Views", ImVec2(200, 0), true)) {
            ImGui::Text("Active Views");
            ImGui::Separator();

            if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
                ViewListID currentList = viewLists[selectedViewList];
                const auto &signatures = viewManager.GetViewSignatures();
                auto it = signatures.find(currentList);

                if (it != signatures.end()) {
                    std::vector<std::string> activeViews;
                    for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
                        if (it->second->count(typeId) > 0) {
                            activeViews.push_back(name);
                        }
                    }

                    for (size_t i = 0; i < activeViews.size(); i++) {
                        if (ImGui::Selectable(activeViews[i].c_str(), selectedActiveView == static_cast<int>(i))) {
                            selectedActiveView = i;
                            selectedInactiveView = -1;
                        }
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Controls between lists
        if (ImGui::BeginChild("Controls", ImVec2(50, 0), false)) {
            bool canAdd = selectedInactiveView >= 0;
            bool canRemove = selectedActiveView >= 0;

            ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);

            if (ImGui::Button(">>", ImVec2(30, 25))) {
                if (canAdd)
                    AddSelectedView();
            }
            if (ImGui::Button(">", ImVec2(30, 25))) {
                if (canAdd)
                    AddSelectedView();
            }
            if (ImGui::Button("<", ImVec2(30, 25))) {
                if (canRemove)
                    RemoveSelectedView();
            }
            if (ImGui::Button("<<", ImVec2(30, 25))) {
                if (canRemove)
                    RemoveSelectedView();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Right column: Available views to add
        if (ImGui::BeginChild("Available Views", ImVec2(200, 0), true)) {
            ImGui::Text("Available Views");
            ImGui::Separator();

            if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
                ViewListID currentList = viewLists[selectedViewList];
                const auto &signatures = viewManager.GetViewSignatures();
                auto it = signatures.find(currentList);

                if (it != signatures.end()) {
                    std::vector<std::string> availableViews;
                    for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
                        if (it->second->count(typeId) == 0) {
                            availableViews.push_back(name);
                        }
                    }

                    for (size_t i = 0; i < availableViews.size(); i++) {
                        if (ImGui::Selectable(availableViews[i].c_str(), selectedInactiveView == static_cast<int>(i))) {
                            selectedInactiveView = i;
                            selectedActiveView = -1;
                        }
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

private:
    void AddSelectedView() {
        if (selectedViewList >= 0 && selectedViewList < viewLists.size() && selectedInactiveView >= 0) {
            ViewListID currentList = viewLists[selectedViewList];

            std::vector<std::string> availableViews;
            const auto &signatures = viewManager.GetViewSignatures();
            auto it = signatures.find(currentList);

            if (it != signatures.end()) {
                for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
                    if (it->second->count(typeId) == 0) {
                        availableViews.push_back(name);
                    }
                }

                if (selectedInactiveView < availableViews.size()) {
                    std::string viewName = availableViews[selectedInactiveView];
                    ViewTypeID typeId = viewManager.GetViewType(viewName);
                    viewManager.AddViewByType(currentList, typeId);
                    selectedInactiveView = -1;
                }
            }
        }
    }

    void RemoveSelectedView() {
        if (selectedViewList >= 0 && selectedViewList < viewLists.size() && selectedActiveView >= 0) {
            ViewListID currentList = viewLists[selectedViewList];

            std::vector<std::string> activeViews;
            const auto &signatures = viewManager.GetViewSignatures();
            auto it = signatures.find(currentList);

            if (it != signatures.end()) {
                for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
                    if (it->second->count(typeId) > 0) {
                        activeViews.push_back(name);
                    }
                }

                if (selectedActiveView < activeViews.size()) {
                    std::string viewName = activeViews[selectedActiveView];
                    ViewTypeID typeId = viewManager.GetViewType(viewName);
                    viewManager.RemoveViewByType(currentList, typeId);
                    selectedActiveView = -1;
                }
            }
        }
    }

private:
    ViewManager &viewManager;
    std::vector<ViewListID> viewLists;
    int selectedViewList;
    int selectedActiveView;
    int selectedInactiveView;
};

} // namespace GUI