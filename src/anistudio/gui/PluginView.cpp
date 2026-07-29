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
#include "OpenGLWrapper.hpp"
#include <stb_image.h>

namespace GUI {

    PluginView::~PluginView() {
        for (auto& plugin : m_availablePlugins) {
            if (plugin.logoTexture != 0) {
                glDeleteTextures(1, &plugin.logoTexture);
                plugin.logoTexture = 0;
            }
        }
    }

    void PluginView::Init() {
        std::filesystem::path buildPath = std::filesystem::current_path();
        std::filesystem::path pluginsPath = buildPath / "plugins";
        std::filesystem::create_directories(pluginsPath);

        m_pluginManager.setStagingDirectory(pluginsPath.string());

        RefreshAvailablePlugins();
        LoadPluginScopeConfig();

        m_pluginManager.enableHotReload(m_hotReloadEnabled);

        std::cout << "[PluginView] Initialized" << std::endl;
    }

    void PluginView::Update(float deltaT) {
        if (m_statusTimer > 0.0f) {
            m_statusTimer -= deltaT;
            if (m_statusTimer <= 0.0f) {
                m_statusMessage.clear();
            }
        }

        m_pluginManager.updatePlugins(deltaT);
    }

    void PluginView::Render() {
        if (!m_hotReloadWasEnabled && m_hotReloadEnabled) {
            m_pluginManager.enableHotReload(true);
            m_hotReloadWasEnabled = true;
        }

        ImGui::SetNextWindowSize(ImVec2(1400, 800), ImGuiCond_FirstUseEver);

        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
            RenderMainContent();
        }
        ImGui::End();

        if (!windowOpen) {
            if (m_hotReloadWasEnabled) {
                m_pluginManager.enableHotReload(false);
                m_hotReloadWasEnabled = false;
            }
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void PluginView::RenderMainContent() {
        ImGui::BeginChild("Toolbar", ImVec2(0, 30), false);
        {
            ImGui::Checkbox("Hot Reload", &m_hotReloadEnabled);
            ImGui::SameLine();
            if (ImGui::Button("Refresh Plugins")) {
                RefreshAvailablePlugins();
                LoadPluginScopeConfig();
                m_selectedPluginForDetails.clear();
            }
            ImGui::SameLine();
            ImGui::Text("| Available: %zu", m_availablePlugins.size());
            int activeCount = std::count_if(m_availablePlugins.begin(), m_availablePlugins.end(),
                [](const AvailablePluginInfo& p) { return p.isLoaded && p.isEnabled; });
            ImGui::SameLine();
            ImGui::Text("Active: %d", activeCount);
        }
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("MainSplit");
        {
            float leftWidth = ImGui::GetContentRegionAvail().x * 0.55f;
            ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
            {
                RenderPluginLists();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
            {
                RenderSelectedPluginDetails();
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::Separator();
        RenderStatusBar();
    }

    void PluginView::RenderPluginLists() {
        std::vector<AvailablePluginInfo*> inactive, preloaded, active;
        for (auto& p : m_availablePlugins) {
            if (p.isLoaded && p.isEnabled)
                active.push_back(&p);
            else if (p.isLoaded && !p.isEnabled)
                preloaded.push_back(&p);
            else
                inactive.push_back(&p);
        }

        auto renderSection = [this](const char* label, const std::vector<AvailablePluginInfo*>& plugins, const ImVec4& color) {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            bool expanded = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopStyleColor();
            if (expanded) {
                if (plugins.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " (none)");
                }
                else {
                    for (auto* plugin : plugins) {
                        bool isSelected = (m_selectedPluginForDetails == plugin->name);
                        if (ImGui::Selectable(plugin->name.c_str(), isSelected)) {
                            m_selectedPluginForDetails = plugin->name;
                            LoadVersionList(plugin->name);
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            if (!plugin->isLoaded) {
                                if (ImGui::MenuItem("Load")) {
                                    LoadPlugin(plugin->name);
                                }
                            }
                            else if (!plugin->isEnabled) {
                                if (ImGui::MenuItem("Enable")) {
                                    EnablePlugin(plugin->name);
                                }
                                if (ImGui::MenuItem("Reload")) {
                                    ReloadPlugin(plugin->name);
                                }
                                if (ImGui::MenuItem("Unload")) {
                                    UnloadPlugin(plugin->name);
                                }
                            }
                            else {
                                if (ImGui::MenuItem("Disable")) {
                                    DisablePlugin(plugin->name);
                                }
                                if (ImGui::MenuItem("Reload")) {
                                    ReloadPlugin(plugin->name);
                                }
                                if (ImGui::MenuItem("Unload")) {
                                    UnloadPlugin(plugin->name);
                                }
                            }
                            ImGui::EndPopup();
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Name: %s", plugin->name.c_str());
                            ImGui::Text("Directory: %s", plugin->directory.string().c_str());
                            ImGui::Text("State: %s", plugin->isLoaded ? (plugin->isEnabled ? "Active" : "Preloaded") : "Inactive");
                            ImGui::EndTooltip();
                        }
                    }
                }
            }
            };

        renderSection("Inactive Plugins", inactive, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        renderSection("Preloaded Plugins (loaded, not active)", preloaded, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
        renderSection("Active Plugins", active, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
    }

    void PluginView::RenderSelectedPluginDetails() {
        if (m_selectedPluginForDetails.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a plugin to view details");
            return;
        }

        auto* availPlugin = FindAvailablePlugin(m_selectedPluginForDetails);
        if (!availPlugin) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Plugin not found");
            return;
        }

        if (availPlugin->logoTexture == 0) {
            availPlugin->logoTexture = LoadLogoTexture(availPlugin->directory.string());
        }
        if (availPlugin->logoTexture != 0) {
            ImGui::Image((ImTextureID)(intptr_t)availPlugin->logoTexture, ImVec2(128, 128));
            ImGui::Separator();
        }

        ImGui::Text("Name: %s", availPlugin->name.c_str());
        ImGui::Text("Directory: %s", availPlugin->directory.string().c_str());
        ImGui::Text("Source: %s", availPlugin->sourceDirectory.c_str());

        const char* stateText = "Inactive";
        ImVec4 stateColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        if (availPlugin->isLoaded) {
            if (availPlugin->isEnabled) {
                stateText = "Active";
                stateColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
            }
            else {
                stateText = "Preloaded";
                stateColor = ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
            }
        }
        ImGui::TextColored(stateColor, "State: %s", stateText);

        auto loadedPlugins = m_pluginManager.getLoadedPlugins();
        auto it = std::find_if(loadedPlugins.begin(), loadedPlugins.end(),
            [this](const Plugins::PluginInfo& p) { return p.name == m_selectedPluginForDetails; });
        if (it != loadedPlugins.end()) {
            const auto& plugin = *it;
            ImGui::Text("Version: %s", plugin.version.c_str());
            ImGui::Text("DLL Version: v%u", plugin.currentVersion);
            if (plugin.hotReloadPending) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Hot reload pending to v%u", plugin.nextVersion);
            }
        }

        ImGui::Separator();

        bool isProject = availPlugin->isProjectScope;
        if (ImGui::RadioButton("Global", !isProject)) {
            SetPluginScope(availPlugin->name, false);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Project", isProject)) {
            SetPluginScope(availPlugin->name, true);
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), isProject ? "(per-project)" : "(global)");

        ImGui::Separator();

        if (!availPlugin->isLoaded) {
            if (ImGui::Button("Load", ImVec2(100, 0))) {
                LoadPlugin(availPlugin->name);
            }
        }
        else {
            if (availPlugin->isEnabled) {
                if (ImGui::Button("Disable", ImVec2(100, 0))) {
                    DisablePlugin(availPlugin->name);
                }
                ImGui::SameLine();
            }
            else {
                if (ImGui::Button("Enable", ImVec2(100, 0))) {
                    EnablePlugin(availPlugin->name);
                }
                ImGui::SameLine();
            }
            if (ImGui::Button("Reload", ImVec2(100, 0))) {
                ReloadPlugin(availPlugin->name);
            }
            ImGui::SameLine();
            if (ImGui::Button("Unload", ImVec2(100, 0))) {
                UnloadPlugin(availPlugin->name);
            }
        }

        ImGui::Separator();

        ImGui::Text("Available Versions:");
        auto versionsIt = m_pluginVersions.find(availPlugin->name);
        if (versionsIt != m_pluginVersions.end() && !versionsIt->second.empty()) {
            for (uint32_t ver : versionsIt->second) {
                bool isActive = (it != loadedPlugins.end() && ver == it->currentVersion);
                if (isActive) {
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "  v%u (active)", ver);
                }
                else {
                    if (ImGui::Selectable(("v" + std::to_string(ver)).c_str(), false)) {
                        SwitchToVersion(availPlugin->name, ver);
                    }
                }
            }
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No version files found");
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
            int activeCount = std::count_if(m_availablePlugins.begin(), m_availablePlugins.end(),
                [](const AvailablePluginInfo& p) { return p.isLoaded && p.isEnabled; });
            ImGui::Text("Ready - Hot reload: %s | Available: %zu | Active: %d",
                actualHotReloadState ? "ENABLED" : "DISABLED",
                m_availablePlugins.size(),
                activeCount);
        }
    }

    void PluginView::RefreshAvailablePlugins() {
        m_availablePlugins.clear();

        std::filesystem::path buildPath = std::filesystem::current_path();
        std::filesystem::path pluginsPath = buildPath / "plugins";

        if (std::filesystem::exists(pluginsPath)) {
            DiscoverPluginsInDirectory(pluginsPath);
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
                return;
            }

            for (const auto& entry : std::filesystem::directory_iterator(searchDir)) {
                if (entry.is_directory()) {
                    std::string dirName = entry.path().filename().string();
                    if (dirName == "staging") continue;

                    auto it = std::find_if(m_availablePlugins.begin(), m_availablePlugins.end(),
                        [&](const AvailablePluginInfo& p) { return p.name == dirName; });

                    if (it == m_availablePlugins.end()) {
                        m_availablePlugins.push_back(
                            AvailablePluginInfo(dirName, entry.path(), searchDir.filename().string())
                        );
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginView] Error discovering plugins in " << searchDir << ": " << e.what() << std::endl;
        }
    }

    GLuint PluginView::LoadLogoTexture(const std::string& pluginDir) {
        std::string logoPath = (std::filesystem::path(pluginDir) / "logo.jpg").string();
        if (!std::filesystem::exists(logoPath)) {
            return 0;
        }

        int width, height, channels;
        unsigned char* data = stbi_load(logoPath.c_str(), &width, &height, &channels, 4);
        if (!data) {
            return 0;
        }

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);

        return texId;
    }

    void PluginView::LoadPlugin(const std::string& pluginName) {
        auto* availPlugin = FindAvailablePlugin(pluginName);
        if (!availPlugin) {
            ShowStatus("Plugin not found: " + pluginName, 5.0f);
            return;
        }

        if (m_pluginManager.loadPlugin(availPlugin->directory.string())) {
            availPlugin->isLoaded = true;
            ShowStatus("Loaded plugin: " + pluginName, 3.0f);

            if (availPlugin->isProjectScope) {
                m_pluginManager.SaveProjectPluginState();
            }
            else {
                m_pluginManager.SaveGlobalPluginState();
            }
        }
        else {
            ShowStatus("Failed to load plugin: " + pluginName, 5.0f);
        }
    }

    void PluginView::EnablePlugin(const std::string& pluginName) {
        if (m_pluginManager.enablePlugin(pluginName)) {
            auto* availPlugin = FindAvailablePlugin(pluginName);
            if (availPlugin) {
                availPlugin->isEnabled = true;

                if (availPlugin->isProjectScope) {
                    m_pluginManager.SaveProjectPluginState();
                }
                else {
                    m_pluginManager.SaveGlobalPluginState();
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
                }
                else {
                    m_pluginManager.SaveGlobalPluginState();
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
        }
    }

    void PluginView::SwitchToVersion(const std::string& pluginName, uint32_t version) {
        ShowStatus("Switching to version v" + std::to_string(version) + " not fully implemented", 3.0f);
    }

    void PluginView::LoadVersionList(const std::string& pluginName) {
        m_pluginVersions[pluginName].clear();

        auto* availPlugin = FindAvailablePlugin(pluginName);
        if (!availPlugin) return;

        std::string pluginDir = availPlugin->directory.string();
        std::regex versionPattern(pluginName + "\\.v(\\d+)\\.(dll|so)");

        for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::smatch matches;
                if (std::regex_match(filename, matches, versionPattern)) {
                    uint32_t ver = std::stoul(matches[1].str());
                    m_pluginVersions[pluginName].push_back(ver);
                }
            }
        }
        std::sort(m_pluginVersions[pluginName].begin(), m_pluginVersions[pluginName].end());
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
                return;
            }

            std::ifstream file(filepath);
            if (!file.is_open()) {
                return;
            }

            nlohmann::json j;
            file >> j;

            if (j.contains("pluginScopes") && j["pluginScopes"].is_object()) {
                for (auto& plugin : m_availablePlugins) {
                    if (j["pluginScopes"].contains(plugin.name)) {
                        plugin.isProjectScope = j["pluginScopes"][plugin.name];
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginView] Failed to load plugin scope config: " << e.what() << std::endl;
        }
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