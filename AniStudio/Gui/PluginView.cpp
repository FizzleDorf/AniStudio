#include "PluginView.hpp"
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace GUI {

	PluginView::PluginView(ECS::EntityManager& entityMgr, Plugin::PluginManager& pluginMgr)
		: BaseView(entityMgr), pluginManager(pluginMgr) {
		viewName = "Plugin Manager";

		// Initialize with default plugin directory
		std::string defaultDir = "./plugins";
		strncpy(watchDirectoryBuffer, defaultDir.c_str(), sizeof(watchDirectoryBuffer) - 1);
		watchDirectoryBuffer[sizeof(watchDirectoryBuffer) - 1] = '\0';
	}

	void PluginView::Init() {
		// Set up callbacks
		pluginManager.SetLoadCallback([this](const std::string& name, bool success) {
			OnPluginLoaded(name, success);
		});

		pluginManager.SetUnloadCallback([this](const std::string& name) {
			OnPluginUnloaded(name);
		});

		// Start hot reload if directory exists
		if (std::filesystem::exists(watchDirectoryBuffer)) {
			pluginManager.StartHotReload(watchDirectoryBuffer, std::chrono::milliseconds(hotReloadInterval));
		}

		std::cout << "PluginView initialized" << std::endl;
	}

	void PluginView::Update(const float deltaT) {
		// Update statistics
		lastStats = pluginManager.GetStats();
	}

	void PluginView::Render() {
		ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Plugin Manager")) {
			// Main tab bar
			if (ImGui::BeginTabBar("PluginTabs")) {

				// Plugin List Tab
				if (ImGui::BeginTabItem("Plugins")) {
					RenderPluginList();
					ImGui::EndTabItem();
				}

				// Hot Reload Tab
				if (ImGui::BeginTabItem("Hot Reload")) {
					RenderHotReloadControls();
					ImGui::EndTabItem();
				}

				// Statistics Tab
				if (ImGui::BeginTabItem("Statistics")) {
					RenderPluginStatistics();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			// Plugin details popup
			if (showPluginDetails) {
				RenderPluginDetails();
			}

			// File dialog for loading plugins
			if (ImGuiFileDialog::Instance()->Display("LoadPluginDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					LoadPluginFromDialog();
				}
				ImGuiFileDialog::Instance()->Close();
			}
		}
		ImGui::End();
	}

	void PluginView::RenderPluginList() {
		// Load controls
		RenderLoadControls();

		ImGui::Separator();

		// Plugin table
		if (ImGui::BeginTable("PluginTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
			ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {

			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80);
			ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 80);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120);
			ImGui::TableHeadersRow();

			auto plugins = pluginManager.GetAllPluginInfo();

			for (const auto& plugin : plugins) {
				ImGui::TableNextRow();

				// Name
				ImGui::TableNextColumn();
				if (ImGui::Selectable(plugin.name.c_str(), selectedPlugin == plugin.name,
					ImGuiSelectableFlags_SpanAllColumns)) {
					selectedPlugin = plugin.name;
				}

				// Double-click to show details
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
					showPluginDetails = true;
				}

				// Status
				ImGui::TableNextColumn();
				ImVec4 statusColor = GetStatusColor(plugin);
				ImGui::TextColored(statusColor, "%s", GetStatusText(plugin));

				// Version
				ImGui::TableNextColumn();
				if (plugin.getVersionFunc && plugin.isLoaded) {
					ImGui::Text("%s", plugin.getVersionFunc());
				}
				else {
					ImGui::TextDisabled("N/A");
				}

				// Path
				ImGui::TableNextColumn();
				std::filesystem::path p(plugin.path);
				std::string shortPath = p.filename().string();
				ImGui::Text("%s", shortPath.c_str());
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", plugin.path.c_str());
				}

				// Actions
				ImGui::TableNextColumn();
				ImGui::PushID(plugin.name.c_str());

				if (plugin.isLoaded) {
					if (ImGui::SmallButton("Unload")) {
						pluginManager.UnloadPlugin(plugin.name);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Reload")) {
						pluginManager.ReloadPlugin(plugin.name);
					}
				}
				else {
					if (ImGui::SmallButton("Load")) {
						pluginManager.LoadPlugin(plugin.path);
					}
				}

				ImGui::SameLine();
				if (ImGui::SmallButton("Details")) {
					selectedPlugin = plugin.name;
					showPluginDetails = true;
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		// Context menu
		if (ImGui::BeginPopupContextWindow()) {
			if (ImGui::MenuItem("Refresh Directory")) {
				pluginManager.RefreshPluginDirectory();
			}
			if (ImGui::MenuItem("Unload All")) {
				pluginManager.UnloadAllPlugins();
			}
			ImGui::EndPopup();
		}
	}

	void PluginView::RenderPluginDetails() {
		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Plugin Details", &showPluginDetails)) {
			auto* info = pluginManager.GetPluginInfo(selectedPlugin);
			if (!info) {
				ImGui::Text("Plugin not found: %s", selectedPlugin.c_str());
			}
			else {
				// Basic info
				ImGui::Text("Name: %s", info->name.c_str());
				ImGui::Text("Path: %s", info->path.c_str());

				if (info->getNameFunc && info->isLoaded) {
					ImGui::Text("Display Name: %s", info->getNameFunc());
				}

				if (info->getVersionFunc && info->isLoaded) {
					ImGui::Text("Version: %s", info->getVersionFunc());
				}

				// Status
				ImVec4 statusColor = GetStatusColor(*info);
				ImGui::TextColored(statusColor, "Status: %s", GetStatusText(*info));

				if (info->hasError) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", info->errorMessage.c_str());
				}

				// File info
				if (std::filesystem::exists(info->path)) {
					auto fileSize = std::filesystem::file_size(info->path);
					ImGui::Text("File Size: %s", FormatFileSize(fileSize).c_str());

					auto writeTime = std::filesystem::last_write_time(info->path);
					auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
						writeTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
					auto cftime = std::chrono::system_clock::to_time_t(sctp);
					ImGui::Text("Last Modified: %s", std::ctime(&cftime));
				}

				ImGui::Separator();

				// Actions
				if (info->isLoaded) {
					if (ImGui::Button("Unload")) {
						pluginManager.UnloadPlugin(info->name);
						showPluginDetails = false;
					}
					ImGui::SameLine();
					if (ImGui::Button("Reload")) {
						pluginManager.ReloadPlugin(info->name);
					}
				}
				else {
					if (ImGui::Button("Load")) {
						pluginManager.LoadPlugin(info->path);
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Close")) {
					showPluginDetails = false;
				}
			}
		}
		ImGui::End();
	}

	void PluginView::RenderHotReloadControls() {
		// Hot reload status
		bool hotReloadActive = pluginManager.IsHotReloadActive();
		ImGui::Text("Hot Reload Status: %s", hotReloadActive ? "Active" : "Inactive");

		ImGui::Separator();

		// Watch directory
		ImGui::Text("Watch Directory:");
		ImGui::SameLine();
		if (ImGui::InputText("##WatchDir", watchDirectoryBuffer, sizeof(watchDirectoryBuffer))) {
			// Directory changed
		}
		ImGui::SameLine();
		if (ImGui::Button("Browse##WatchDir")) {
			IGFD::FileDialogConfig config;
			config.path = watchDirectoryBuffer;
			ImGuiFileDialog::Instance()->OpenDialog("ChooseWatchDir", "Choose Watch Directory", nullptr, config);
		}

		// Check interval
		ImGui::Text("Check Interval (ms):");
		ImGui::SameLine();
		ImGui::SliderInt("##Interval", &hotReloadInterval, 100, 5000);

		// Auto-load new plugins
		ImGui::Checkbox("Auto-load new plugins", &autoLoadNewPlugins);

		ImGui::Separator();

		// Controls
		if (hotReloadActive) {
			if (ImGui::Button("Stop Hot Reload")) {
				pluginManager.StopHotReload();
			}
			ImGui::SameLine();
			if (ImGui::Button("Restart with New Settings")) {
				pluginManager.StopHotReload();
				pluginManager.StartHotReload(watchDirectoryBuffer, std::chrono::milliseconds(hotReloadInterval));
			}
		}
		else {
			if (ImGui::Button("Start Hot Reload")) {
				pluginManager.StartHotReload(watchDirectoryBuffer, std::chrono::milliseconds(hotReloadInterval));
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Refresh Directory")) {
			pluginManager.RefreshPluginDirectory();
		}

		// Directory browser dialog
		if (ImGuiFileDialog::Instance()->Display("ChooseWatchDir", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string selectedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
				strncpy(watchDirectoryBuffer, selectedDir.c_str(), sizeof(watchDirectoryBuffer) - 1);
				watchDirectoryBuffer[sizeof(watchDirectoryBuffer) - 1] = '\0';
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	void PluginView::RenderPluginStatistics() {
		ImGui::Text("Plugin Statistics");
		ImGui::Separator();

		// Basic stats
		ImGui::Text("Total Plugins: %zu", lastStats.totalPlugins);
		ImGui::Text("Loaded Plugins: %zu", lastStats.loadedPlugins);
		ImGui::Text("Error Plugins: %zu", lastStats.errorPlugins);
		ImGui::Text("Hot Reload Interval: %s", FormatDuration(lastStats.lastCheckTime).c_str());

		// Progress bars
		if (lastStats.totalPlugins > 0) {
			float loadedRatio = static_cast<float>(lastStats.loadedPlugins) / lastStats.totalPlugins;
			ImGui::Text("Load Success Rate:");
			ImGui::ProgressBar(loadedRatio, ImVec2(-1, 0), "");
			ImGui::SameLine();
			ImGui::Text("%.1f%%", loadedRatio * 100.0f);
		}

		ImGui::Separator();

		// Plugin list with details
		ImGui::Text("Plugin Details:");

		auto plugins = pluginManager.GetAllPluginInfo();
		for (const auto& plugin : plugins) {
			ImGui::BulletText("%s", plugin.name.c_str());
			ImGui::Indent();
			ImGui::Text("Status: %s", GetStatusText(plugin));
			if (plugin.hasError) {
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", plugin.errorMessage.c_str());
			}
			ImGui::Unindent();
		}
	}

	void PluginView::RenderLoadControls() {
		if (ImGui::Button("Load Plugin")) {
			IGFD::FileDialogConfig config;
			config.path = watchDirectoryBuffer;
#ifdef _WIN32
			ImGuiFileDialog::Instance()->OpenDialog("LoadPluginDialog", "Load Plugin", ".dll", config);
#else
			ImGuiFileDialog::Instance()->OpenDialog("LoadPluginDialog", "Load Plugin", ".so", config);
#endif
		}

		ImGui::SameLine();
		if (ImGui::Button("Unload All")) {
			pluginManager.UnloadAllPlugins();
		}

		ImGui::SameLine();
		if (ImGui::Button("Refresh")) {
			pluginManager.RefreshPluginDirectory();
		}
	}

	void PluginView::LoadPluginFromDialog() {
		std::string pluginPath = ImGuiFileDialog::Instance()->GetFilePathName();
		if (!pluginPath.empty()) {
			pluginManager.LoadPlugin(pluginPath);
		}
	}

	void PluginView::RefreshPluginList() {
		pluginManager.RefreshPluginDirectory();
	}

	const char* PluginView::GetStatusText(const Plugin::PluginManager::PluginInfo& info) {
		if (info.hasError) {
			return "Error";
		}
		else if (info.isLoaded) {
			return "Loaded";
		}
		else {
			return "Unloaded";
		}
	}

	ImVec4 PluginView::GetStatusColor(const Plugin::PluginManager::PluginInfo& info) {
		if (info.hasError) {
			return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
		}
		else if (info.isLoaded) {
			return ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green
		}
		else {
			return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Gray
		}
	}

	std::string PluginView::FormatFileSize(size_t bytes) {
		const char* units[] = { "B", "KB", "MB", "GB" };
		int unit = 0;
		double size = static_cast<double>(bytes);

		while (size >= 1024.0 && unit < 3) {
			size /= 1024.0;
			unit++;
		}

		std::ostringstream oss;
		oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
		return oss.str();
	}

	std::string PluginView::FormatDuration(std::chrono::milliseconds ms) {
		auto count = ms.count();
		if (count < 1000) {
			return std::to_string(count) + "ms";
		}
		else {
			return std::to_string(count / 1000) + "s";
		}
	}

	void PluginView::OnPluginLoaded(const std::string& name, bool success) {
		std::cout << "Plugin " << name << (success ? " loaded successfully" : " failed to load") << std::endl;
	}

	void PluginView::OnPluginUnloaded(const std::string& name) {
		std::cout << "Plugin " << name << " unloaded" << std::endl;

		// Clear selection if the unloaded plugin was selected
		if (selectedPlugin == name) {
			selectedPlugin.clear();
			showPluginDetails = false;
		}
	}

	nlohmann::json PluginView::Serialize() const {
		nlohmann::json j = BaseView::Serialize();
		j["watchDirectory"] = std::string(watchDirectoryBuffer);
		j["hotReloadInterval"] = hotReloadInterval;
		j["autoLoadNewPlugins"] = autoLoadNewPlugins;
		return j;
	}

	void PluginView::Deserialize(const nlohmann::json& j) {
		BaseView::Deserialize(j);

		if (j.contains("watchDirectory")) {
			std::string dir = j["watchDirectory"];
			strncpy(watchDirectoryBuffer, dir.c_str(), sizeof(watchDirectoryBuffer) - 1);
			watchDirectoryBuffer[sizeof(watchDirectoryBuffer) - 1] = '\0';
		}

		if (j.contains("hotReloadInterval")) {
			hotReloadInterval = j["hotReloadInterval"];
		}

		if (j.contains("autoLoadNewPlugins")) {
			autoLoadNewPlugins = j["autoLoadNewPlugins"];
		}
	}

} // namespace GUI