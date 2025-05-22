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

// PluginView.hpp
#pragma once

#include "Base/BaseView.hpp"
#include "PluginManager.hpp"

namespace GUI {

	class PluginView : public BaseView {
	public:
		PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr)
			: BaseView(entityMgr), pluginManager(pluginMgr) {
			viewName = "Plugin Manager";
		}

		virtual ~PluginView() = default;

		void Render() override {
			ImGui::Begin(viewName.c_str());

			// Display plugin info
			ImGui::Text("Loaded Plugins:");
			ImGui::Separator();

			auto plugins = pluginManager.GetLoadedPlugins();

			if (plugins.empty()) {
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "No plugins loaded");
			}
			else {
				if (ImGui::BeginTable("PluginsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Version");
					ImGui::TableSetupColumn("Description");
					ImGui::TableSetupColumn("Actions");
					ImGui::TableHeadersRow();

					for (const auto& name : plugins) {
						Plugin::IPlugin* plugin = pluginManager.GetPlugin(name);
						if (!plugin) continue;

						ImGui::TableNextRow();

						// Name
						ImGui::TableNextColumn();
						ImGui::TextWrapped("%s", plugin->GetName().c_str());

						// Version
						ImGui::TableNextColumn();
						ImGui::TextWrapped("%s", plugin->GetVersion().c_str());

						// Description
						ImGui::TableNextColumn();
						ImGui::TextWrapped("%s", plugin->GetDescription().c_str());

						// Actions
						ImGui::TableNextColumn();
						if (ImGui::Button(("Reload##" + name).c_str())) {
							std::string path = pluginManager.GetPluginPath(name);
							pluginManager.UnloadPlugin(name);
							pluginManager.LoadPlugin(path);
						}

						ImGui::SameLine();

						if (ImGui::Button(("Unload##" + name).c_str())) {
							pluginManager.UnloadPlugin(name);
						}
					}

					ImGui::EndTable();
				}
			}

			ImGui::Separator();

			// Button to scan for new plugins
			if (ImGui::Button("Scan for Plugins")) {
				pluginManager.ScanForPlugins();
			}

			ImGui::SameLine();

			// Button to reload all plugins
			if (ImGui::Button("Reload All Plugins")) {
				pluginManager.HotReloadPlugins();
			}

			ImGui::End();
		}

	private:
		Plugin::PluginManager& pluginManager;
	};

} // namespace GUI