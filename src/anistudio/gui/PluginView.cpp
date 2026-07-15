// PluginView.cpp
#include "PluginView.hpp"
#include "BasePlugin.hpp"
#include "Events.hpp"
#include "FileDialogUtil.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <fstream>
#include <imgui.h>

namespace GUI {

    PluginView::PluginView(ECS::EntityManager& entityMgr, Plugins::StudioPluginManager& pluginMgr)
        : BaseView(entityMgr), m_pluginManager(pluginMgr) {
        viewName = "PluginView";
    }

    void PluginView::Init() {
        std::filesystem::path executablePath = std::filesystem::current_path();
        std::filesystem::path buildPath = executablePath.parent_path();

        std::filesystem::path pluginsPath = buildPath / "plugins";
        m_searchDirectories.push_back(PluginDirectoryInfo(pluginsPath, true));

        std::filesystem::path addonsPath = buildPath / "addons";
        m_searchDirectories.push_back(PluginDirectoryInfo(addonsPath, true));

        std::filesystem::create_directories(pluginsPath);
        std::filesystem::create_directories(addonsPath);

        m_pluginManager.setStagingDirectory(pluginsPath.string());

        RefreshAvailablePlugins();
        LoadPluginScopeConfig();
        SelectAllDirectories();

        m_pluginManager.enableHotReload(m_hotReloadEnabled);

        std::cout << "[PluginView] Initialized with " << m_searchDirectories.size() << " search directories" << std::endl;
        std::cout << "[PluginView] Current path: " << executablePath << std::endl;
        std::cout << "[PluginView] Build path: " << buildPath << std::endl;
        std::cout << "[PluginView] Plugin search path: " << pluginsPath << std::endl;
        std::cout << "[PluginView] Addon search path: " << addonsPath << std::endl;
    }

    void PluginView::Update(float deltaT) {
        if (m_statusTimer > 0.0f) {
            m_statusTimer -= deltaT;
            if (m_statusTimer <= 0.0f) {
                m_statusMessage.clear();
            }
        }
    }

    void PluginView::Render() {
        if (!m_hotReloadWasEnabled && m_hotReloadEnabled) {
            m_pluginManager.enableHotReload(true);
            m_hotReloadWasEnabled = true;
            std::cout << "[PluginView] Hot reload re-enabled because PluginView opened" << std::endl;
        }

        ImGui::SetNextWindowSize(ImVec2(1400, 800), ImGuiCond_FirstUseEver);

        bool windowShouldClose = false;
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowShouldClose)) {
            if (windowShouldClose) {
                if (m_hotReloadWasEnabled) {
                    m_pluginManager.enableHotReload(false);
                    m_hotReloadWasEnabled = false;
                    std::cout << "[PluginView] Hot reload disabled because PluginView was closed" << std::endl;
                }

                std::unordered_map<std::string, std::any> eventData;
                eventData["workspaceID"] = GetID();
                eventData["viewTypeName"] = viewName;
                ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
            }

            RenderMainContent();
        }
        ImGui::End();

        windowOpen = !windowShouldClose;
    }

    void PluginView::RenderMainContent() {
        ImGui::BeginChild("PluginSplit", ImVec2(0, -30), false);

        ImGui::BeginChild("FilterList", ImVec2(m_filterListWidth, 0), true);
        RenderFilterList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ContentArea", ImVec2(0, 0), true);
        RenderPluginLists();
        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::Separator();
        RenderStatusBar();
    }

    void PluginView::RenderPluginLists() {
        float availableHeight = ImGui::GetContentRegionAvail().y;
        float listsHeight = availableHeight * 0.5f;

        ImGui::BeginChild("PluginListsArea", ImVec2(0, listsHeight), true);
        {
            if (ImGui::BeginTable("PluginListsTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Available Plugins", ImGuiTableColumnFlags_WidthFixed, 400.0f);
                ImGui::TableSetupColumn("Active Plugins", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::BeginChild("AvailablePluginsColumn", ImVec2(0, 0), false);
                RenderAvailablePluginsList();
                ImGui::EndChild();

                ImGui::TableSetColumnIndex(1);
                ImGui::BeginChild("ActivePluginsColumn", ImVec2(0, 0), false);
                RenderActivePluginsList();
                ImGui::EndChild();

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("PluginDetailsArea", ImVec2(0, 0), true);
        RenderPluginDetails();
        ImGui::EndChild();
    }

    void PluginView::RenderFilterList() {
        ImGui::Text("Plugin Search Directories");
        ImGui::Separator();

        if (ImGui::Button("Add Directory", ImVec2(-1, 0))) {
            std::string selectedPath;
            if (FileDialog::SelectFolder("Select Plugin Directory", selectedPath)) {
                if (!selectedPath.empty()) {
                    AddSearchDirectory(std::filesystem::path(selectedPath));
                    RefreshAvailablePlugins();
                }
            }
        }

        if (ImGui::Button("Refresh All", ImVec2(-1, 0))) {
            RefreshAvailablePlugins();
            ShowStatus("Refreshed plugin directories", 2.0f);
        }

        ImGui::Separator();

        if (ImGui::Button("Show All", ImVec2(-1, 0))) {
            SelectAllDirectories();
        }
        if (ImGui::Button("Deselect All", ImVec2(-1, 0))) {
            DeselectAllDirectories();
        }

        ImGui::Separator();

        for (const auto& dirInfo : m_searchDirectories) {
            std::string displayName = dirInfo.displayName;
            if (dirInfo.isDefault) {
                displayName += " [Default]";
            }

            bool isSelected = m_selectedDirectories.find(dirInfo.displayName) != m_selectedDirectories.end();

            ImGui::PushStyleColor(ImGuiCol_Text, isSelected ?
                ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

            bool clicked = ImGui::Selectable(
                displayName.c_str(),
                isSelected,
                ImGuiSelectableFlags_AllowDoubleClick
            );

            ImGui::PopStyleColor();

            if (clicked) {
                HandleDirectorySelection(dirInfo.displayName,
                    ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Path: %s", dirInfo.path.string().c_str());
                ImGui::Text("Type: %s", dirInfo.isDefault ? "Default" : "Custom");
                ImGui::EndTooltip();
            }

            if (!dirInfo.isDefault && ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove Directory")) {
                    RemoveSearchDirectory(dirInfo.path);
                    RefreshAvailablePlugins();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::Separator();
        ImGui::Text("Selected: %zu/%zu", m_selectedDirectories.size(), m_searchDirectories.size());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+Click: Multi-select");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Right-Click: Options");
    }

    void PluginView::RenderAvailablePluginsList() {
        ImGui::BeginChild("AvailablePluginsList", ImVec2(0, 0), false);
        ImGui::Text("Available Plugins in Selected Directories");
        ImGui::Separator();

        std::vector<AvailablePluginInfo*> filteredPlugins;
        for (auto& plugin : m_availablePlugins) {
            if (IsPluginInSelectedDirectories(plugin)) {
                filteredPlugins.push_back(&plugin);
            }
        }

        if (filteredPlugins.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No plugins found in selected directories");
        }
        else {
            for (auto* plugin : filteredPlugins) {
                ImVec4 color;
                if (plugin->isLoaded && plugin->isEnabled) {
                    color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                }
                else if (plugin->isLoaded) {
                    color = ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
                }
                else {
                    color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                bool isSelected = (m_selectedPluginForDetails == plugin->name);

                if (ImGui::Selectable(plugin->name.c_str(), isSelected)) {
                    m_selectedPluginForDetails = plugin->name;
                }

                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", plugin->name.c_str());
                    ImGui::Text("Directory: %s", plugin->directory.string().c_str());
                    ImGui::Text("Source: %s", plugin->sourceDirectory.c_str());
                    ImGui::Text("Status: %s", plugin->isLoaded ? (plugin->isEnabled ? "Active" : "Loaded") : "Not Loaded");
                    ImGui::EndTooltip();
                }

                if (ImGui::BeginPopupContextItem()) {
                    if (!plugin->isLoaded) {
                        if (ImGui::MenuItem("Load Plugin")) {
                            LoadPlugin(plugin->name);
                        }
                    }
                    else {
                        if (!plugin->isEnabled) {
                            if (ImGui::MenuItem("Enable Plugin")) {
                                EnablePlugin(plugin->name);
                            }
                        }
                        else {
                            if (ImGui::MenuItem("Disable Plugin")) {
                                DisablePlugin(plugin->name);
                            }
                        }
                        if (ImGui::MenuItem("Reload Plugin")) {
                            ReloadPlugin(plugin->name);
                        }
                        if (ImGui::MenuItem("Unload Plugin")) {
                            UnloadPlugin(plugin->name);
                        }
                    }
                    ImGui::EndPopup();
                }
            }
        }

        ImGui::EndChild();
        ImGui::Text("Plugins: %zu", filteredPlugins.size());
    }

    void PluginView::RenderActivePluginsList() {
        ImGui::BeginChild("ActivePluginsList", ImVec2(0, 0), false);
        ImGui::Text("Active Plugins");
        ImGui::Separator();

        auto loadedPlugins = m_pluginManager.getLoadedPlugins();

        std::vector<const Plugins::PluginInfo*> activePlugins;
        for (const auto& plugin : loadedPlugins) {
            if (plugin.enabled) {
                activePlugins.push_back(&plugin);
            }
        }

        if (activePlugins.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No active plugins");
        }
        else {
            for (const auto* plugin : activePlugins) {
                ImVec4 stateColor = GetPluginStateColor(*plugin);
                std::string displayName = plugin->name;

                if (plugin->hotReloadPending) {
                    displayName += " [HOT RELOAD PENDING v" + std::to_string(plugin->nextVersion) + "]";
                    stateColor = ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
                }
                else if (plugin->currentVersion > 0) {
                    displayName += " [v" + std::to_string(plugin->currentVersion) + "]";
                }

                ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
                bool isSelected = (m_selectedPluginForDetails == plugin->name);

                if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                    m_selectedPluginForDetails = plugin->name;
                }
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", plugin->name.c_str());
                    ImGui::Text("Version: %s", plugin->version.c_str());
                    ImGui::Text("DLL Version: v%u", plugin->currentVersion);
                    ImGui::Text("Path: %s", plugin->path.c_str());
                    if (plugin->hotReloadPending) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Hot reload pending!");
                    }
                    ImGui::EndTooltip();
                }

                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Disable Plugin")) {
                        DisablePlugin(plugin->name);
                    }
                    if (ImGui::MenuItem("Reload Plugin")) {
                        ReloadPlugin(plugin->name);
                    }
                    if (ImGui::MenuItem("Unload Plugin")) {
                        UnloadPlugin(plugin->name);
                    }
                    ImGui::EndPopup();
                }
            }
        }

        ImGui::EndChild();
        ImGui::Text("Active: %zu", activePlugins.size());
    }

    void PluginView::RenderPluginDetails() {
        if (m_selectedPluginForDetails.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a plugin to view details");
        }
        else {
            auto loadedPlugins = m_pluginManager.getLoadedPlugins();
            auto pluginIt = std::find_if(loadedPlugins.begin(), loadedPlugins.end(),
                [this](const Plugins::PluginInfo& p) { return p.name == m_selectedPluginForDetails; });

            if (pluginIt != loadedPlugins.end()) {
                const auto& plugin = *pluginIt;
                auto* availPlugin = FindAvailablePlugin(m_selectedPluginForDetails);

                ImGui::Text("Plugin Details");
                ImGui::Separator();

                ImGui::Text("Name: %s", plugin.name.c_str());
                ImGui::Text("Version: %s", plugin.version.c_str());
                ImGui::Text("Path: %s", plugin.path.c_str());

                ImVec4 stateColor = GetPluginStateColor(plugin);
                ImGui::TextColored(stateColor, "State: %s", GetPluginStateText(plugin));

                ImGui::Separator();

                ImGui::Text("Load Scope:");
                ImGui::SameLine();
                if (availPlugin) {
                    bool isProject = availPlugin->isProjectScope;
                    if (ImGui::RadioButton("Global", !isProject)) {
                        SetPluginScope(plugin.name, false);
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Project", isProject)) {
                        SetPluginScope(plugin.name, true);
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                        isProject ? "(saved per-project)" : "(saved globally)");
                }

                ImGui::Separator();

                ImGui::Text("Versioned Hot Reload Information:");
                ImGui::Text("Current DLL Version: v%u", plugin.currentVersion);
                ImGui::Text("Next DLL Version: v%u", plugin.nextVersion);
                if (!plugin.activeDllPath.empty()) {
                    ImGui::Text("Active DLL: %s", plugin.activeDllPath.c_str());
                }
                if (!plugin.stagingPath.empty()) {
                    ImGui::Text("Staging Dir: %s", plugin.stagingPath.c_str());
                }

                if (plugin.hotReloadPending) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f),
                        "Versioned hot reload pending! Will create v%u", plugin.nextVersion);
                    ImGui::Text("Next update cycle will reload this plugin with new version");
                }

                ImGui::Separator();

                if (plugin.loaded) {
                    if (plugin.enabled) {
                        if (ImGui::Button("Disable", ImVec2(100, 0))) {
                            DisablePlugin(plugin.name);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reload", ImVec2(100, 0))) {
                            ReloadPlugin(plugin.name);
                        }
                    }
                    else {
                        if (ImGui::Button("Enable", ImVec2(100, 0))) {
                            EnablePlugin(plugin.name);
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Unload", ImVec2(100, 0))) {
                        UnloadPlugin(plugin.name);
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Plugin not loaded");
                }

                ImGui::Separator();

                if (plugin.instance) {
                    ImGui::Text("Plugin Instance: Available");
                    ImGui::Text("Initialized: %s", plugin.instance->IsInitialized() ? "Yes" : "No");
                }
                else {
                    ImGui::Text("Plugin Instance: None");
                }
            }
            else {
                auto* availablePlugin = FindAvailablePlugin(m_selectedPluginForDetails);
                if (availablePlugin) {
                    ImGui::Text("Available Plugin Details");
                    ImGui::Separator();

                    ImGui::Text("Name: %s", availablePlugin->name.c_str());
                    ImGui::Text("Directory: %s", availablePlugin->directory.string().c_str());
                    ImGui::Text("Source Directory: %s", availablePlugin->sourceDirectory.c_str());
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Status: Not Loaded");

                    ImGui::Separator();

                    ImGui::Text("Load Scope:");
                    ImGui::SameLine();
                    bool isProject = availablePlugin->isProjectScope;
                    if (ImGui::RadioButton("Global", !isProject)) {
                        availablePlugin->isProjectScope = false;
                        SavePluginScopeConfig();
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Project", isProject)) {
                        availablePlugin->isProjectScope = true;
                        SavePluginScopeConfig();
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                        isProject ? "(will save per-project)" : "(will save globally)");

                    ImGui::Separator();

                    if (ImGui::Button("Load Plugin", ImVec2(150, 0))) {
                        LoadPlugin(availablePlugin->name);
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Selected plugin not found");
                }
            }
        }
    }

    void PluginView::RenderStatusBar() {
        if (!m_statusMessage.empty()) {
            float alpha = std::min(1.0f, m_statusTimer / 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
            ImGui::Text("%s", m_statusMessage.c_str());
            ImGui::PopStyleColor();
        }
        else {
            bool actualHotReloadState = m_pluginManager.isHotReloadEnabled();
            ImGui::Text("Ready - Hot reload: %s | Available: %zu | Active: %zu",
                actualHotReloadState ? "ENABLED" : "DISABLED",
                m_availablePlugins.size(),
                std::count_if(m_availablePlugins.begin(), m_availablePlugins.end(),
                    [](const AvailablePluginInfo& p) { return p.isEnabled; }));
        }
    }

    void PluginView::RefreshAvailablePlugins() {
        m_availablePlugins.clear();

        for (const auto& dirInfo : m_searchDirectories) {
            if (std::filesystem::exists(dirInfo.path)) {
                DiscoverPluginsInDirectory(dirInfo.path);
            }
        }

        auto loadedPlugins = m_pluginManager.getLoadedPlugins();
        for (auto& availPlugin : m_availablePlugins) {
            auto it = std::find_if(loadedPlugins.begin(), loadedPlugins.end(),
                [&](const Plugins::PluginInfo& p) { return p.name == availPlugin.name; });

            if (it != loadedPlugins.end()) {
                availPlugin.isLoaded = it->loaded;
                availPlugin.isEnabled = it->enabled;
                availPlugin.currentVersion = it->currentVersion;
            }
        }

        std::cout << "[PluginView] Discovered " << m_availablePlugins.size() << " plugins" << std::endl;
    }

    void PluginView::DiscoverPluginsInDirectory(const std::filesystem::path& searchDir) {
        try {
            if (!std::filesystem::exists(searchDir)) {
                std::cout << "[PluginView] Search directory does not exist: " << searchDir << std::endl;
                return;
            }

            std::cout << "[PluginView] Scanning directory: " << searchDir << std::endl;

            for (const auto& entry : std::filesystem::directory_iterator(searchDir)) {
                if (entry.is_directory()) {
                    std::string dirName = entry.path().filename().string();

                    std::cout << "[PluginView] Found subdirectory: " << dirName << std::endl;

                    if (dirName == "staging") {
                        std::cout << "[PluginView] Skipping staging directory" << std::endl;
                        continue;
                    }

                    auto it = std::find_if(m_availablePlugins.begin(), m_availablePlugins.end(),
                        [&](const AvailablePluginInfo& p) { return p.name == dirName; });

                    if (it == m_availablePlugins.end()) {
                        m_availablePlugins.push_back(
                            AvailablePluginInfo(dirName, entry.path(), searchDir.filename().string())
                        );
                        std::cout << "[PluginView] Added plugin: " << dirName << " from " << searchDir.filename().string() << std::endl;
                    }
                    else {
                        std::cout << "[PluginView] Plugin already exists: " << dirName << std::endl;
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginView] Error discovering plugins in " << searchDir << ": " << e.what() << std::endl;
        }
    }

    void PluginView::AddSearchDirectory(const std::filesystem::path& newDir) {
        auto it = std::find_if(m_searchDirectories.begin(), m_searchDirectories.end(),
            [&](const PluginDirectoryInfo& info) { return info.path == newDir; });

        if (it == m_searchDirectories.end()) {
            m_searchDirectories.push_back(PluginDirectoryInfo(newDir, false));
            ShowStatus("Added search directory: " + newDir.filename().string(), 3.0f);
            std::cout << "[PluginView] Added search directory: " << newDir << std::endl;
        }
        else {
            ShowStatus("Directory already in search list", 3.0f);
        }
    }

    void PluginView::RemoveSearchDirectory(const std::filesystem::path& dir) {
        auto it = std::find_if(m_searchDirectories.begin(), m_searchDirectories.end(),
            [&](const PluginDirectoryInfo& info) { return info.path == dir; });

        if (it != m_searchDirectories.end() && !it->isDefault) {
            std::string dirName = it->displayName;
            m_searchDirectories.erase(it);
            m_selectedDirectories.erase(dirName);
            ShowStatus("Removed search directory: " + dirName, 3.0f);
            std::cout << "[PluginView] Removed search directory: " << dir << std::endl;
        }
    }

    void PluginView::LoadPlugin(const std::string& pluginName) {
        auto* availPlugin = FindAvailablePlugin(pluginName);
        if (!availPlugin) {
            ShowStatus("Plugin not found: " + pluginName, 5.0f);
            return;
        }

        std::cout << "[PluginView] Loading plugin: " << pluginName << " from " << availPlugin->directory << std::endl;

        if (m_pluginManager.loadPlugin(availPlugin->directory.string())) {
            availPlugin->isLoaded = true;
            ShowStatus("Loaded plugin: " + pluginName, 3.0f);
            std::cout << "[PluginView] Successfully loaded plugin: " << pluginName << std::endl;

            if (availPlugin->isProjectScope) {
                m_pluginManager.SaveProjectPluginState();
                std::cout << "[PluginView] Auto-saved project plugin state for: " << pluginName << std::endl;
            }
            else {
                m_pluginManager.SaveGlobalPluginState();
                std::cout << "[PluginView] Auto-saved global plugin state for: " << pluginName << std::endl;
            }
        }
        else {
            ShowStatus("Failed to load plugin: " + pluginName, 5.0f);
            std::cerr << "[PluginView] Failed to load plugin: " << pluginName << std::endl;
        }
    }

    void PluginView::EnablePlugin(const std::string& pluginName) {
        if (m_pluginManager.enablePlugin(pluginName)) {
            auto* availPlugin = FindAvailablePlugin(pluginName);
            if (availPlugin) {
                availPlugin->isEnabled = true;

                if (availPlugin->isProjectScope) {
                    m_pluginManager.SaveProjectPluginState();
                    std::cout << "[PluginView] Auto-saved project plugin state" << std::endl;
                }
                else {
                    m_pluginManager.SaveGlobalPluginState();
                    std::cout << "[PluginView] Auto-saved global plugin state" << std::endl;
                }
            }
            ShowStatus("Enabled plugin: " + pluginName, 3.0f);
        }
        else {
            ShowStatus("Failed to enable plugin: " + pluginName, 5.0f);
        }
    }

    void PluginView::DisablePlugin(const std::string& pluginName) {
        if (m_pluginManager.disablePlugin(pluginName)) {
            auto* availPlugin = FindAvailablePlugin(pluginName);
            if (availPlugin) {
                availPlugin->isEnabled = false;

                if (availPlugin->isProjectScope) {
                    m_pluginManager.SaveProjectPluginState();
                    std::cout << "[PluginView] Auto-saved project plugin state" << std::endl;
                }
                else {
                    m_pluginManager.SaveGlobalPluginState();
                    std::cout << "[PluginView] Auto-saved global plugin state" << std::endl;
                }
            }
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
        auto* availPlugin = FindAvailablePlugin(pluginName);

        if (m_pluginManager.unloadPlugin(pluginName)) {
            if (availPlugin) {
                availPlugin->isLoaded = false;
                availPlugin->isEnabled = false;
                availPlugin->currentVersion = 0;
            }
            ShowStatus("Unloaded plugin: " + pluginName, 3.0f);
        }
        else {
            ShowStatus("Failed to unload plugin: " + pluginName, 5.0f);
        }
    }

    void PluginView::SetPluginScope(const std::string& pluginName, bool isProjectScope) {
        auto* availPlugin = FindAvailablePlugin(pluginName);
        if (availPlugin) {
            availPlugin->isProjectScope = isProjectScope;
            SavePluginScopeConfig();
            ShowStatus(std::string("Set ") + pluginName + " to " + (isProjectScope ? "project" : "global") + " scope", 3.0f);
            std::cout << "[PluginView] " << pluginName << " scope changed to: " << (isProjectScope ? "project" : "global") << std::endl;
        }
    }

    void PluginView::SavePluginScopeConfig() {
        try {
            nlohmann::json j;
            j["version"] = "1.0";
            j["pluginScopes"] = nlohmann::json::object();

            for (const auto& plugin : m_availablePlugins) {
                j["pluginScopes"][plugin.name] = plugin.isProjectScope;
            }

            std::filesystem::path dataPath = std::filesystem::current_path().parent_path() / "data";
            std::filesystem::create_directories(dataPath);
            std::string filepath = (dataPath / "plugin_scopes.json").string();

            std::ofstream file(filepath);
            if (file.is_open()) {
                file << j.dump(4);
                std::cout << "[PluginView] Saved plugin scope configuration to: " << filepath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginView] Failed to save plugin scope config: " << e.what() << std::endl;
        }
    }

    void PluginView::LoadPluginScopeConfig() {
        try {
            std::filesystem::path dataPath = std::filesystem::current_path().parent_path() / "data";
            std::string filepath = (dataPath / "plugin_scopes.json").string();

            if (!std::filesystem::exists(filepath)) {
                std::cout << "[PluginView] No plugin scope configuration found" << std::endl;
                return;
            }

            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[PluginView] Failed to open plugin scope config: " << filepath << std::endl;
                return;
            }

            nlohmann::json j;
            file >> j;

            if (j.contains("pluginScopes") && j["pluginScopes"].is_object()) {
                for (auto& plugin : m_availablePlugins) {
                    if (j["pluginScopes"].contains(plugin.name)) {
                        plugin.isProjectScope = j["pluginScopes"][plugin.name];
                        std::cout << "[PluginView] Loaded scope for " << plugin.name << ": "
                            << (plugin.isProjectScope ? "project" : "global") << std::endl;
                    }
                }
            }

            std::cout << "[PluginView] Plugin scope configuration loaded from: " << filepath << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginView] Failed to load plugin scope config: " << e.what() << std::endl;
        }
    }

    void PluginView::HandleDirectorySelection(const std::string& dirName, bool ctrlHeld, bool shiftHeld) {
        if (!ctrlHeld && !shiftHeld) {
            m_selectedDirectories.clear();
            m_selectedDirectories.insert(dirName);
            m_lastSelectedDirectory = dirName;
        }
        else if (ctrlHeld) {
            if (m_selectedDirectories.count(dirName)) {
                m_selectedDirectories.erase(dirName);
            }
            else {
                m_selectedDirectories.insert(dirName);
            }
            m_lastSelectedDirectory = dirName;
        }
        else if (shiftHeld && !m_lastSelectedDirectory.empty()) {
            std::vector<std::string> dirOrder;
            for (const auto& dir : m_searchDirectories) {
                dirOrder.push_back(dir.displayName);
            }

            auto startIt = std::find(dirOrder.begin(), dirOrder.end(), m_lastSelectedDirectory);
            auto endIt = std::find(dirOrder.begin(), dirOrder.end(), dirName);

            if (startIt != dirOrder.end() && endIt != dirOrder.end()) {
                if (startIt > endIt) std::swap(startIt, endIt);

                for (auto it = startIt; it <= endIt; ++it) {
                    m_selectedDirectories.insert(*it);
                }
            }
        }
    }

    void PluginView::SelectAllDirectories() {
        m_selectedDirectories.clear();
        for (const auto& dir : m_searchDirectories) {
            m_selectedDirectories.insert(dir.displayName);
        }
    }

    void PluginView::DeselectAllDirectories() {
        m_selectedDirectories.clear();
    }

    bool PluginView::IsPluginInSelectedDirectories(const AvailablePluginInfo& plugin) const {
        if (m_selectedDirectories.empty()) {
            return true;
        }

        return m_selectedDirectories.find(plugin.sourceDirectory) != m_selectedDirectories.end();
    }

    AvailablePluginInfo* PluginView::FindAvailablePlugin(const std::string& pluginName) {
        auto it = std::find_if(m_availablePlugins.begin(), m_availablePlugins.end(),
            [&](const AvailablePluginInfo& p) { return p.name == pluginName; });

        return (it != m_availablePlugins.end()) ? &(*it) : nullptr;
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
            return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        }
        else if (plugin.enabled) {
            return ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
        }
        else {
            return ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
        }
    }

} // namespace GUI