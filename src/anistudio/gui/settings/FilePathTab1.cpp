#include "FilePathTab.hpp"
#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include "FileDialogUtil.hpp"
#include "MissingPathsPopup.hpp"

namespace ECS {

    std::unordered_map<std::string, std::string> FilePathTab::s_defaultPaths;
    std::unordered_map<std::string, std::string> FilePathTab::s_modelRootDependentPaths;
    std::vector<FilePathTab::CategoryMapper> FilePathTab::s_categoryMappers;
    std::string FilePathTab::s_currentModelRoot;

    FilePathTab::FilePathTab(FilePathSystem& fs) : m_fs(fs) {
        InitializeDefaults();
    }

    void FilePathTab::RegisterDefaultPath(const std::string& key, const std::string& defaultPath) {
        s_defaultPaths[key] = defaultPath;
        Utils::SetDefaultPath(key, defaultPath);
    }

    void FilePathTab::RegisterModelRootDependentPath(const std::string& key, const std::string& subdirectory) {
        s_modelRootDependentPaths[key] = subdirectory;
        if (!s_currentModelRoot.empty()) {
            std::string defaultPath = (std::filesystem::path(s_currentModelRoot) / subdirectory).string();
            s_defaultPaths[key] = defaultPath;
            Utils::SetDefaultPath(key, defaultPath);
        }
    }

    void FilePathTab::UpdateModelRootDefaults(const std::string& modelRoot) {
        s_currentModelRoot = modelRoot;
        for (const auto& [key, subdir] : s_modelRootDependentPaths) {
            std::string defaultPath = (std::filesystem::path(modelRoot) / subdir).string();
            s_defaultPaths[key] = defaultPath;
            Utils::SetDefaultPath(key, defaultPath);
        }
    }

    void FilePathTab::RegisterCategoryMapper(CategoryMapper mapper) {
        s_categoryMappers.push_back(mapper);
    }

    void FilePathTab::InitializeDefaults() {
        s_defaultPaths = {
            {"DataPath", "./data"},
            {"DefaultProject", "./projects"},
            {"Plugins", "./plugins"},
            {"ImguiState", "./data/imgui.ini"},
            {"Assets", "./assets"},
            {"Docs", "./docs"},
            {"Scripts", "./scripts"},
            {"Templates", "./data/templates"},
            {"Shaders", "./shaders"}
        };

        for (const auto& [key, value] : s_defaultPaths) {
            Utils::SetDefaultPath(key, value);
        }
    }

    std::string FilePathTab::GetDefaultPath(const std::string& key) const {
        auto it = s_defaultPaths.find(key);
        if (it != s_defaultPaths.end()) {
            return it->second;
        }
        return "";
    }

    bool FilePathTab::IsPathHidden(const std::string& key) const {
        return (key == "CurrentProject" || key == "ProjectData" ||
            key == "ProjectAssets" || key == "ProjectOutput" ||
            key == "Output" || key == "LastOpenProject" || key == "ProjectDataPath" ||
            key == "AssetsFolder" || key == "OutputFolder");
    }

    std::string FilePathTab::GetCategoryForPath(const std::string& key) const {
        if (IsPathHidden(key)) {
            return "";
        }

        for (const auto& mapper : s_categoryMappers) {
            std::string category = mapper(key);
            if (!category.empty()) {
                return category;
            }
        }

        if (key == "DataPath" || key == "DefaultProject" || key == "Plugins") {
            return "Core";
        }

        if (key == "ImguiState" || key == "Templates" || key == "Shaders") {
            return "Application Data";
        }

        if (key == "Assets" || key == "Docs" || key == "Scripts") {
            return "Assets";
        }

        return "Other";
    }

    void FilePathTab::DetermineCategoryOrder(const std::map<std::string, std::vector<std::string>>& categoryMap,
        std::vector<std::string>& order) const {
        std::vector<std::string> coreOrder = {
            "Core",
            "Application Data",
            "Assets"
        };

        for (const auto& category : coreOrder) {
            if (categoryMap.find(category) != categoryMap.end()) {
                order.push_back(category);
            }
        }

        std::vector<std::string> remaining;
        for (const auto& [category, _] : categoryMap) {
            if (std::find(coreOrder.begin(), coreOrder.end(), category) == coreOrder.end()) {
                remaining.push_back(category);
            }
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
            lower.find(".txt") != std::string::npos);
    }

    bool FilePathTab::FilterPass(const std::string& key) const {
        if (m_filter.empty()) return true;
        std::string lower = key;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void FilePathTab::Render() {
        if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
            ImGui::Text("Filter paths");
            ImGui::SameLine();
            ImGui::InputText("##PathFilter", m_pathFilter, sizeof(m_pathFilter));
            m_filter = m_pathFilter;
            ImGui::SameLine();
            RenderActionButtons();
            ImGui::Separator();
            RenderPathsTable();
        }
        ImGui::EndChild();
    }

    void FilePathTab::RenderActionButtons() {
        if (ImGui::Button("Default All")) {
            for (const auto& [key, value] : s_defaultPaths) {
                m_fs.SetPath(key, value);
            }
            m_hasChanges = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All")) {
            auto keys = m_fs.GetAllKeys();
            for (const auto& key : keys) {
                if (IsPathHidden(key)) continue;
                m_fs.SetPath(key, "");
            }
            m_hasChanges = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            SaveSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save and Close")) {
            SaveSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            if (m_hasChanges) {
                ImGui::OpenPopup("UnsavedChangesPopup");
            }
        }

        if (ImGui::BeginPopupModal("UnsavedChangesPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. Close anyway?");
            if (ImGui::Button("Yes")) {
                RestoreFromBackup();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void FilePathTab::RenderPathsTable() {
        std::vector<std::string> allKeys = m_fs.GetAllKeys();
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

        for (const auto& categoryName : categoryOrder) {
            auto it = categoryMap.find(categoryName);
            if (it == categoryMap.end()) continue;

            auto& keys = it->second;
            std::sort(keys.begin(), keys.end());

            bool open = ImGui::CollapsingHeader(categoryName.c_str());
            if (!open) continue;

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
                    std::string currentPath = m_fs.GetPath(key);
                    bool isFile = IsFileSelector(key);

                    ImGui::TableNextRow();
                    ImGui::PushID(key.c_str());

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(key.c_str());

                    ImGui::TableNextColumn();
                    if (ImGui::Button("...", ImVec2(35, 0))) {
                        std::string defaultPath = currentPath.empty() ? "." : currentPath;
                        std::string newPath;
                        if (isFile) {
                            std::string filter = "All Files{.*},SafeTensors{.safetensors},Checkpoint{.ckpt},PyTorch{.pth}";
                            if (FileDialog::OpenFile("Select File", filter, newPath, defaultPath)) {
                                if (!newPath.empty() && newPath != currentPath) {
                                    m_fs.SetPath(key, newPath);
                                    m_hasChanges = true;
                                    if (key == "ModelRoot") {
                                        UpdateModelRootDefaults(newPath);
                                    }
                                }
                            }
                        }
                        else {
                            if (FileDialog::SelectFolder("Select Directory", newPath, defaultPath)) {
                                if (!newPath.empty() && newPath != currentPath) {
                                    m_fs.SetPath(key, newPath);
                                    m_hasChanges = true;
                                    if (key == "ModelRoot") {
                                        UpdateModelRootDefaults(newPath);
                                    }
                                }
                            }
                        }
                    }

                    ImGui::TableNextColumn();
                    std::string defaultPath = GetDefaultPath(key);
                    if (!defaultPath.empty()) {
                        if (ImGui::Button("Default", ImVec2(55, 0))) {
                            m_fs.SetPath(key, defaultPath);
                            m_hasChanges = true;
                            if (key == "ModelRoot") {
                                UpdateModelRootDefaults(defaultPath);
                            }
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
                        m_fs.SetPath(key, "");
                        m_hasChanges = true;
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

    void FilePathTab::CreateBackup() {
        m_backupPaths.clear();
        std::vector<std::string> keys = m_fs.GetAllKeys();
        for (const auto& key : keys) {
            if (IsPathHidden(key)) continue;
            m_backupPaths[key] = m_fs.GetPath(key);
        }
    }

    void FilePathTab::RestoreFromBackup() {
        for (const auto& [key, path] : m_backupPaths) {
            m_fs.SetPath(key, path);
        }
        m_hasChanges = false;
    }

    void FilePathTab::ResetToDefaults() {
        for (const auto& [key, value] : s_defaultPaths) {
            m_fs.SetPath(key, value);
        }
        if (!s_currentModelRoot.empty()) {
            UpdateModelRootDefaults(s_currentModelRoot);
        }
        m_hasChanges = true;
    }

    bool FilePathTab::SaveSettings() {
        std::string dataPath = m_fs.GetPath("DataPath");
        if (dataPath.empty()) {
            dataPath = "./data";
        }
        std::string filePath = dataPath + "/paths.json";
        m_fs.SaveToFile(filePath);
        m_hasChanges = false;
        CreateBackup();
        Utils::CheckMissingPaths(&m_fs);
        return true;
    }

    bool FilePathTab::LoadSettings() {
        std::string dataPath = m_fs.GetPath("DataPath");
        if (dataPath.empty()) {
            dataPath = "./data";
        }
        std::string filePath = dataPath + "/paths.json";
        m_fs.LoadFromFile(filePath);

        std::string modelRoot = m_fs.GetPath("ModelRoot");
        if (!modelRoot.empty()) {
            UpdateModelRootDefaults(modelRoot);
        }

        m_hasChanges = false;
        CreateBackup();
        return true;
    }

}