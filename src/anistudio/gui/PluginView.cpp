///*
//		d8888          d8b  .d8888b.  888                  888 d8b
//	   d88888          Y8P d88P  Y88b 888                  888 Y8P
//	  d88P888              Y88b.      888                  888
//	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
//	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
//   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
//  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
// d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"
//
// * This file is part of AniStudio.
// * Copyright (C) 2025 FizzleDorf (AnimAnon)
// *
// * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
// * and a commercial license. You may choose to use it under either license.
// *
// * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
// * For commercial license information, please contact legal@kframe.ai.
// */
//
//#include "PluginView.hpp"
//#include "PluginManager.hpp"
//#include <imgui.h>
//#include <algorithm>
//#include <iostream>
//#include <cstring>
//#include "../events/Events.hpp"
//
//namespace GUI {
//
//	// ========== PluginView Implementation ==========
//
//	PluginView::PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr)
//		: BaseView(entityMgr), pluginManager(pluginMgr) {
//		viewName = "PluginView";
//
//		// Initialize directory buffer
//		std::string currentDir = pluginManager.GetWatchDirectory();
//		if (currentDir.empty()) {
//			currentDir = "./plugins";
//		}
//		std::strncpy(directoryInputBuffer, currentDir.c_str(), sizeof(directoryInputBuffer) - 1);
//		directoryInputBuffer[sizeof(directoryInputBuffer) - 1] = '\0';
//	}
//
//	void PluginView::Init() {
//		std::cout << "[PluginView] Initialized for plugins" << std::endl;
//		AddNotification("Plugin View initialized", false);
//		ScanForAvailablePlugins();
//	}
//
//	void PluginView::Update(float deltaTime) {
//		UpdateNotifications(deltaTime);
//
//		// Auto-refresh plugin list
//		if (autoRefresh) {
//			refreshTimer += deltaTime;
//			if (refreshTimer >= refreshInterval) {
//				ScanForAvailablePlugins();
//				refreshTimer = 0.0f;
//			}
//		}
//
//		// Periodically rescan for new plugins
//		lastScanTime += deltaTime;
//		if (lastScanTime >= scanInterval) {
//			ScanForAvailablePlugins();
//			lastScanTime = 0.0f;
//		}
//
//		HandleKeyboardShortcuts();
//	}
//
//	void PluginView::Render() {
//		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
//
//			if (!windowOpen) {
//				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
//			}
//
//			// Menu bar
//			if (ImGui::BeginMenuBar()) {
//				if (ImGui::BeginMenu("Options")) {
//					ImGui::MenuItem("Advanced Controls", nullptr, &showAdvancedControls);
//					ImGui::MenuItem("Statistics", nullptr, &showStats);
//					ImGui::MenuItem("Auto Refresh", nullptr, &autoRefresh);
//					ImGui::EndMenu();
//				}
//
//				if (ImGui::BeginMenu("Actions")) {
//					if (ImGui::MenuItem("Scan for Plugins")) {
//						ScanForAvailablePlugins();
//						AddNotification("Rescanned plugin directories", false);
//					}
//					if (ImGui::MenuItem("Load All Available")) {
//						LoadAllAvailablePlugins();
//					}
//					if (ImGui::MenuItem("Unload All")) {
//						pluginManager.UnloadAllPlugins();
//						AddNotification("All plugins unloaded", false);
//					}
//					ImGui::EndMenu();
//				}
//				ImGui::EndMenuBar();
//			}
//
//			// Plugin type indicator
//			ImGui::Text("Plugin Manager");
//			ImGui::Separator();
//
//			RenderPluginControls();
//			ImGui::Separator();
//			RenderAvailablePlugins();
//			ImGui::Separator();
//			RenderPluginList();
//			ImGui::Separator();
//			RenderHotReloadControls();
//
//			if (showAdvancedControls) {
//				ImGui::Separator();
//				ImGui::Text("Advanced Controls");
//				ImGui::SliderFloat("Refresh Interval", &refreshInterval, 0.5f, 10.0f, "%.1f sec");
//				if (ImGui::Button("Clear Notifications")) {
//					notifications.clear();
//				}
//			}
//
//			if (showStats) {
//				ImGui::Separator();
//				RenderPluginStats();
//			}
//		}
//		ImGui::End();
//
//		RenderNotifications();
//	}
//
//	void PluginView::RenderPluginControls() {
//		ImGui::Text("Manual Plugin Path:");
//		ImGui::InputText("##PluginPath", pluginPathBuffer, sizeof(pluginPathBuffer));
//
//		ImGui::SameLine();
//		if (ImGui::Button("Browse")) {
//			AddNotification("File browser not implemented yet", true);
//		}
//
//		ImGui::SameLine();
//		if (ImGui::Button("Load Path")) {
//			if (strlen(pluginPathBuffer) > 0) {
//				if (pluginManager.LoadPlugin(std::string(pluginPathBuffer))) {
//					AddNotification("Plugin loaded successfully", false);
//					memset(pluginPathBuffer, 0, sizeof(pluginPathBuffer));
//				}
//				else {
//					AddNotification("Failed to load plugin", true);
//				}
//			}
//		}
//	}
//
//	void PluginView::RenderAvailablePlugins() {
//		if (ImGui::CollapsingHeader("Available Plugins", ImGuiTreeNodeFlags_DefaultOpen)) {
//			if (availablePlugins.empty()) {
//				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No plugins found in scan directories");
//				ImGui::Text("Scanned: ../plugins/, ../../plugins/, plugins/");
//			}
//			else {
//				ImGui::Text("Found %zu plugin(s):", availablePlugins.size());
//
//				for (const auto& pluginPath : availablePlugins) {
//					std::string pluginName = GetPluginNameFromPath(pluginPath);
//					bool isLoaded = pluginManager.IsPluginLoaded(pluginName);
//
//					ImGui::PushID(pluginPath.c_str());
//
//					if (isLoaded) {
//						ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[LOADED] %s", pluginName.c_str());
//					}
//					else {
//						ImGui::Text("%s", pluginName.c_str());
//						ImGui::SameLine();
//
//						if (ImGui::Button("Load")) {
//							if (pluginManager.LoadPlugin(pluginPath)) {
//								AddNotification("Loaded: " + pluginName, false);
//							}
//							else {
//								AddNotification("Failed to load: " + pluginName, true);
//							}
//						}
//					}
//
//					// Show path on hover
//					if (ImGui::IsItemHovered()) {
//						ImGui::BeginTooltip();
//						ImGui::Text("Path: %s", pluginPath.c_str());
//						ImGui::EndTooltip();
//					}
//
//					ImGui::PopID();
//				}
//			}
//		}
//	}
//
//	void PluginView::RenderPluginList() {
//		if (ImGui::CollapsingHeader("Loaded Plugins", pluginListOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
//			pluginListOpen = true;
//
//			auto loadedPlugins = pluginManager.GetLoadedPluginNames();
//
//			if (loadedPlugins.empty()) {
//				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No plugins loaded");
//			}
//			else {
//				for (const auto& pluginName : loadedPlugins) {
//					bool isSelected = (selectedPluginName == pluginName);
//
//					if (ImGui::Selectable(pluginName.c_str(), isSelected)) {
//						selectedPluginName = pluginName;
//					}
//
//					// Context menu
//					if (ImGui::BeginPopupContextItem()) {
//						if (ImGui::MenuItem("Unload")) {
//							if (pluginManager.UnloadPlugin(pluginName)) {
//								AddNotification("Plugin unloaded: " + pluginName, false);
//								if (selectedPluginName == pluginName) {
//									selectedPluginName.clear();
//								}
//							}
//							else {
//								AddNotification("Failed to unload: " + pluginName, true);
//							}
//						}
//						if (ImGui::MenuItem("Reload")) {
//							if (pluginManager.ReloadPlugin(pluginName)) {
//								AddNotification("Plugin reloaded: " + pluginName, false);
//							}
//							else {
//								AddNotification("Failed to reload: " + pluginName, true);
//							}
//						}
//						ImGui::EndPopup();
//					}
//				}
//
//				// Quick actions for selected plugin
//				if (!selectedPluginName.empty()) {
//					ImGui::Separator();
//					if (ImGui::Button("Reload Selected")) {
//						if (pluginManager.ReloadPlugin(selectedPluginName)) {
//							AddNotification("Plugin reloaded: " + selectedPluginName, false);
//						}
//						else {
//							AddNotification("Failed to reload: " + selectedPluginName, true);
//						}
//					}
//					ImGui::SameLine();
//					if (ImGui::Button("Unload Selected")) {
//						if (pluginManager.UnloadPlugin(selectedPluginName)) {
//							AddNotification("Plugin unloaded: " + selectedPluginName, false);
//							selectedPluginName.clear();
//						}
//						else {
//							AddNotification("Failed to unload: " + selectedPluginName, true);
//						}
//					}
//				}
//			}
//		}
//	}
//
//	void PluginView::RenderHotReloadControls() {
//		ImGui::Text("Hot Reload");
//
//		bool isActive = pluginManager.IsHotReloadActive();
//		ImGui::Text("Status: %s", isActive ? "ACTIVE" : "INACTIVE");
//
//		if (isActive) {
//			ImGui::SameLine();
//			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "(%s)", pluginManager.GetWatchDirectory().c_str());
//
//			if (ImGui::Button("Stop Hot Reload")) {
//				pluginManager.StopHotReload();
//				AddNotification("Hot reload stopped", false);
//			}
//		}
//		else {
//			RenderDirectoryControls();
//
//			if (ImGui::Button("Start Hot Reload")) {
//				std::string watchDir = std::string(directoryInputBuffer);
//				pluginManager.StartHotReload(watchDir);
//				AddNotification("Hot reload started for: " + watchDir, false);
//			}
//		}
//	}
//
//	void PluginView::RenderDirectoryControls() {
//		if (!showDirectoryInput) {
//			if (ImGui::Button("Change Directory")) {
//				showDirectoryInput = true;
//			}
//		}
//		else {
//			ImGui::Text("Watch Directory:");
//			if (ImGui::InputText("##DirectoryInput", directoryInputBuffer, sizeof(directoryInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
//				showDirectoryInput = false;
//			}
//
//			ImGui::SameLine();
//			if (ImGui::Button("Cancel")) {
//				// Reset buffer from current directory
//				std::string currentDir = pluginManager.GetWatchDirectory();
//				if (currentDir.empty()) currentDir = "./plugins";
//				std::strncpy(directoryInputBuffer, currentDir.c_str(), sizeof(directoryInputBuffer) - 1);
//				directoryInputBuffer[sizeof(directoryInputBuffer) - 1] = '\0';
//				showDirectoryInput = false;
//			}
//		}
//	}
//
//	void PluginView::RenderPluginStats() {
//		ImGui::Text("Plugin Statistics");
//		auto loadedPlugins = pluginManager.GetLoadedPluginNames();
//		ImGui::Text("Loaded Plugins: %zu", loadedPlugins.size());
//		ImGui::Text("Available Plugins: %zu", availablePlugins.size());
//		ImGui::Text("Refresh Timer: %.1f / %.1f", refreshTimer, refreshInterval);
//		ImGui::Text("Hot Reload: %s", pluginManager.IsHotReloadActive() ? "Active" : "Inactive");
//	}
//
//	void PluginView::RenderNotifications() {
//		ImGuiIO& io = ImGui::GetIO();
//		float startY = 30.0f;
//		float notifHeight = 50.0f;
//
//		for (size_t i = 0; i < notifications.size(); ++i) {
//			const auto& notif = notifications[i];
//
//			ImVec2 pos(io.DisplaySize.x - 300.0f, startY + i * notifHeight);
//			ImGui::SetNextWindowPos(pos);
//			ImGui::SetNextWindowSize(ImVec2(290.0f, notifHeight - 5.0f));
//
//			std::string windowName = "##Notification" + std::to_string(i);
//			ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
//				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
//
//			if (ImGui::Begin(windowName.c_str(), nullptr, flags)) {
//				ImVec4 color = notif.isError ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
//				ImGui::PushStyleColor(ImGuiCol_Text, color);
//				ImGui::TextWrapped("%s", notif.message.c_str());
//				ImGui::PopStyleColor();
//
//				float progress = notif.timeLeft / notificationDuration;
//				ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "");
//			}
//			ImGui::End();
//		}
//	}
//
//	void PluginView::UpdateNotifications(float deltaT) {
//		for (auto it = notifications.begin(); it != notifications.end();) {
//			it->timeLeft -= deltaT;
//			if (it->timeLeft <= 0.0f) {
//				it = notifications.erase(it);
//			}
//			else {
//				++it;
//			}
//		}
//	}
//
//	void PluginView::AddNotification(const std::string& message, bool isError) {
//		PluginNotification notif;
//		notif.message = message;
//		notif.isError = isError;
//		notif.timeLeft = notificationDuration;
//		notif.color = isError ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
//
//		notifications.push_back(notif);
//
//		// Limit notifications
//		if (notifications.size() > 5) {
//			notifications.erase(notifications.begin());
//		}
//	}
//
//	void PluginView::HandleKeyboardShortcuts() {
//		ImGuiIO& io = ImGui::GetIO();
//
//		if (io.KeyCtrl) {
//			if (ImGui::IsKeyPressed(ImGuiKey_R)) {
//				if (!selectedPluginName.empty()) {
//					pluginManager.ReloadPlugin(selectedPluginName);
//					AddNotification("Reloaded: " + selectedPluginName, false);
//				}
//			}
//			if (ImGui::IsKeyPressed(ImGuiKey_U)) {
//				if (!selectedPluginName.empty()) {
//					pluginManager.UnloadPlugin(selectedPluginName);
//					AddNotification("Unloaded: " + selectedPluginName, false);
//					selectedPluginName.clear();
//				}
//			}
//		}
//	}
//
//	void PluginView::ScanForAvailablePlugins() {
//		availablePlugins.clear();
//
//		// Plugin directory search paths (from /anistudio/build/bin)
//		std::vector<std::string> searchPaths = {
//			"../plugins/",      // /anistudio/build/plugins/
//			"../../plugins/",   // /anistudio/plugins/
//			"plugins/",         // /anistudio/build/bin/plugins/
//			"./plugins/"        // Current + plugins
//		};
//
//		for (const auto& searchPath : searchPaths) {
//			try {
//				if (std::filesystem::exists(searchPath) && std::filesystem::is_directory(searchPath)) {
//					// Look for plugin subdirectories
//					for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
//						if (entry.is_directory()) {
//							std::string pluginDir = entry.path().filename().string();
//
//#ifdef _WIN32
//							std::string dllPath = searchPath + pluginDir + "/" + pluginDir + ".dll";
//#else
//							std::string dllPath = searchPath + pluginDir + "/lib" + pluginDir + ".so";
//#endif
//							// Check if the library exists
//							if (std::filesystem::exists(dllPath)) {
//								availablePlugins.push_back(dllPath);
//							}
//						}
//					}
//				}
//			}
//			catch (const std::filesystem::filesystem_error& e) {
//				// Directory doesn't exist or can't be accessed, skip silently
//			}
//		}
//
//		// Remove duplicates
//		std::sort(availablePlugins.begin(), availablePlugins.end());
//		availablePlugins.erase(std::unique(availablePlugins.begin(), availablePlugins.end()), availablePlugins.end());
//	}
//
//	void PluginView::LoadAllAvailablePlugins() {
//		int loadedCount = 0;
//		int failedCount = 0;
//
//		for (const auto& pluginPath : availablePlugins) {
//			std::string pluginName = GetPluginNameFromPath(pluginPath);
//
//			if (!pluginManager.IsPluginLoaded(pluginName)) {
//				if (pluginManager.LoadPlugin(pluginPath)) {
//					loadedCount++;
//				}
//				else {
//					failedCount++;
//				}
//			}
//		}
//
//		if (loadedCount > 0) {
//			AddNotification("Loaded " + std::to_string(loadedCount) + " plugin(s)", false);
//		}
//		if (failedCount > 0) {
//			AddNotification("Failed to load " + std::to_string(failedCount) + " plugin(s)", true);
//		}
//		if (loadedCount == 0 && failedCount == 0) {
//			AddNotification("All available plugins already loaded", false);
//		}
//	}
//
//	ImVec4 PluginView::GetStatusColor(bool isLoaded, bool hasError) const {
//		if (hasError) return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
//		if (isLoaded) return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
//		return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Gray
//	}
//
//	const char* PluginView::GetStatusText(bool isLoaded, bool hasError) const {
//		if (hasError) return "ERROR";
//		if (isLoaded) return "LOADED";
//		return "NOT LOADED";
//	}
//
//	std::string PluginView::GetPluginNameFromPath(const std::string& path) const {
//		std::filesystem::path p(path);
//		return p.stem().string();
//	}
//
//} // namespace GUI