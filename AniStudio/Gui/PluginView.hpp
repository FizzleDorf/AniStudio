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
 */

#pragma once

#include "Base/BaseView.hpp"
#include "PluginManager.hpp"
#include "ImGuiFileDialog.h"
#include <imgui.h>
#include <string>
#include <vector>

namespace GUI {

	class PluginView : public BaseView {
	public:
		PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr)
			: BaseView(entityMgr), pluginManager(pluginMgr) {
			viewName = "Plugin Manager";
		}

		~PluginView() = default;

		void Init() override {
			// Initialize any resources needed for the plugin view
		}

		void Render() override {
			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Plugin Manager")) {
				RenderToolbar();
				ImGui::Separator();
				RenderPluginList();
				ImGui::Separator();
				RenderPluginDetails();
			}
			ImGui::End();

			// Handle file dialog
			HandleFileDialog();
		}

	private:
		Plugin::PluginManager& pluginManager;
		std::string selectedPlugin;
		char pluginPathBuffer[512] = "";

		void RenderToolbar() {
			if (ImGui::Button("Load Plugin...")) {
				IGFD::FileDialogConfig config;
				config.path = Utils::FilePaths::pluginPath;
				ImGuiFileDialog::Instance()->OpenDialog(
					"LoadPluginDialog",
					"Choose Plugin",
#ifdef _WIN32
					".dll",
#else
					".so",
#endif
					config
				);
			}

			ImGui::SameLine();
			if (ImGui::Button("Scan Plugin Directory")) {
				pluginManager.ScanPluginDirectory(Utils::FilePaths::pluginPath);
			}

			ImGui::SameLine();
			if (ImGui::Button("Reload All Plugins")) {
				pluginManager.ReloadAllPlugins();
			}

			ImGui::SameLine();
			if (!selectedPlugin.empty()) {
				if (ImGui::Button("Unload Selected")) {
					pluginManager.UnloadPlugin(selectedPlugin);
					selectedPlugin.clear();
				}

				ImGui::SameLine();
				if (ImGui::Button("Reload Selected")) {
					pluginManager.ReloadPlugin(selectedPlugin);
				}
			}
		}

		void RenderPluginList() {
			ImGui::Text("Loaded Plugins (%zu):", pluginManager.GetLoadedPluginCount());

			if (ImGui::BeginTable("PluginTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 100.0f);
				ImGui::TableHeadersRow();

				auto pluginNames = pluginManager.GetLoadedPluginNames();
				for (const auto& name : pluginNames) {
					const auto* pluginInfo = pluginManager.GetPluginInfo(name);
					if (!pluginInfo) continue;

					ImGui::TableNextRow();

					// Name column
					ImGui::TableNextColumn();
					bool isSelected = (selectedPlugin == name);
					if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
						selectedPlugin = isSelected ? "" : name;
					}

					// Version column
					ImGui::TableNextColumn();
					ImGui::Text("%s", pluginInfo->version.c_str());

					// Description column
					ImGui::TableNextColumn();
					ImGui::Text("%s", pluginInfo->description.c_str());

					// Actions column
					ImGui::TableNextColumn();
					ImGui::PushID(name.c_str());

					if (ImGui::SmallButton("Reload")) {
						pluginManager.ReloadPlugin(name);
					}

					ImGui::SameLine();
					if (ImGui::SmallButton("Unload")) {
						pluginManager.UnloadPlugin(name);
						if (selectedPlugin == name) {
							selectedPlugin.clear();
						}
					}

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		}

		void RenderPluginDetails() {
			if (selectedPlugin.empty()) {
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a plugin to view details");
				return;
			}

			const auto* pluginInfo = pluginManager.GetPluginInfo(selectedPlugin);
			if (!pluginInfo) {
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Plugin information not available");
				return;
			}

			ImGui::Text("Plugin Details:");
			ImGui::Separator();

			if (ImGui::BeginTable("DetailsTable", 2, ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Name:");
				ImGui::TableNextColumn();
				ImGui::Text("%s", pluginInfo->name.c_str());

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Version:");
				ImGui::TableNextColumn();
				ImGui::Text("%s", pluginInfo->version.c_str());

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Description:");
				ImGui::TableNextColumn();
				ImGui::TextWrapped("%s", pluginInfo->description.c_str());

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("File Path:");
				ImGui::TableNextColumn();
				ImGui::TextWrapped("%s", pluginInfo->filePath.c_str());

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Status:");
				ImGui::TableNextColumn();
				if (pluginInfo->initialized) {
					ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Initialized");
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Not Initialized");
				}

				ImGui::EndTable();
			}

			// Plugin settings UI if available
			if (pluginInfo->plugin && pluginInfo->plugin->HasSettingsUI()) {
				ImGui::Spacing();
				ImGui::Text("Plugin Settings:");
				ImGui::Separator();

				try {
					pluginInfo->plugin->RenderSettingsUI();
				}
				catch (const std::exception& e) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
						"Error rendering plugin settings: %s", e.what());
				}
			}
		}

		void HandleFileDialog() {
			if (ImGuiFileDialog::Instance()->Display("LoadPluginDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					pluginManager.LoadPlugin(filePath);
				}
				ImGuiFileDialog::Instance()->Close();
			}
		}
	};

} // namespace GUI