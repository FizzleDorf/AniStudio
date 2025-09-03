/*
 * PluginView.cpp - Implementation of PluginView
 */

#include "PluginView.hpp"
#include "BasePlugin.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <imgui.h>

namespace GUI {

	PluginView::PluginView(ECS::EntityManager& entityMgr, Plugins::StudioPluginManager& pluginMgr)
		: BaseView(entityMgr), m_pluginManager(pluginMgr) {
		viewName = "PluginView";
	}

	void PluginView::Init() {
		// Set default plugin directory to ../plugins relative to executable
		std::filesystem::path exePath = std::filesystem::current_path();
		std::filesystem::path pluginsPath = exePath.parent_path() / "plugins";
		m_pluginDirectory = pluginsPath.string();

		// Scan for plugin directories
		RefreshPluginDirectories();

		// Enable hot reload by default
		m_pluginManager.enableHotReload(m_hotReloadEnabled);

		std::cout << "[PluginView] Initialized with plugin directory: " << m_pluginDirectory << std::endl;
	}

	void PluginView::Update(float deltaT) {
		// Update status timer
		if (m_statusTimer > 0.0f) {
			m_statusTimer -= deltaT;
			if (m_statusTimer <= 0.0f) {
				m_statusMessage.clear();
			}
		}

		// Check for hot reload changes if enabled
		if (m_hotReloadEnabled) {
			m_pluginManager.checkForChanges();
		}
	}

	void PluginView::Render() {
		if (!windowOpen) return;

		ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			RenderToolbar();
			ImGui::Separator();

			// Split view: left panel for plugin directories, middle for loaded plugins, right for details
			if (ImGui::BeginTable("PluginManagerTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Plugin Directories", ImGuiTableColumnFlags_WidthFixed, 250.0f);
				ImGui::TableSetupColumn("Loaded Plugins", ImGuiTableColumnFlags_WidthFixed, 250.0f);
				ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();

				// Plugin directories column
				ImGui::TableSetColumnIndex(0);
				RenderPluginDirectoryList();

				// Loaded plugins column
				ImGui::TableSetColumnIndex(1);
				RenderLoadedPluginsList();

				// Details column
				ImGui::TableSetColumnIndex(2);
				RenderPluginDetails();

				ImGui::EndTable();
			}

			ImGui::Separator();
			RenderStatusBar();
		}
		ImGui::End();
	}

	void PluginView::RefreshPluginDirectories() {
		m_pluginDirectories.clear();

		try {
			if (std::filesystem::exists(m_pluginDirectory)) {
				// Scan for plugin directories
				for (const auto& entry : std::filesystem::directory_iterator(m_pluginDirectory)) {
					if (entry.is_directory()) {
						m_pluginDirectories.push_back(entry.path());
					}
				}

				std::sort(m_pluginDirectories.begin(), m_pluginDirectories.end());

				ShowStatus("Found " + std::to_string(m_pluginDirectories.size()) + " plugin directories", 2.0f);
				std::cout << "[PluginView] Found " << m_pluginDirectories.size() << " plugin directories" << std::endl;
			}
			else {
				ShowStatus("Plugin directory not found: " + m_pluginDirectory, 5.0f);
				std::cerr << "[PluginView] Plugin directory not found: " << m_pluginDirectory << std::endl;
			}
		}
		catch (const std::exception& e) {
			ShowStatus("Error scanning plugin directories: " + std::string(e.what()), 5.0f);
			std::cerr << "[PluginView] Error refreshing plugin directories: " << e.what() << std::endl;
		}
	}

	void PluginView::LoadPlugin(const std::filesystem::path& pluginDir) {
		std::string pluginName = pluginDir.filename().string();

		// Try multiple possible DLL names and extensions
		std::vector<std::filesystem::path> possiblePaths = {
			pluginDir / (pluginName + ".dll"),
			pluginDir / (pluginName + ".so"),
			pluginDir / ("lib" + pluginName + ".so"),
			pluginDir / "plugin.dll",
			pluginDir / "plugin.so"
		};

		bool found = false;
		for (const auto& dllPath : possiblePaths) {
			if (std::filesystem::exists(dllPath)) {
				if (m_pluginManager.loadPlugin(dllPath.string())) {
					ShowStatus("Loaded plugin: " + pluginName, 3.0f);
					std::cout << "[PluginView] Loaded plugin: " << pluginName << " from " << dllPath << std::endl;
					found = true;
					break;
				}
			}
		}

		if (!found) {
			// If we get here, no DLL was found
			ShowStatus("No valid plugin DLL found in: " + pluginDir.string(), 5.0f);
			std::cerr << "[PluginView] No valid plugin DLL found in: " << pluginDir << std::endl;

			// Debug: list directory contents
			try {
				std::cout << "[PluginView] Directory contents:" << std::endl;
				for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
					std::cout << "  " << entry.path().filename() << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[PluginView] Error listing directory: " << e.what() << std::endl;
			}
		}
	}

	void PluginView::LoadPluginFromFile() {
		if (strlen(m_loadDialogPath) > 0) {
			std::string path = m_loadDialogPath;

			if (m_pluginManager.loadPlugin(path)) {
				ShowStatus("Plugin loaded: " + std::filesystem::path(path).filename().string(), 3.0f);
			}
			else {
				ShowStatus("Failed to load plugin: " + std::filesystem::path(path).filename().string(), 5.0f);
			}

			// Clear the path
			m_loadDialogPath[0] = '\0';
			m_showLoadDialog = false;
		}
	}

	void PluginView::EnablePlugin(const std::string& pluginName) {
		if (m_pluginManager.enablePlugin(pluginName)) {
			ShowStatus("Enabled plugin: " + pluginName, 3.0f);
		}
		else {
			ShowStatus("Failed to enable plugin: " + pluginName, 5.0f);
		}
	}

	void PluginView::DisablePlugin(const std::string& pluginName) {
		if (m_pluginManager.disablePlugin(pluginName)) {
			ShowStatus("Disabled plugin: " + pluginName, 3.0f);
		}
		else {
			ShowStatus("Failed to disable plugin: " + pluginName, 5.0f);
		}
	}

	void PluginView::ReloadPlugin(const std::string& pluginName) {
		if (m_pluginManager.reloadPlugin(pluginName)) {
			ShowStatus("Reloaded plugin: " + pluginName, 3.0f);
		}
		else {
			ShowStatus("Failed to reload plugin: " + pluginName, 5.0f);
		}
	}

	void PluginView::UnloadPlugin(const std::string& pluginName) {
		if (m_pluginManager.unloadPlugin(pluginName)) {
			ShowStatus("Unloaded plugin: " + pluginName, 3.0f);
			// Clear selection if it was the unloaded plugin
			if (m_selectedPlugin == pluginName) {
				m_selectedPlugin.clear();
			}
		}
		else {
			ShowStatus("Failed to unload plugin: " + pluginName, 5.0f);
		}
	}

	void PluginView::RenderToolbar() {
		// Plugin directory
		ImGui::Text("Plugin Directory:");
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		if (ImGui::InputText("##PluginDir", m_pluginDirectory.data(), m_pluginDirectory.capacity() + 1)) {
			m_pluginDirectory = std::string(m_pluginDirectory.data());
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("Refresh Directories")) {
			RefreshPluginDirectories();
		}

		// Hot reload toggle
		ImGui::SameLine();
		if (ImGui::Checkbox("Hot Reload", &m_hotReloadEnabled)) {
			m_pluginManager.enableHotReload(m_hotReloadEnabled);
			if (m_hotReloadEnabled) {
				ShowStatus("Hot reload enabled", 2.0f);
			}
			else {
				ShowStatus("Hot reload disabled", 2.0f);
			}
		}

		// Load plugin from file
		ImGui::SameLine();
		if (ImGui::Button("Load Plugin...")) {
			m_showLoadDialog = true;
		}

		// Simple file dialog (basic implementation)
		if (m_showLoadDialog) {
			ImGui::OpenPopup("Load Plugin File");
		}

		if (ImGui::BeginPopupModal("Load Plugin File", &m_showLoadDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Enter plugin file path:");
			ImGui::InputText("##PluginPath", m_loadDialogPath, sizeof(m_loadDialogPath));

			if (ImGui::Button("Load")) {
				LoadPluginFromFile();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				m_showLoadDialog = false;
				m_loadDialogPath[0] = '\0';
			}

			ImGui::EndPopup();
		}
	}

	void PluginView::RenderPluginDirectoryList() {
		ImGui::BeginChild("PluginDirectoryList", ImVec2(0, -30), true);
		ImGui::Text("Plugin Directories");
		ImGui::Separator();

		if (m_pluginDirectories.empty()) {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No plugin directories found");
		}
		else {
			for (const auto& pluginDir : m_pluginDirectories) {
				std::string pluginName = pluginDir.filename().string();
				std::filesystem::path dllPath = pluginDir / (pluginName + ".dll");
				bool dllExists = std::filesystem::exists(dllPath);

				ImGui::PushID(pluginName.c_str());

				if (dllExists) {
					// Directory with DLL exists - show as clickable
					if (ImGui::Selectable(pluginName.c_str(), false)) {
						LoadPlugin(pluginDir);
					}

					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Click to load: %s.dll", pluginName.c_str());
						ImGui::Text("Path: %s", pluginDir.string().c_str());
						ImGui::EndTooltip();
					}
				}
				else {
					// Directory exists but no DLL - show as disabled
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("%s (No DLL)", pluginName.c_str());
					ImGui::PopStyleColor();

					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Missing: %s.dll", pluginName.c_str());
						ImGui::Text("Expected at: %s", dllPath.string().c_str());
						ImGui::EndTooltip();
					}
				}

				ImGui::PopID();
			}
		}

		ImGui::EndChild();

		// Directory count
		ImGui::Text("Directories: %zu", m_pluginDirectories.size());
	}

	void PluginView::RenderLoadedPluginsList() {
		ImGui::BeginChild("LoadedPluginsList", ImVec2(0, -30), true);
		ImGui::Text("Loaded Plugins");
		ImGui::Separator();

		auto loadedPlugins = m_pluginManager.getLoadedPlugins();

		if (loadedPlugins.empty()) {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No plugins loaded");
		}
		else {
			for (const auto& plugin : loadedPlugins) {
				ImVec4 stateColor = GetPluginStateColor(plugin);

				// Plugin name with state indicator
				ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
				bool isSelected = (m_selectedPlugin == plugin.name);

				if (ImGui::Selectable(plugin.name.c_str(), isSelected)) {
					m_selectedPlugin = plugin.name;
				}
				ImGui::PopStyleColor();

				// Show tooltip with basic info
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Name: %s", plugin.name.c_str());
					ImGui::Text("Version: %s", plugin.version.c_str());
					ImGui::Text("State: %s", GetPluginStateText(plugin));
					ImGui::Text("Path: %s", plugin.path.c_str());
					ImGui::EndTooltip();
				}
			}
		}

		ImGui::EndChild();

		// Plugin count
		ImGui::Text("Plugins: %zu", loadedPlugins.size());
	}

	void PluginView::RenderPluginDetails() {
		ImGui::BeginChild("PluginDetails");

		if (m_selectedPlugin.empty()) {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a plugin to view details");
		}
		else {
			auto loadedPlugins = m_pluginManager.getLoadedPlugins();
			auto pluginIt = std::find_if(loadedPlugins.begin(), loadedPlugins.end(),
				[this](const Plugins::PluginInfo& p) { return p.name == m_selectedPlugin; });

			if (pluginIt != loadedPlugins.end()) {
				const auto& plugin = *pluginIt;

				ImGui::Text("Plugin Details");
				ImGui::Separator();

				ImGui::Text("Name: %s", plugin.name.c_str());
				ImGui::Text("Version: %s", plugin.version.c_str());
				ImGui::Text("Path: %s", plugin.path.c_str());

				ImVec4 stateColor = GetPluginStateColor(plugin);
				ImGui::TextColored(stateColor, "State: %s", GetPluginStateText(plugin));

				ImGui::Separator();

				// Control buttons
				if (plugin.loaded) {
					if (plugin.enabled) {
						if (ImGui::Button("Disable")) {
							DisablePlugin(plugin.name);
						}
						ImGui::SameLine();
						if (ImGui::Button("Reload")) {
							ReloadPlugin(plugin.name);
						}
					}
					else {
						if (ImGui::Button("Enable")) {
							EnablePlugin(plugin.name);
						}
					}

					ImGui::SameLine();
					if (ImGui::Button("Unload")) {
						UnloadPlugin(plugin.name);
					}
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Plugin not loaded");
				}

				ImGui::Separator();

				// Additional info
				if (plugin.instance) {
					ImGui::Text("Plugin Instance: Available");
					ImGui::Text("Initialized: %s", plugin.instance->IsInitialized() ? "Yes" : "No");
				}
				else {
					ImGui::Text("Plugin Instance: None");
				}

			}
			else {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Selected plugin not found");
			}
		}

		ImGui::EndChild();
	}

	void PluginView::RenderStatusBar() {
		if (!m_statusMessage.empty()) {
			float alpha = std::min(1.0f, m_statusTimer / 1.0f); // Fade out in last second
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
			ImGui::Text("%s", m_statusMessage.c_str());
			ImGui::PopStyleColor();
		}
		else {
			ImGui::Text("Ready");
		}
	}

	void PluginView::ShowStatus(const std::string& message, float duration) {
		m_statusMessage = message;
		m_statusTimer = duration;
		std::cout << "[PluginView] Status: " << message << std::endl;
	}

	const char* PluginView::GetPluginStateText(const Plugins::PluginInfo& plugin) const {
		if (!plugin.loaded) {
			return "Not Loaded";
		}
		else if (plugin.enabled) {
			return "Enabled";
		}
		else {
			return "Loaded";
		}
	}

	ImVec4 PluginView::GetPluginStateColor(const Plugins::PluginInfo& plugin) const {
		if (!plugin.loaded) {
			return ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // Gray
		}
		else if (plugin.enabled) {
			return ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green
		}
		else {
			return ImVec4(1.0f, 1.0f, 0.5f, 1.0f); // Yellow
		}
	}

} // namespace GUI