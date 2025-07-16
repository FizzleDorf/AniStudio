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

		WorkspaceView(ECS::EntityManager &entityMgr, ViewManager &viewMgr)
			: BaseView(entityMgr), viewManager(viewMgr), selectedWorkspace(-1) {
			viewName = "WorkspaceView";
		}

		void Init() override { RefreshWorkspaces(); }

		void RefreshWorkspaces() {
			// Get all active workspaces
			workspaces = viewManager.GetAllWorkspaces();

			// Reset selection index when refreshing
			selectedWorkspace = workspaces.empty() ? -1 : 0;
			selectedActiveViews.clear();
			selectedAvailableViews.clear();
		}

		void Render() override {
			ImGui::Begin("Workspace Manager");

			// Top section: Workspace management
			if (ImGui::Button("New Workspace")) {
				WorkspaceID newWorkspace = viewManager.CreateView();
				RefreshWorkspaces();
				selectedWorkspace = static_cast<int>(workspaces.size()) - 1;
			}

			ImGui::SameLine();
			if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size()) &&
				ImGui::Button("Remove Selected Workspace")) {
				viewManager.DestroyView(workspaces[selectedWorkspace]);
				RefreshWorkspaces();
				if (selectedWorkspace >= static_cast<int>(workspaces.size())) {
					selectedWorkspace = workspaces.empty() ? -1 : static_cast<int>(workspaces.size()) - 1;
				}
			}

			ImGui::Separator();

			// Left column: Workspace selection
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

			// Middle column: Active views in selected workspace
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

			// Controls between lists
			if (ImGui::BeginChild("Controls", ImVec2(60, 0), false)) {
				bool canAdd = !selectedAvailableViews.empty();
				bool canRemove = !selectedActiveViews.empty();

				ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);

				// Move all views to active
				if (ImGui::Button("Add All", ImVec2(50, 25))) {
					MoveAllToActive();
				}

				// Move selected views to active
				if (ImGui::Button("Add >>", ImVec2(50, 25))) {
					if (canAdd) {
						MoveSelectedToActive();
					}
				}

				// Move selected views to inactive
				if (ImGui::Button("<< Remove", ImVec2(50, 25))) {
					if (canRemove) {
						MoveSelectedToInactive();
					}
				}

				// Move all views to inactive
				if (ImGui::Button("Remove All", ImVec2(50, 25))) {
					MoveAllToInactive();
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// Right column: Available views
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

			// Bottom section: Workspace info
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
			const auto &signatures = viewManager.GetWorkspaceSignatures();
			auto it = signatures.find(workspaceId);

			if (it != signatures.end()) {
				for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
					if (it->second->count(typeId) == 0) {
						availableViews.push_back(name);
					}
				}
			}
			else {
				// If workspace has no signature yet, all views are available
				for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
					availableViews.push_back(name);
				}
			}
			return availableViews;
		}

		std::vector<std::string> GetActiveViews(WorkspaceID workspaceId) {
			std::vector<std::string> activeViews;
			const auto &signatures = viewManager.GetWorkspaceSignatures();
			auto it = signatures.find(workspaceId);

			if (it != signatures.end()) {
				for (const auto &[name, typeId] : viewManager.GetRegisteredViews()) {
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
						ViewTypeID typeId = viewManager.GetViewType(availableViews[index]);
						viewManager.AddViewByType(currentWorkspace, typeId);
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
						ViewTypeID typeId = viewManager.GetViewType(activeViews[index]);
						viewManager.RemoveViewByType(currentWorkspace, typeId);
					}
				}
				selectedActiveViews.clear();
			}
		}

		void MoveAllToActive() {
			if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
				WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
				auto availableViews = GetAvailableViews(currentWorkspace);

				for (const auto &viewName : availableViews) {
					ViewTypeID typeId = viewManager.GetViewType(viewName);
					viewManager.AddViewByType(currentWorkspace, typeId);
				}
				selectedAvailableViews.clear();
				selectedActiveViews.clear();
			}
		}

		void MoveAllToInactive() {
			if (selectedWorkspace >= 0 && selectedWorkspace < static_cast<int>(workspaces.size())) {
				WorkspaceID currentWorkspace = workspaces[selectedWorkspace];
				auto activeViews = GetActiveViews(currentWorkspace);

				for (const auto &viewName : activeViews) {
					ViewTypeID typeId = viewManager.GetViewType(viewName);
					viewManager.RemoveViewByType(currentWorkspace, typeId);
				}
				selectedAvailableViews.clear();
				selectedActiveViews.clear();
			}
		}

	private:
		ViewManager &viewManager;
		std::vector<WorkspaceID> workspaces;
		int selectedWorkspace;
		size_t lastSelectedActiveView = static_cast<size_t>(-1);
		std::unordered_set<size_t> selectedActiveViews;
		std::unordered_set<size_t> selectedAvailableViews;
	};

} // namespace GUI