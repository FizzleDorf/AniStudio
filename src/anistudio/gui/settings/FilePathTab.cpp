#include "FilePathTab.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "FileDialogUtil.hpp"
#include <imgui.h>
#include <algorithm>
#include <map>

namespace ECS {

    FilePathTab::FilePathTab(SettingsSystem& sys) : m_settingsSystem(sys) {}

    bool FilePathTab::FilterPass(const std::string& key) const {
        if (m_filter.empty()) return true;
        std::string lower = key;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    std::string FilePathTab::GetCategoryForPath(const std::string& key) const {
        if (FilePathComponent::IsPathHidden(key)) return "";
        if (key == "DataPath" || key == "DefaultProject" || key == "Plugins") return "Core";
        if (key == "ImguiState" || key == "Templates" || key == "Shaders" || key == "StylesDir") return "Application Data";
        if (key == "Assets" || key == "Docs" || key == "Scripts") return "Assets";
        return "Other";
    }

    void FilePathTab::DetermineCategoryOrder(const std::map<std::string, std::vector<std::string>>& categoryMap,
        std::vector<std::string>& order) const {
        std::vector<std::string> coreOrder = { "Core", "Application Data", "Assets" };
        for (const auto& category : coreOrder) {
            if (categoryMap.find(category) != categoryMap.end()) order.push_back(category);
        }
        std::vector<std::string> remaining;
        for (const auto& [category, _] : categoryMap) {
            if (std::find(coreOrder.begin(), coreOrder.end(), category) == coreOrder.end())
                remaining.push_back(category);
        }
        std::sort(remaining.begin(), remaining.end());
        order.insert(order.end(), remaining.begin(), remaining.end());
    }

    bool FilePathTab::IsFileSelector(const std::string& key) const {
        std::string lower = key;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return (lower.find(".safetensors") != std::string::npos ||
            lower.find(".ckpt") != std::string::npos ||
            lower.find(".pth") != std::string::npos ||
            lower.find(".pt") != std::string::npos ||
            lower.find(".onnx") != std::string::npos ||
            lower.find(".bin") != std::string::npos ||
            lower.find(".json") != std::string::npos ||
            lower.find(".txt") != std::string::npos ||
            lower.find(".ini") != std::string::npos);
    }

    void FilePathTab::Render() {
        if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
            ImGui::Text("Filter paths");
            ImGui::SameLine();
            ImGui::InputText("##PathFilter", m_pathFilter, sizeof(m_pathFilter));
            m_filter = m_pathFilter;
            ImGui::SameLine();
            if (ImGui::Button("Default All")) {
                auto* fc = m_settingsSystem.GetFilePathComponent();
                if (fc) {
                    for (const auto& [key, value] : fc->GetDefaultMap()) {
                        m_settingsSystem.SetPath(key, value);
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear All")) {
                auto keys = m_settingsSystem.GetAllKeys();
                for (const auto& key : keys) {
                    if (FilePathComponent::IsPathHidden(key)) continue;
                    m_settingsSystem.SetPath(key, "");
                }
            }
            ImGui::Separator();
            RenderPathsTable();
        }
        ImGui::EndChild();
    }

    void FilePathTab::RenderPathsTable() {
        std::vector<std::string> allKeys = m_settingsSystem.GetAllKeys();
        if (allKeys.empty()) {
            ImGui::TextDisabled("No paths registered.");
            return;
        }

        std::map<std::string, std::vector<std::string>> categoryMap;
        for (const auto& key : allKeys) {
            if (!FilterPass(key)) continue;
            std::string cat = GetCategoryForPath(key);
            if (cat.empty()) continue;
            categoryMap[cat].push_back(key);
        }

        if (categoryMap.empty()) {
            ImGui::TextDisabled("No paths match filter.");
            return;
        }

        std::vector<std::string> categoryOrder;
        DetermineCategoryOrder(categoryMap, categoryOrder);

        auto* fc = m_settingsSystem.GetFilePathComponent();

        for (const auto& categoryName : categoryOrder) {
            auto it = categoryMap.find(categoryName);
            if (it == categoryMap.end()) continue;
            auto& keys = it->second;
            std::sort(keys.begin(), keys.end());

            if (!ImGui::CollapsingHeader(categoryName.c_str())) continue;

            if (ImGui::BeginTable(("table_" + categoryName).c_str(), 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& key : keys) {
                    std::string currentPath = m_settingsSystem.GetPath(key);
                    bool isFile = IsFileSelector(key);

                    ImGui::TableNextRow();
                    ImGui::PushID(key.c_str());

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(key.c_str());

                    ImGui::TableNextColumn();
                    if (ImGui::Button("...", ImVec2(35, 0))) {
                        std::string startPath = currentPath.empty() ? "." : currentPath;
                        std::string newPath;
                        if (isFile) {
                            std::string filter = "All Files{.*},SafeTensors{.safetensors},Checkpoint{.ckpt},PyTorch{.pth}";
                            if (FileDialog::OpenFile("Select File", filter, newPath, startPath)) {
                                if (!newPath.empty() && newPath != currentPath)
                                    m_settingsSystem.SetPath(key, newPath);
                            }
                        }
                        else {
                            if (FileDialog::SelectFolder("Select Directory", newPath, startPath)) {
                                if (!newPath.empty() && newPath != currentPath)
                                    m_settingsSystem.SetPath(key, newPath);
                            }
                        }
                    }

                    ImGui::TableNextColumn();
                    std::string defaultPath = fc ? fc->GetDefaultPath(key) : "";
                    if (!defaultPath.empty()) {
                        if (ImGui::Button("Default", ImVec2(55, 0))) {
                            m_settingsSystem.SetPath(key, defaultPath);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Set to default: %s", defaultPath.c_str());
                        }
                    }
                    else {
                        ImGui::TextDisabled("-");
                    }

                    ImGui::TableNextColumn();
                    if (ImGui::Button("X", ImVec2(25, 0))) {
                        m_settingsSystem.SetPath(key, "");
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Clear this path");
                    }

                    ImGui::TableNextColumn();
                    if (currentPath.empty()) {
                        ImGui::TextDisabled("(not set)");
                    }
                    else {
                        std::string displayPath = currentPath;
                        if (displayPath.length() > 60) {
                            displayPath = "..." + displayPath.substr(displayPath.length() - 57);
                        }
                        ImGui::Text("%s", displayPath.c_str());
                        if (ImGui::IsItemHovered() && currentPath.length() > 60) {
                            ImGui::SetTooltip("%s", currentPath.c_str());
                        }
                    }

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }

}