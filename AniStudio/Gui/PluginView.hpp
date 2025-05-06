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