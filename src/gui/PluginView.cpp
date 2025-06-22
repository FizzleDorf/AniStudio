//============================================================================
// PluginView.cpp - FIXED Plugin Management UI Implementation
//============================================================================

#include "PluginView.hpp"
#include <imgui.h>
#include <iostream>

namespace Plugin {

	PluginView::PluginView(ECS::EntityManager& entityMgr, PluginManager& pluginMgr)
		: BaseView(entityMgr)
		, pluginManager(pluginMgr)
	{
		viewName = "Plugin Manager";
	}

	PluginView::~PluginView() = default;

	void PluginView::Init() {
		std::cout << "PluginView initialized" << std::endl;
	}

	void PluginView::Render() {
		if (!ImGui::Begin(viewName.c_str())) {
			ImGui::End();
			return;
		}

		RenderToolbar();
		ImGui::Separator();

		// Create columns layout
		if (ImGui::BeginTable("PluginManagerLayout", 2, ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("Plugin List", ImGuiTableColumnFlags_WidthFixed, 300.0f);
			ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			RenderPluginList();

			ImGui::TableNextColumn();
			RenderPluginDetails();

			ImGui::EndTable();
		}

		// Error modal
		if (showErrorModal) {
			if (ImGui::BeginPopupModal("Error")) {
				ImGui::Text("%s", errorMessage.c_str());
				if (ImGui::Button("OK")) {
					showErrorModal = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

	void PluginView::Update(float deltaTime) {
		// Update UI state if needed
	}

	void PluginView::RenderToolbar() {
		if (ImGui::Button("Scan Plugins")) {
			pluginManager.ScanForPlugins();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload All")) {
			auto loadedPlugins = pluginManager.GetLoadedPlugins();
			for (const auto& plugin : loadedPlugins) {
				pluginManager.ReloadPlugin(plugin);
			}
		}

		ImGui::SameLine();
		ImGui::Checkbox("Show Only Loaded", &showOnlyLoaded);

		// Search filter
		ImGui::SetNextItemWidth(200.0f);
		char searchBuffer[256];
		strncpy(searchBuffer, searchFilter.c_str(), sizeof(searchBuffer));
		searchBuffer[sizeof(searchBuffer) - 1] = '\0';
		if (ImGui::InputTextWithHint("##search", "Search plugins...", searchBuffer, sizeof(searchBuffer))) {
			searchFilter = searchBuffer;
		}
	}

	void PluginView::RenderPluginList() {
		ImGui::Text("Available Plugins");
		ImGui::Separator();

		auto availablePlugins = pluginManager.GetAvailablePlugins();

		for (const auto& pluginName : availablePlugins) {
			if (!searchFilter.empty() && pluginName.find(searchFilter) == std::string::npos) {
				continue;
			}

			bool isLoaded = pluginManager.IsPluginLoaded(pluginName);

			if (showOnlyLoaded && !isLoaded) {
				continue;
			}

			bool isSelected = (selectedPlugin == pluginName);

			if (ImGui::Selectable(pluginName.c_str(), isSelected)) {
				selectedPlugin = pluginName;
			}

			// Status indicator
			ImGui::SameLine();
			if (isLoaded) {
				RenderStatusBadge("Loaded", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
			else {
				RenderStatusBadge("Unloaded", ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
			}

			// Context menu
			if (ImGui::BeginPopupContextItem()) {
				if (isLoaded) {
					if (ImGui::MenuItem("Unload")) {
						pluginManager.UnloadPlugin(pluginName);
					}
					if (ImGui::MenuItem("Reload")) {
						pluginManager.ReloadPlugin(pluginName);
					}
				}
				else {
					if (ImGui::MenuItem("Load")) {
						if (!pluginManager.LoadPlugin(pluginName)) {
							ShowErrorModal();
						}
					}
				}

				ImGui::EndPopup();
			}
		}
	}

	void PluginView::RenderPluginDetails() {
		if (selectedPlugin.empty()) {
			ImGui::Text("Select a plugin to view details");
			return;
		}

		bool isLoaded = pluginManager.IsPluginLoaded(selectedPlugin);

		ImGui::Text("Plugin: %s", selectedPlugin.c_str());

		if (isLoaded) {
			RenderStatusBadge("Loaded", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
		}
		else {
			RenderStatusBadge("Unloaded", ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
		}

		ImGui::Separator();

		// Action buttons
		if (isLoaded) {
			if (ImGui::Button("Unload")) {
				UnloadSelectedPlugin();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload")) {
				ReloadSelectedPlugin();
			}
		}
		else {
			if (ImGui::Button("Load")) {
				LoadSelectedPlugin();
			}
		}
	}

	void PluginView::RenderStatusBadge(const std::string& status, const ImVec4& color) {
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
		ImGui::SmallButton(status.c_str());
		ImGui::PopStyleColor(3);
	}

	void PluginView::LoadSelectedPlugin() {
		if (!pluginManager.LoadPlugin(selectedPlugin)) {
			ShowErrorModal();
		}
	}

	void PluginView::UnloadSelectedPlugin() {
		if (!pluginManager.UnloadPlugin(selectedPlugin)) {
			ShowErrorModal();
		}
	}

	void PluginView::ReloadSelectedPlugin() {
		if (!pluginManager.ReloadPlugin(selectedPlugin)) {
			ShowErrorModal();
		}
	}

	void PluginView::RemoveSelectedPlugin() {
		selectedPlugin.clear();
	}

	void PluginView::ShowErrorModal() {
		errorTitle = "Plugin Error";
		errorMessage = pluginManager.GetLastError();
		showErrorModal = true;
		ImGui::OpenPopup("Error");
	}

} // namespace Plugin