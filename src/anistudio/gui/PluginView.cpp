// PluginView.cpp - Implementation of plugin management GUI
#include "PluginView.hpp"
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace GUI {

	PluginView::PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr)
		: BaseView(entityMgr), pluginManager(pluginMgr) {
		// Initialize directory buffer with current plugin directory (if available)
		std::string currentDir = pluginManager.GetWatchDirectory();
		if (currentDir.empty()) {
			currentDir = "./plugins";
		}
		strcpy_s(directoryInputBuffer, sizeof(directoryInputBuffer), currentDir.c_str());
		directoryInputBuffer[sizeof(directoryInputBuffer) - 1] = '\0';
	}

	void PluginView::Init() {
		std::cout << "PluginView initialized" << std::endl;

		// Set up event callbacks
		pluginManager.SetLoadCallback([this](const std::string& name, bool isReload) {
			OnPluginLoaded(name, isReload);
		});

		pluginManager.SetUnloadCallback([this](const std::string& name) {
			OnPluginUnloaded(name);
		});

		pluginManager.SetErrorCallback([this](const std::string& name, const std::string& error) {
			OnPluginError(name, error);
		});

		AddNotification("Plugin View initialized", false);
	}

	void PluginView::Update(const float deltaT) {
		UpdateNotifications(deltaT);

		// Auto-refresh plugin list
		if (autoRefresh) {
			refreshTimer += deltaT;
			if (refreshTimer >= refreshInterval) {
				pluginManager.RefreshPluginDirectory();
				refreshTimer = 0.0f;
			}
		}

		HandleKeyboardShortcuts();
	}

	void PluginView::Render() {
		if (ImGui::Begin("Plugin Manager", nullptr, ImGuiWindowFlags_MenuBar)) {
			// Menu bar
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Refresh Plugin List", "Ctrl+R")) {
						pluginManager.RefreshPluginDirectory();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Open Plugin Directory")) {
						AddNotification("Directory opening not implemented", false);
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View")) {
					ImGui::MenuItem("Show Advanced Controls", nullptr, &showAdvancedControls);
					ImGui::MenuItem("Show Statistics", nullptr, &showStats);
					ImGui::MenuItem("Auto Refresh", nullptr, &autoRefresh);
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Plugin")) {
					auto loadedPlugins = pluginManager.GetLoadedPluginNames();
					bool hasPlugins = !loadedPlugins.empty();

					if (ImGui::MenuItem("Reload Selected", "F5", false, hasPlugins && !selectedPluginName.empty())) {
						if (!selectedPluginName.empty()) {
							pluginManager.ReloadPlugin(selectedPluginName);
						}
					}

					if (ImGui::MenuItem("Unload Selected", nullptr, false, hasPlugins && !selectedPluginName.empty())) {
						if (!selectedPluginName.empty()) {
							pluginManager.UnloadPlugin(selectedPluginName);
						}
					}

					if (ImGui::MenuItem("Unload All", nullptr, false, hasPlugins)) {
						pluginManager.UnloadAllPlugins();
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			// Main content
			RenderPluginControls();

			ImGui::Separator();

			RenderPluginList();

			ImGui::Separator();

			RenderHotReloadControls();

			ImGui::Separator();

			RenderPluginInfo();

			if (showAdvancedControls) {
				ImGui::Separator();
				ImGui::Text("Advanced Controls");

				ImGui::SliderFloat("Refresh Interval", &refreshInterval, 0.5f, 10.0f, "%.1f sec");

				if (ImGui::Button("Force Refresh")) {
					pluginManager.RefreshPluginDirectory();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear Notifications")) {
					notifications.clear();
				}
			}

			if (showStats) {
				ImGui::Separator();
				RenderPluginStats();
			}
		}
		ImGui::End();

		// Render notifications overlay
		RenderNotifications();
	}

	void PluginView::RenderPluginControls() {
		RenderDirectoryControls();

		ImGui::Separator();

		// Quick actions
		if (ImGui::Button("Refresh List")) {
			pluginManager.RefreshPluginDirectory();
			refreshTimer = 0.0f;
		}

		ImGui::SameLine();
		if (ImGui::Button("Unload All")) {
			pluginManager.UnloadAllPlugins();
		}

		// Directory input
		if (showDirectoryInput) {
			ImGui::Text("Plugin Directory:");
			if (ImGui::InputText("##DirectoryInput", directoryInputBuffer, sizeof(directoryInputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				pluginManager.SetWatchDirectory(std::string(directoryInputBuffer));
				showDirectoryInput = false;
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				// Reset buffer from current directory
				std::string currentDir = pluginManager.GetWatchDirectory();
				strcpy_s(directoryInputBuffer, sizeof(directoryInputBuffer), currentDir.c_str());
				directoryInputBuffer[sizeof(directoryInputBuffer) - 1] = '\0';
				showDirectoryInput = false;
			}
		}
	}

	void PluginView::RenderPluginList() {
		if (ImGui::CollapsingHeader("Loaded Plugins", pluginListOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			pluginListOpen = true;

			auto loadedPlugins = pluginManager.GetLoadedPluginNames();

			if (loadedPlugins.empty()) {
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No plugins loaded");
			}
			else {
				for (const auto& pluginName : loadedPlugins) {
					bool isSelected = (selectedPluginName == pluginName);
					if (ImGui::Selectable(pluginName.c_str(), isSelected)) {
						selectedPluginName = pluginName;
					}

					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Plugin: %s", pluginName.c_str());

						auto* info = pluginManager.GetPluginInfo(pluginName);
						if (info) {
							ImGui::Text("Path: %s", info->path.c_str());
							ImGui::Text("Status: %s", info->hasError ? "ERROR" : "OK");
							if (info->hasError) {
								ImGui::Text("Error: %s", info->errorMessage.c_str());
							}
						}
						ImGui::EndTooltip();
					}

					// Context menu
					if (ImGui::BeginPopupContextItem()) {
						if (ImGui::MenuItem("Reload")) {
							pluginManager.ReloadPlugin(pluginName);
						}
						if (ImGui::MenuItem("Unload")) {
							pluginManager.UnloadPlugin(pluginName);
						}
						ImGui::EndPopup();
					}
				}
			}

			ImGui::Separator();

			// File browser for loading plugins
			static char pluginPath[512] = "";
			ImGui::Text("Plugin Path:");
			ImGui::InputText("##PluginPath", pluginPath, sizeof(pluginPath));

			ImGui::SameLine();
			if (ImGui::Button("Browse")) {
				// Would need file dialog implementation
				AddNotification("File browser not implemented", false);
			}

			ImGui::SameLine();
			if (ImGui::Button("Load Plugin")) {
				if (strlen(pluginPath) > 0) {
					if (pluginManager.LoadPlugin(std::string(pluginPath))) {
						AddNotification("Plugin load initiated", false);
					}
					else {
						AddNotification("Failed to initiate plugin load", true);
					}
				}
			}
		}
		else {
			pluginListOpen = false;
		}
	}

	void PluginView::RenderHotReloadControls() {
		if (ImGui::CollapsingHeader("Hot Reload Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool hotReloadActive = pluginManager.IsHotReloadActive();

			ImGui::Text("Hot Reload Status: %s", hotReloadActive ? "ACTIVE" : "INACTIVE");

			if (hotReloadActive) {
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Watching: %s", pluginManager.GetWatchDirectory().c_str());

				if (ImGui::Button("Stop Hot Reload")) {
					pluginManager.StopHotReload();
				}
			}
			else {
				if (ImGui::Button("Start Hot Reload")) {
					pluginManager.StartHotReload(pluginManager.GetWatchDirectory());
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Manual Reload (F5)")) {
				if (!selectedPluginName.empty()) {
					pluginManager.ReloadPlugin(selectedPluginName);
				}
				else {
					AddNotification("No plugin selected for reload", true);
				}
			}

			if (hotReloadActive) {
				ImGui::Text("Plugin will automatically reload when file changes are detected");
			}
		}
	}

	void PluginView::RenderPluginInfo() {
		if (ImGui::CollapsingHeader("Plugin Information", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (!selectedPluginName.empty()) {
				auto* info = pluginManager.GetPluginInfo(selectedPluginName);
				if (info) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Selected: %s", selectedPluginName.c_str());
					ImGui::Text("Path: %s", info->path.c_str());
					ImGui::Text("Status: %s", info->isLoaded ? "LOADED" : "UNLOADED");

					if (info->hasError) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", info->errorMessage.c_str());
					}
					else {
						ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: OK");
					}
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Plugin info not found!");
				}
			}
			else {
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No plugin selected");
				ImGui::Text("Click on a plugin in the list to view its information");
			}
		}
	}

	void PluginView::RenderPluginStats() {
		if (ImGui::CollapsingHeader("Statistics")) {
			ImGui::Text("Plugin Statistics:");
			ImGui::Indent();

			auto loadedPlugins = pluginManager.GetLoadedPluginNames();
			ImGui::Text("Loaded Plugins: %zu", loadedPlugins.size());
			ImGui::Text("Watch Directory: %s", pluginManager.GetWatchDirectory().c_str());
			ImGui::Text("Hot Reload: %s", pluginManager.IsHotReloadActive() ? "Active" : "Inactive");

			if (!loadedPlugins.empty()) {
				ImGui::Separator();
				ImGui::Text("Plugin List:");
				for (const auto& name : loadedPlugins) {
					auto* info = pluginManager.GetPluginInfo(name);
					if (info) {
						ImVec4 color = info->hasError ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
						ImGui::TextColored(color, "  - %s", name.c_str());
					}
				}
			}

			ImGui::Unindent();
		}
	}

	void PluginView::RenderNotifications() {
		// Render notifications in top-right corner
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 windowPos = ImVec2(io.DisplaySize.x - 350.0f, 30.0f);

		for (size_t i = 0; i < notifications.size(); ++i) {
			const auto& notification = notifications[i];

			ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y + i * 60.0f));
			ImGui::SetNextWindowSize(ImVec2(340.0f, 50.0f));

			std::string windowName = "##Notification" + std::to_string(i);
			if (ImGui::Begin(windowName.c_str(), nullptr,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing)) {

				ImGui::PushStyleColor(ImGuiCol_Text, notification.color);
				ImGui::TextWrapped("%s", notification.message.c_str());
				ImGui::PopStyleColor();

				// Progress bar showing time left
				float progress = notification.timeLeft / notificationDuration;
				ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "");
			}
			ImGui::End();
		}
	}

	void PluginView::OnPluginLoaded(const std::string& name, bool isReload) {
		std::string message = isReload ? "Reloaded: " + name : "Loaded: " + name;
		AddNotification(message, false);
	}

	void PluginView::OnPluginUnloaded(const std::string& name) {
		AddNotification("Unloaded: " + name, false);
		if (selectedPluginName == name) {
			selectedPluginName.clear();
		}
	}

	void PluginView::OnPluginError(const std::string& name, const std::string& error) {
		std::string message = "Error in " + name + ": " + error;
		AddNotification(message, true);
	}

	void PluginView::UpdateNotifications(float deltaT) {
		// Update notification timers
		for (auto& notification : notifications) {
			notification.timeLeft -= deltaT;
		}

		// Remove expired notifications
		notifications.erase(
			std::remove_if(notifications.begin(), notifications.end(),
				[](const PluginNotification& n) { return n.timeLeft <= 0.0f; }),
			notifications.end()
		);
	}

	void PluginView::AddNotification(const std::string& message, bool isError) {
		PluginNotification notification;
		notification.message = message;
		notification.isError = isError;
		notification.timeLeft = notificationDuration;
		notification.color = isError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);

		notifications.push_back(notification);

		// Limit number of notifications
		const size_t maxNotifications = 5;
		if (notifications.size() > maxNotifications) {
			notifications.erase(notifications.begin());
		}
	}

	void PluginView::RenderDirectoryControls() {
		ImGui::Text("Plugin Directory: %s", pluginManager.GetWatchDirectory().c_str());

		ImGui::SameLine();
		if (ImGui::Button("Change##Directory")) {
			showDirectoryInput = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("Open##Directory")) {
			// Platform-specific code to open file explorer
			std::string directory = pluginManager.GetWatchDirectory();
#ifdef _WIN32
			std::string command = "explorer \"" + directory + "\"";
			system(command.c_str());
#elif defined(__APPLE__)
			std::string command = "open \"" + directory + "\"";
			system(command.c_str());
#else
			std::string command = "xdg-open \"" + directory + "\"";
			system(command.c_str());
#endif
		}
	}

	void PluginView::HandleKeyboardShortcuts() {
		ImGuiIO& io = ImGui::GetIO();

		// F5 for reload
		if (ImGui::IsKeyPressed(ImGuiKey_F5) && !io.WantTextInput) {
			if (!selectedPluginName.empty()) {
				pluginManager.ReloadPlugin(selectedPluginName);
			}
		}

		// Ctrl+R for refresh
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R) && !io.WantTextInput) {
			pluginManager.RefreshPluginDirectory();
		}
	}

	ImVec4 PluginView::GetStatusColor(bool isLoaded, bool hasError) const {
		if (hasError) {
			return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for error
		}
		else if (isLoaded) {
			return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for loaded
		}
		else {
			return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray for unloaded
		}
	}

	const char* PluginView::GetStatusText(bool isLoaded, bool hasError) const {
		if (hasError) {
			return "ERROR";
		}
		else if (isLoaded) {
			return "LOADED";
		}
		else {
			return "UNLOADED";
		}
	}

} // namespace GUI