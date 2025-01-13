#pragma once
#include "Base/BaseView.hpp"
#include "PluginManager.hpp"
#include <imgui.h>

using namespace Plugin;

namespace GUI {

class PluginView : public BaseView {
public:
    PluginView(ECS::EntityManager &entityMgr, Plugin::PluginManager &pluginMgr)
        : BaseView(entityMgr), pluginManager(pluginMgr) {
        viewName = "Plugin View";
    }

    void Render() override {
        ImGui::Begin("Plugin Manager");

        // Refresh button and configurable directory
        if (ImGui::Button("Refresh Plugins")) {
            pluginManager.ScanPlugins();
        }

        // Plugin list table
        if (ImGui::BeginTable("Plugins", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Plugin Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            const auto &plugins = pluginManager.GetPlugins();
            for (const auto &[name, loader] : plugins) {
                ImGui::TableNextRow();

                // Plugin name
                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());

                // Version
                ImGui::TableNextColumn();
                if (auto *plugin = loader.Get()) {
                    auto version = plugin->GetVersion();
                    ImGui::Text("%d.%d.%d", version.major, version.minor, version.patch);
                } else {
                    ImGui::Text("-");
                }

                // Status
                ImGui::TableNextColumn();
                if (auto *plugin = loader.Get()) {
                    switch (plugin->GetState()) {
                    case Plugin::PluginState::Created:
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Created");
                        break;
                    case Plugin::PluginState::Loaded:
                        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Loaded");
                        break;
                    case Plugin::PluginState::Started:
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Running");
                        break;
                    case Plugin::PluginState::Stopped:
                        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Stopped");
                        break;
                    case Plugin::PluginState::Unloaded:
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Unloaded");
                        break;
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Not Loaded");
                }

                // Actions
                ImGui::TableNextColumn();
                try {
                    RenderPluginActions(name, loader);
                } catch (const PluginError &e) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
                }
            }

            ImGui::EndTable();
        }

        if (ImGui::BeginChild("Plugin Details", ImVec2(0, 200), true)) {
            if (selectedPlugin) {
                RenderPluginDetails(*selectedPlugin);
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a plugin to view details");
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

private:
    void RenderPluginActions(const std::string &name, const PluginLoader &loader) {
        auto *plugin = loader.Get();

        if (!plugin) {
            if (ImGui::Button(("Load##" + name).c_str())) {
                pluginManager.LoadPlugin(name);
            }
            return;
        }

        switch (plugin->GetState()) {
        case PluginState::Loaded:
            if (ImGui::Button(("Start##" + name).c_str())) {
                pluginManager.StartPlugin(name);
            }
            ImGui::SameLine();
            if (ImGui::Button(("Unload##" + name).c_str())) {
                pluginManager.UnloadPlugin(name);
            }
            break;

        case PluginState::Started:
            if (ImGui::Button(("Stop##" + name).c_str())) {
                pluginManager.StopPlugin(name);
            }
            break;

        case PluginState::Stopped:
            if (ImGui::Button(("Start##" + name).c_str())) {
                pluginManager.StartPlugin(name);
            }
            ImGui::SameLine();
            if (ImGui::Button(("Unload##" + name).c_str())) {
                pluginManager.UnloadPlugin(name);
            }
            break;

        default:
            break;
        }

        // Show plugin details button
        ImGui::SameLine();
        if (ImGui::Button(("Details##" + name).c_str())) {
            selectedPlugin = plugin;
        }
    }

    void RenderPluginDetails(const IPlugin &plugin) {
        ImGui::Text("Plugin: %s", plugin.GetName());

        auto version = plugin.GetVersion();
        ImGui::Text("Version: %d.%d.%d", version.major, version.minor, version.patch);

        ImGui::Text("State: %s", GetPluginStateString(plugin.GetState()));

        if (ImGui::CollapsingHeader("Dependencies")) {
            auto deps = plugin.GetDependencies();
            if (deps.empty()) {
                ImGui::Text("No dependencies");
            } else {
                for (const auto &dep : deps) {
                    ImGui::BulletText("%s", dep.c_str());
                }
            }
        }
    }

    const char *GetPluginStateString(PluginState state) {
        switch (state) {
        case Plugin::PluginState::Created:
            return "Created";
        case Plugin::PluginState::Loaded:
            return "Loaded";
        case Plugin::PluginState::Started:
            return "Running";
        case Plugin::PluginState::Stopped:
            return "Stopped";
        case Plugin::PluginState::Unloaded:
            return "Unloaded";
        default:
            return "Unknown";
        }
    }

private:
    const IPlugin *selectedPlugin = nullptr;
    Plugin::PluginManager &pluginManager;
};

} // namespace GUI