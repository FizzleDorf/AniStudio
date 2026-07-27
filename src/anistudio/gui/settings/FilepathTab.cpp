#include "FilePathTab.hpp"
#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include "FileDialogUtil.hpp"

namespace ECS {

    FilePathTab::FilePathTab(FilePathSystem& fs) : m_fs(fs) {}

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
            ImGui::Separator();
            RenderPathsTable();
        }
        ImGui::EndChild();
    }

    void FilePathTab::RenderPathsTable() {
        std::vector<std::string> allKeys = m_fs.GetAllKeys();
        if (allKeys.empty()) {
            ImGui::TextDisabled("No paths registered.");
            return;
        }

        if (ImGui::BeginTable("PathsTable", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& key : allKeys) {
                if (!FilterPass(key)) continue;

                std::string currentPath = m_fs.GetPath(key);
                bool isFileSelector = (key == "ClipL" || key == "ClipG" || key == "T5XXL");

                ImGui::TableNextRow();
                ImGui::PushID(key.c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(key.c_str());

                ImGui::TableNextColumn();
                if (ImGui::Button("...", ImVec2(30, 0))) {
                    std::string defaultPath = currentPath.empty() ? "." : currentPath;
                    std::string newPath;
                    if (isFileSelector) {
                        std::string filter = "All Files{.*},SafeTensors{.safetensors},Checkpoint{.ckpt},PyTorch{.pth}";
                        if (FileDialog::OpenFile("Select File", filter, newPath, defaultPath)) {
                            if (!newPath.empty() && newPath != currentPath) {
                                m_fs.SetPath(key, newPath);
                                m_hasChanges = true;
                            }
                        }
                    }
                    else {
                        if (FileDialog::SelectFolder("Select Directory", newPath, defaultPath)) {
                            if (!newPath.empty() && newPath != currentPath) {
                                m_fs.SetPath(key, newPath);
                                m_hasChanges = true;
                            }
                        }
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::Button("X", ImVec2(30, 0))) {
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

    void FilePathTab::CreateBackup() {
        m_backupPaths.clear();
        auto keys = m_fs.GetAllKeys();
        for (const auto& key : keys) {
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
        std::map<std::string, std::string> defaults = {
            {"ModelRoot", "./models"},
            {"Checkpoint", "./models/checkpoints"},
            {"Encoder", "./models/clip"},
            {"Vae", "./models/vae"},
            {"Unet", "./models/unet"},
            {"Lora", "./models/loras"},
            {"ControlNet", "./models/controlnet"},
            {"Upscale", "./models/upscale_models"},
            {"Embed", "./models/embeddings"},
            {"ClipL", "./models/clip/clip_l.safetensors"},
            {"ClipG", "./models/clip/clip_g.safetensors"},
            {"T5XXL", "./models/clip/t5xxl.safetensors"}
        };
        for (const auto& [key, value] : defaults) {
            m_fs.SetPath(key, value);
        }
        m_hasChanges = true;
    }

    bool FilePathTab::SaveSettings() {
        m_fs.SaveToFile(m_settingsFilePath);
        m_hasChanges = false;
        CreateBackup();
        return true;
    }

    bool FilePathTab::LoadSettings() {
        m_fs.LoadFromFile(m_settingsFilePath);
        m_hasChanges = false;
        CreateBackup();
        return true;
    }

}