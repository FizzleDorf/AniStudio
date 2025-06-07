/*
	Fixed PluginView.cpp - Properly handles plugin loading/unloading without crashing
*/

#include "PluginView.hpp"
#include "../Events/Events.hpp"
#include <imgui.h>
#include <algorithm>

namespace GUI {

	PluginView::PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr)
		: BaseView(entityMgr), pluginManager(pluginMgr) {
		viewName = "PluginView";
	}

	PluginView::~PluginView() {
		// Clear callbacks to prevent calling into a destroyed object
		pluginManager.SetLoadCallback(nullptr);
		pluginManager.SetUnloadCallback(nullptr);
	}

	void PluginView::Init() {
		std::cout << "PluginView initialized" << std::endl;

		// Set up callbacks for plugin events with proper safety checks
		pluginManager.SetLoadCallback([this](const std::string& name, bool isReload) {
			try {
				// Check if this object is still valid by checking a member
				if (this && !viewName.empty()) {
					OnPluginLoaded(name, isReload);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in load callback: " << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << "Unknown exception in load callback" << std::endl;
			}
		});

		pluginManager.SetUnloadCallback([this](const std::string& name) {
			try {
				// Check if this object is still valid by checking a member
				if (this && !viewName.empty()) {
					OnPluginUnloaded(name);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in unload callback: " << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << "Unknown exception in unload callback" << std::endl;
			}
		});

		// Initial scan of plugins
		RefreshPluginList();
	}

	void PluginView::Render() {
		if (ImGui::Begin("Plugin Manager")) {
			// Plugin directory controls
			RenderDirectoryControls();

			ImGui::Separator();

			// Hot reload controls
			RenderHotReloadControls();

			ImGui::Separator();

			// Plugin statistics
			RenderPluginStats();

			ImGui::Separator();

			// Plugin list
			RenderPluginList();

			ImGui::Separator();

			// Plugin details (if one is selected)
			RenderPluginDetails();
		}
		ImGui::End();
	}

	void PluginView::Update(const float deltaT) {
		// Update plugin manager
		pluginManager.Update(deltaT);

		// Periodically refresh the plugin list
		refreshTimer += deltaT;
		if (refreshTimer >= 2.0f) { // Refresh every 2 seconds
			RefreshPluginList();
			refreshTimer = 0.0f;
		}
	}

	void PluginView::RefreshPluginList() {
		try {
			pluginManager.RefreshPluginDirectory();
			cachedPluginInfo = pluginManager.GetAllPluginInfo();
		}
		catch (const std::exception& e) {
			std::cerr << "Error refreshing plugin list: " << e.what() << std::endl;
		}
	}

	void PluginView::OnPluginLoaded(const std::string& name, bool isReload) {
		std::cout << "PluginView load callback called for: " << name << std::endl;

		// Add to loaded plugins list if not already there
		if (std::find(loadedPlugins.begin(), loadedPlugins.end(), name) == loadedPlugins.end()) {
			loadedPlugins.push_back(name);
		}

		// Update cached info
		RefreshPluginList();

		// Show notification
		std::string message = isReload ? "Reloaded: " + name : "Loaded: " + name;
		notifications.push_back({ message, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 3.0f });

		std::cout << "Plugin " << name << " was loaded" << std::endl;
	}

	void PluginView::OnPluginUnloaded(const std::string& name) {
		std::cout << "PluginView unload callback called for: " << name << std::endl;

		// Remove from loaded plugins list
		loadedPlugins.erase(
			std::remove(loadedPlugins.begin(), loadedPlugins.end(), name),
			loadedPlugins.end()
		);

		// Update cached info
		RefreshPluginList();

		// Show notification
		notifications.push_back({ "Unloaded: " + name, ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 3.0f });

		// Clear selection if this plugin was selected
		if (selectedPlugin == name) {
			selectedPlugin.clear();
		}

		std::cout << "Plugin " << name << " was unloaded" << std::endl;
	}

	void PluginView::RenderDirectoryControls() {
		ImGui::Text("Plugin Directory: %s", pluginManager.GetWatchDirectory().c_str());

		if (ImGui::Button("Refresh Directory")) {
			RefreshPluginList();
		}

		ImGui::SameLine();
		if (ImGui::Button("Open Directory")) {
			// Platform-specific code to open file explorer
#ifdef _WIN32
			std::string command = "explorer \"" + pluginManager.GetWatchDirectory() + "\"";
			system(command.c_str());
#elif defined(__APPLE__)
			std::string command = "open \"" + pluginManager.GetWatchDirectory() + "\"";
			system(command.c_str());
#else
			std::string command = "xdg-open \"" + pluginManager.GetWatchDirectory() + "\"";
			system(command.c_str());
#endif
		}
	}

	void PluginView::RenderHotReloadControls() {
		bool hotReloadActive = pluginManager.IsHotReloadActive();

		if (hotReloadActive) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Hot Reload: Active");
			if (ImGui::Button("Stop Hot Reload")) {
				pluginManager.StopHotReload();
			}
		}
		else {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Hot Reload: Inactive");
			if (ImGui::Button("Start Hot Reload")) {
				pluginManager.StartHotReload(pluginManager.GetWatchDirectory());
			}
		}
	}

	void PluginView::RenderPluginStats() {
		auto stats = pluginManager.GetStats();

		ImGui::Text("Total Plugins: %zu", stats.totalPlugins);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded: %zu", stats.loadedPlugins);
		ImGui::SameLine();
		if (stats.errorPlugins > 0) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Errors: %zu", stats.errorPlugins);
		}
		else {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Errors: 0");
		}
	}

	void PluginView::RenderPluginList() {
		ImGui::Text("Available Plugins:");

		if (ImGui::BeginTable("PluginsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Status");
			ImGui::TableSetupColumn("Version");
			ImGui::TableSetupColumn("Actions");
			ImGui::TableHeadersRow();

			for (const auto& plugin : cachedPluginInfo) {
				ImGui::TableNextRow();

				// Name column
				ImGui::TableNextColumn();
				if (ImGui::Selectable(plugin.name.c_str(), selectedPlugin == plugin.name, ImGuiSelectableFlags_SpanAllColumns)) {
					selectedPlugin = plugin.name;
				}

				// Status column
				ImGui::TableNextColumn();
				if (plugin.hasError) {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error");
				}
				else if (plugin.isLoaded) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
				}
				else {
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Unloaded");
				}

				// Version column
				ImGui::TableNextColumn();
				if (plugin.getVersionFunc && plugin.isLoaded) {
					ImGui::Text("%s", plugin.getVersionFunc());
				}
				else {
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Unknown");
				}

				// Actions column
				ImGui::TableNextColumn();

				ImGui::PushID(plugin.name.c_str());

				if (plugin.isLoaded) {
					if (ImGui::Button("Unload")) {
						try {
							pluginManager.UnloadPlugin(plugin.name);
						}
						catch (const std::exception& e) {
							notifications.push_back({ "Error unloading " + plugin.name + ": " + e.what(),
								ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 5.0f });
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Reload")) {
						try {
							pluginManager.ReloadPlugin(plugin.name);
						}
						catch (const std::exception& e) {
							notifications.push_back({ "Error reloading " + plugin.name + ": " + e.what(),
								ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 5.0f });
						}
					}
				}
				else {
					if (ImGui::Button("Load")) {
						try {
							std::cout << "Plugin load initiated successfully" << std::endl;
							pluginManager.LoadPlugin(plugin.path);
						}
						catch (const std::exception& e) {
							notifications.push_back({ "Error loading " + plugin.name + ": " + e.what(),
								ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 5.0f });
						}
					}
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		// Render notifications
		RenderNotifications();
	}

	void PluginView::RenderPluginDetails() {
		if (selectedPlugin.empty()) {
			return;
		}

		auto* pluginInfo = pluginManager.GetPluginInfo(selectedPlugin);
		if (!pluginInfo) {
			return;
		}

		ImGui::Text("Plugin Details: %s", selectedPlugin.c_str());

		if (ImGui::BeginTable("DetailsTable", 2, ImGuiTableFlags_Borders)) {
			ImGui::TableSetupColumn("Property");
			ImGui::TableSetupColumn("Value");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Path");
			ImGui::TableNextColumn();
			ImGui::Text("%s", pluginInfo->path.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Status");
			ImGui::TableNextColumn();
			if (pluginInfo->hasError) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", pluginInfo->errorMessage.c_str());
			}
			else if (pluginInfo->isLoaded) {
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
			}
			else {
				ImGui::Text("Unloaded");
			}

			if (pluginInfo->isLoaded && pluginInfo->getNameFunc) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Plugin Name");
				ImGui::TableNextColumn();
				ImGui::Text("%s", pluginInfo->getNameFunc());
			}

			if (pluginInfo->isLoaded && pluginInfo->getVersionFunc) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("Version");
				ImGui::TableNextColumn();
				ImGui::Text("%s", pluginInfo->getVersionFunc());
			}

			ImGui::EndTable();
		}
	}

	void PluginView::RenderNotifications() {
		// Update and render notifications
		for (auto it = notifications.begin(); it != notifications.end();) {
			it->timeRemaining -= ImGui::GetIO().DeltaTime;
			if (it->timeRemaining <= 0.0f) {
				it = notifications.erase(it);
			}
			else {
				++it;
			}
		}

		// Render active notifications
		if (!notifications.empty()) {
			ImGui::Separator();
			ImGui::Text("Notifications:");
			for (const auto& notification : notifications) {
				ImGui::TextColored(notification.color, "%s", notification.message.c_str());
			}
		}
	}

	nlohmann::json PluginView::Serialize() const {
		nlohmann::json j;
		j["viewName"] = viewName;
		j["selectedPlugin"] = selectedPlugin;
		j["loadedPlugins"] = loadedPlugins;
		return j;
	}

	void PluginView::Deserialize(const nlohmann::json& j) {
		BaseView::Deserialize(j);

		if (j.contains("selectedPlugin"))
			selectedPlugin = j["selectedPlugin"];

		if (j.contains("loadedPlugins"))
			loadedPlugins = j["loadedPlugins"];
	}

} // namespace GUI