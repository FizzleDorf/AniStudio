/*
 * PluginView.cpp - Implementation of PluginView with Versioned Hot Reload Status
 */

#include "PluginView.hpp"
#include "BasePlugin.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <imgui.h>

namespace GUI {

	PluginView::PluginView(ECS::EntityManager& entityMgr, Plugins::StudioPluginManager& pluginMgr)
		: BaseView(entityMgr), m_pluginManager(pluginMgr) {
		viewName = "PluginView";
	}

	void PluginView::Init() {
		// Set default plugin directory to ../plugins (where versioned hot reload happens)
		m_pluginDirectory = m_pluginManager.getStagingDirectory();

		if (m_pluginDirectory.empty()) {
			m_pluginDirectory = "../plugins";
		}

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

		// Hot reload processing happens in PluginManager
		// We don't need to call it explicitly here
	}

	void PluginView::Render() {
		if (!windowOpen) return;

		ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			RenderToolbar();
			ImGui::Separator();

			// Split view: left panel for plugin directories, middle for loaded plugins, right for details
			if (ImGui::BeginTable("PluginManagerTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Plugin Directories", ImGuiTableColumnFlags_WidthFixed, 300.0f);
				ImGui::TableSetupColumn("Loaded Plugins", ImGuiTableColumnFlags_WidthFixed, 300.0f);
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
				for (const auto& entry : std::filesystem::directory_iterator(m_pluginDirectory)) {
					if (entry.is_directory()) {
						std::string dirName = entry.path().filename().string();
						if (dirName == "staging") continue;
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

	uint32_t PluginView::GetHighestVersionInDirectory(const std::filesystem::path& pluginDir, const std::string& pluginName) const {
		if (!std::filesystem::exists(pluginDir)) return 0;

		uint32_t highestVersion = 0;
		std::string pattern = pluginName + "_v(\\d+)\\.dll";
		std::regex versionRegex(pattern);

		try {
			for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
				if (entry.is_regular_file()) {
					std::string filename = entry.path().filename().string();
					std::smatch match;

					if (std::regex_search(filename, match, versionRegex)) {
						uint32_t version = static_cast<uint32_t>(std::stoul(match[1].str()));
						if (version > highestVersion) {
							highestVersion = version;
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginView] Error scanning versions: " << e.what() << std::endl;
		}

		return highestVersion;
	}

	size_t PluginView::CountVersionedDlls(const std::filesystem::path& pluginDir, const std::string& pluginName) const {
		if (!std::filesystem::exists(pluginDir)) return 0;

		size_t count = 0;
		std::string pattern = pluginName + "_v\\d+\\.dll";
		std::regex versionRegex(pattern);

		try {
			for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
				if (entry.is_regular_file()) {
					std::string filename = entry.path().filename().string();
					if (std::regex_search(filename, versionRegex)) {
						count++;
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginView] Error counting versions: " << e.what() << std::endl;
		}

		return count;
	}

	void PluginView::LoadPlugin(const std::filesystem::path& pluginDir) {
		std::string pluginName = pluginDir.filename().string();
		std::cout << "[PluginView] Loading plugin: " << pluginName << " from directory: " << pluginDir << std::endl;

		if (m_pluginManager.loadPlugin(pluginDir.string())) {
			ShowStatus("Loaded plugin: " + pluginName, 3.0f);
			std::cout << "[PluginView] Successfully loaded plugin: " << pluginName << std::endl;
		}
		else {
			ShowStatus("Failed to load plugin: " + pluginName, 5.0f);
			std::cerr << "[PluginView] Failed to load plugin: " << pluginName << std::endl;
		}
	}

	void PluginView::LoadPluginFromFile() {
		if (strlen(m_loadDialogPath) > 0) {
			std::string path = m_loadDialogPath;
			std::filesystem::path pluginPath(path);

			if (std::filesystem::is_directory(pluginPath)) {
				if (m_pluginManager.loadPlugin(path)) {
					ShowStatus("Plugin loaded: " + pluginPath.filename().string(), 3.0f);
				}
				else {
					ShowStatus("Failed to load plugin: " + pluginPath.filename().string(), 5.0f);
				}
			}
			else if (std::filesystem::is_regular_file(pluginPath) &&
				(pluginPath.extension() == ".dll" || pluginPath.extension() == ".so")) {
				std::string parentDir = pluginPath.parent_path().string();
				if (m_pluginManager.loadPlugin(parentDir)) {
					ShowStatus("Plugin loaded: " + pluginPath.stem().string(), 3.0f);
				}
				else {
					ShowStatus("Failed to load plugin: " + pluginPath.stem().string(), 5.0f);
				}
			}
			else {
				ShowStatus("Invalid plugin path: must be directory or DLL file", 5.0f);
			}

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
			if (m_selectedPlugin == pluginName) {
				m_selectedPlugin.clear();
			}
		}
		else {
			ShowStatus("Failed to unload plugin: " + pluginName, 5.0f);
		}
	}

	void PluginView::RenderToolbar() {
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

		ImGui::SameLine();
		if (ImGui::Checkbox("Hot Reload", &m_hotReloadEnabled)) {
			m_pluginManager.enableHotReload(m_hotReloadEnabled);
			if (m_hotReloadEnabled) {
				ShowStatus("Versioned hot reload enabled - will detect staging changes", 3.0f);
			}
			else {
				ShowStatus("Versioned hot reload disabled", 2.0f);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Load Plugin...")) {
			m_showLoadDialog = true;
		}

		// Hot reload status indicator
		ImGui::SameLine();
		if (m_hotReloadEnabled) {
			ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[VERSIONED HOT RELOAD: ON]");
		}
		else {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[VERSIONED HOT RELOAD: OFF]");
		}

		// Simple file dialog
		if (m_showLoadDialog) {
			ImGui::OpenPopup("Load Plugin File");
		}

		if (ImGui::BeginPopupModal("Load Plugin File", &m_showLoadDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Enter plugin directory or DLL file path:");
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

				// Check directory contents for versioned hot reload status
				bool hasDll = false;
				bool hasStaging = false;
				bool stagingHasDll = false;
				uint32_t highestVersion = 0;
				size_t versionCount = 0;

				try {
					// Check for staging directory and its contents
					std::filesystem::path stagingPath = pluginDir / "staging";
					if (std::filesystem::exists(stagingPath) && std::filesystem::is_directory(stagingPath)) {
						hasStaging = true;
						for (const auto& entry : std::filesystem::directory_iterator(stagingPath)) {
							if (entry.is_regular_file()) {
								auto ext = entry.path().extension();
								if (ext == ".dll" || ext == ".so") {
									stagingHasDll = true;
									break;
								}
							}
						}
					}

					// Check for versioned DLLs and regular DLLs
					highestVersion = GetHighestVersionInDirectory(pluginDir, pluginName);
					versionCount = CountVersionedDlls(pluginDir, pluginName);

					// Check for non-versioned DLL
					for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
						if (entry.is_regular_file()) {
							auto ext = entry.path().extension();
							std::string filename = entry.path().filename().string();
							if ((ext == ".dll" || ext == ".so") &&
								filename.find("_v") == std::string::npos) {
								hasDll = true;
							}
						}
					}
				}
				catch (...) {
					// Ignore errors during directory scanning
				}

				ImGui::PushID(pluginName.c_str());

				// Color coding based on versioned hot reload status
				ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
				std::string statusSuffix = "";

				if (stagingHasDll) {
					textColor = ImVec4(1.0f, 1.0f, 0.5f, 1.0f); // Yellow - staging ready
					statusSuffix = " [STAGING READY]";
				}
				else if (versionCount > 0) {
					textColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green - versioned DLLs
					statusSuffix = " [v" + std::to_string(highestVersion) + " (" + std::to_string(versionCount) + ")]";
				}
				else if (hasDll) {
					textColor = ImVec4(0.8f, 0.8f, 0.5f, 1.0f); // Orange - non-versioned DLL
					statusSuffix = " [NON-VERSIONED]";
				}
				else {
					textColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // Gray - no DLL
					statusSuffix = " [NO DLL]";
				}

				if (hasDll || versionCount > 0 || hasStaging) {
					ImGui::PushStyleColor(ImGuiCol_Text, textColor);
					if (ImGui::Selectable((pluginName + statusSuffix).c_str(), false)) {
						LoadPlugin(pluginDir);
					}
					ImGui::PopStyleColor();

					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Plugin: %s", pluginName.c_str());
						ImGui::Text("Path: %s", pluginDir.string().c_str());
						if (versionCount > 0) {
							ImGui::Text("Versioned DLLs: %zu (highest: v%u)", versionCount, highestVersion);
						}
						if (hasDll) ImGui::Text("Non-versioned DLL present");
						if (hasStaging) ImGui::Text("Staging directory present");
						if (stagingHasDll) ImGui::Text("Staging DLL ready for versioned hot reload");
						ImGui::Text("Click to load plugin");
						ImGui::EndTooltip();
					}
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Text, textColor);
					ImGui::Text("%s%s", pluginName.c_str(), statusSuffix.c_str());
					ImGui::PopStyleColor();

					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("No DLL files found");
						ImGui::Text("Path: %s", pluginDir.string().c_str());
						ImGui::EndTooltip();
					}
				}

				ImGui::PopID();
			}
		}

		ImGui::EndChild();
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
				std::string displayName = plugin.name;

				// Add versioned hot reload indicator
				if (plugin.hotReloadPending) {
					displayName += " [HOT RELOAD PENDING v" + std::to_string(plugin.nextVersion) + "]";
					stateColor = ImVec4(1.0f, 1.0f, 0.5f, 1.0f); // Yellow
				}
				else if (plugin.currentVersion > 0) {
					displayName += " [v" + std::to_string(plugin.currentVersion) + "]";
				}

				ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
				bool isSelected = (m_selectedPlugin == plugin.name);

				if (ImGui::Selectable(displayName.c_str(), isSelected)) {
					m_selectedPlugin = plugin.name;
				}
				ImGui::PopStyleColor();

				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Name: %s", plugin.name.c_str());
					ImGui::Text("Version: %s", plugin.version.c_str());
					ImGui::Text("State: %s", GetPluginStateText(plugin));
					ImGui::Text("DLL Version: v%u (next: v%u)", plugin.currentVersion, plugin.nextVersion);
					ImGui::Text("Path: %s", plugin.path.c_str());
					if (!plugin.activeDllPath.empty()) {
						ImGui::Text("Active DLL: %s", plugin.activeDllPath.c_str());
					}
					if (!plugin.stagingPath.empty()) {
						ImGui::Text("Staging: %s", plugin.stagingPath.c_str());
					}
					if (plugin.hotReloadPending) {
						ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Versioned hot reload pending!");
					}
					ImGui::EndTooltip();
				}
			}
		}

		ImGui::EndChild();
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

				// Versioned hot reload section
				ImGui::Text("Versioned Hot Reload Information:");
				ImGui::Text("Current DLL Version: v%u", plugin.currentVersion);
				ImGui::Text("Next DLL Version: v%u", plugin.nextVersion);
				ImGui::Text("Active DLL: %s", plugin.activeDllPath.c_str());
				ImGui::Text("Staging Dir: %s", plugin.stagingPath.c_str());

				// Check for versioned DLLs in plugin directory
				size_t versionCount = CountVersionedDlls(plugin.path, plugin.name);
				uint32_t highestVersion = GetHighestVersionInDirectory(plugin.path, plugin.name);

				if (versionCount > 0) {
					ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
						"Versioned DLLs: %zu (highest: v%u)", versionCount, highestVersion);
				}
				else {
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Versioned DLLs: None");
				}

				// Check if staging has DLL
				std::string stagingDllPath;
				std::vector<std::string> possibleNames = {
					plugin.name + ".dll",
					plugin.name + ".so"
				};

				for (const auto& name : possibleNames) {
					std::string testPath = plugin.stagingPath + "/" + name;
					if (std::filesystem::exists(testPath)) {
						stagingDllPath = testPath;
						break;
					}
				}

				if (!stagingDllPath.empty()) {
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Staging DLL: %s", stagingDllPath.c_str());
				}
				else {
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Staging DLL: None");
				}

				if (plugin.hotReloadPending) {
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f),
						"Versioned hot reload pending! Will create v%u", plugin.nextVersion);
					ImGui::Text("Next update cycle will reload this plugin with new version");
				}

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

				ImGui::Separator();

				// Versioned hot reload instructions
				ImGui::Text("Versioned Hot Reload Instructions:");
				ImGui::TextWrapped("1. Build your plugin - DLL goes to staging directory");
				ImGui::TextWrapped("2. PluginManager detects new DLL in staging");
				ImGui::TextWrapped("3. Creates new versioned DLL (e.g., %s_v%u.dll)",
					plugin.name.c_str(), plugin.nextVersion);
				ImGui::TextWrapped("4. Safely unloads old DLL, loads new versioned DLL");
				ImGui::TextWrapped("5. Old versions can be cleaned up (not in use)");
				ImGui::TextWrapped("6. No Windows file locking issues!");

			}
			else {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Selected plugin not found");
			}
		}

		ImGui::EndChild();
	}

	void PluginView::RenderStatusBar() {
		if (!m_statusMessage.empty()) {
			float alpha = std::min(1.0f, m_statusTimer / 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
			ImGui::Text("%s", m_statusMessage.c_str());
			ImGui::PopStyleColor();
		}
		else {
			ImGui::Text("Ready - Versioned hot reload: %s", m_hotReloadEnabled ? "ENABLED" : "DISABLED");
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