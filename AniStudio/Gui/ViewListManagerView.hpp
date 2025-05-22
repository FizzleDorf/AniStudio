/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once
#include "Base/BaseView.hpp"
#include "Base/ViewManager.hpp"
#include "imgui.h"
#include "pch.h"

namespace GUI {

class ViewListManagerView : public BaseView {
public:
    ViewListManagerView(ECS::EntityManager &entityMgr, ViewManager &viewMgr)
        : BaseView(entityMgr), viewManager(viewMgr), selectedViewList(-1), selectedActiveView(-1),
          selectedInactiveView(-1) {
        viewName = "ViewListManagerView";
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

            if (selectedViewList >= 0 && ImGui::Button("Remove Selected")) {
                viewManager.DestroyView(viewLists[selectedViewList]);
                RefreshViewLists();
                if (selectedViewList >= viewLists.size()) {
                    selectedViewList = viewLists.empty() ? -1 : viewLists.size() - 1;
                }
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
                        bool isSelected = selectedActiveViews.count(i) > 0;
                        if (ImGui::Selectable(activeViews[i].c_str(), isSelected,
                                              ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (ImGui::GetIO().KeyShift && lastSelectedActiveView != -1) {
                                size_t start = std::min(lastSelectedActiveView, i);
                                size_t end = std::max(lastSelectedActiveView, i);
                                for (size_t j = start; j <= end; j++) {
                                    selectedActiveViews.insert(j);
                                }
                            } else if (ImGui::GetIO().KeyCtrl) {
                                if (isSelected) {
                                    selectedActiveViews.erase(i);
                                } else {
                                    selectedActiveViews.insert(i);
                                }
                            } else {
                                selectedActiveViews.clear();
                                selectedActiveViews.insert(i);
                            }
                            lastSelectedActiveView = i;
                            selectedInactiveViews.clear();
                        }
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Controls between lists
        if (ImGui::BeginChild("Controls", ImVec2(50, 0), false)) {
            bool canAdd = !selectedInactiveViews.empty();
            bool canRemove = !selectedActiveViews.empty();

            ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);

            // Move all views to active
            if (ImGui::Button("<<", ImVec2(30, 25))) {
                MoveAllToActive();
            }

            // Move selected views to active
            if (ImGui::Button("<", ImVec2(30, 25))) {
                if (canAdd) {
                    MoveSelectedToActive();
                }
            }

            // Move selected views to inactive
            if (ImGui::Button(">", ImVec2(30, 25))) {
                if (canRemove) {
                    MoveSelectedToInactive();
                }
            }

            // Move all views to inactive
            if (ImGui::Button(">>", ImVec2(30, 25))) {
                MoveAllToInactive();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Right column: Available views
        if (ImGui::BeginChild("Available Views", ImVec2(200, 0), true)) {
            ImGui::Text("Available Views");
            ImGui::Separator();

            auto availableViews = GetAvailableViews();
            for (size_t i = 0; i < availableViews.size(); i++) {
                bool isSelected = selectedInactiveViews.count(i) > 0;
                if (ImGui::Selectable(availableViews[i].c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::GetIO().KeyCtrl) {
                        if (isSelected) {
                            selectedInactiveViews.erase(i);
                        } else {
                            selectedInactiveViews.insert(i);
                        }
                    } else {
                        selectedInactiveViews.clear();
                        selectedInactiveViews.insert(i);
                    }
                    selectedActiveViews.clear();
                }
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

private:
    std::vector<std::string> GetAvailableViews() {
        std::vector<std::string> availableViews;
        if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
            ViewListID currentList = viewLists[selectedViewList];
            const auto &signatures = viewManager.GetViewSignatures();
            auto it = signatures.find(currentList);

            if (it != signatures.end()) {
                for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
                    if (it->second->count(typeId) == 0) {
                        availableViews.push_back(name);
                    }
                }
            }
        }
        return availableViews;
    }

    std::vector<std::string> GetActiveViews() {
        std::vector<std::string> activeViews;
        if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
            ViewListID currentList = viewLists[selectedViewList];
            const auto &signatures = viewManager.GetViewSignatures();
            auto it = signatures.find(currentList);

            if (it != signatures.end()) {
                for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
                    if (it->second->count(typeId) > 0) {
                        activeViews.push_back(name);
                    }
                }
            }
        }
        return activeViews;
    }

    void MoveSelectedToActive() {
        if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
            auto availableViews = GetAvailableViews();
            for (size_t index : selectedInactiveViews) {
                if (index < availableViews.size()) {
                    ViewTypeID typeId = viewManager.GetViewType(availableViews[index]);
                    viewManager.AddViewByType(viewLists[selectedViewList], typeId);
                }
            }
            selectedInactiveViews.clear();
            RefreshViewLists(); // Refresh the UI after changes
        }
    }

    void MoveSelectedToInactive() {
        if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
            auto activeViews = GetActiveViews();
            for (size_t index : selectedActiveViews) {
                if (index < activeViews.size()) {
                    ViewTypeID typeId = viewManager.GetViewType(activeViews[index]);
                    viewManager.RemoveViewByType(viewLists[selectedViewList], typeId);
                }
            }
            selectedActiveViews.clear();
            RefreshViewLists(); // Refresh the UI after changes
        }
    }

    void MoveAllToActive() {
        if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
            auto availableViews = GetAvailableViews();
            for (const auto &viewName : availableViews) {
                ViewTypeID typeId = viewManager.GetViewType(viewName);
                viewManager.AddViewByType(viewLists[selectedViewList], typeId);
            }
            selectedInactiveViews.clear();
            selectedActiveViews.clear();
            RefreshViewLists(); // Refresh the UI after changes
        }
    }

    void MoveAllToInactive() {
        if (selectedViewList >= 0 && selectedViewList < viewLists.size()) {
            auto activeViews = GetActiveViews();
            for (const auto &viewName : activeViews) {
                ViewTypeID typeId = viewManager.GetViewType(viewName);
                viewManager.RemoveViewByType(viewLists[selectedViewList], typeId);
            }
            selectedInactiveViews.clear();
            selectedActiveViews.clear();
            RefreshViewLists(); // Refresh the UI after changes
        }
    }

private:
    ViewManager &viewManager;
    std::vector<ViewListID> viewLists;
    size_t selectedViewList;
    size_t selectedActiveView;
    size_t selectedInactiveView;
    size_t lastSelectedActiveView = -1;
    size_t lastSelectedInactiveView = -1;
    std::unordered_set<size_t> selectedActiveViews;
    std::unordered_set<size_t> selectedInactiveViews;
};

} // namespace GUI