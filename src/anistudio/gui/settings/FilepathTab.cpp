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
            ImGui::Separator();
            RenderResetButton();
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
                            }
                        }
                    }
                    else {
                        if (FileDialog::SelectFolder("Select Directory", newPath, defaultPath)) {
                            if (!newPath.empty() && newPath != currentPath) {
                                m_fs.SetPath(key, newPath);
                            }
                        }
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::Button("X", ImVec2(30, 0))) {
                    m_fs.SetPath(key, "");
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

    void FilePathTab::RenderResetButton() {
        std::string modelRoot = m_fs.GetPath("ModelRoot");
        if (!modelRoot.empty()) {
            if (ImGui::Button("Reset Model Subdirectories to Root")) {
                std::map<std::string, std::string> subdirs = {
                    {"Checkpoint", "/checkpoints"},
                    {"Encoder", "/clip"},
                    {"Vae", "/vae"},
                    {"Unet", "/unet"},
                    {"Lora", "/loras"},
                    {"ControlNet", "/controlnet"},
                    {"Upscale", "/upscale_models"},
                    {"Embed", "/embeddings"}
                };
                for (const auto& [key, subdir] : subdirs) {
                    m_fs.SetPath(key, modelRoot + subdir);
                }
                std::string clipDir = modelRoot + "/clip";
                m_fs.SetPath("ClipL", clipDir + "/clip_l.safetensors");
                m_fs.SetPath("ClipG", clipDir + "/clip_g.safetensors");
                m_fs.SetPath("T5XXL", clipDir + "/t5xxl.safetensors");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(Updates all model-related paths)");
        }
    }

}