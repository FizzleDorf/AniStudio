// PluginView.hpp
#pragma once
#include "Base/BaseView.hpp"
#include "PluginManager.hpp"
#include <imgui.h>

namespace GUI {

class PluginView : public BaseView {
public:
    PluginView() = default;

    void Render() override {
        ImGui::Begin("Plugin Manager");

        // Refresh button
        if (ImGui::Button("Refresh Plugins")) {
            ANI::pluginMgr.ScanPlugins("plugins"); // TODO: Make path configurable
        }

        // Plugin list table
        if (ImGui::BeginTable("Plugins", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Plugin Name");
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            for (const auto &[name, info] : ANI::pluginMgr.GetPlugins()) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", info.path.c_str());
                }

                ImGui::TableNextColumn();
                if (info.isActive) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Inactive");
                }

                ImGui::TableNextColumn();
                if (!info.isActive) {
                    if (ImGui::Button(("Load##" + name).c_str())) {
                        ANI::pluginMgr.LoadPlugin(name);
                    }
                } else {
                    if (ImGui::Button(("Unload##" + name).c_str())) {
                        ANI::pluginMgr.UnloadPlugin(const_cast<ANI::PluginInfo &>(info));
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }
};

} // namespace GUI