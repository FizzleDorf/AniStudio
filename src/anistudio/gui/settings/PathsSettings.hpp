#pragma once

#include "BaseTabObject.hpp"
#include "ImGuiFileDialog.h"
#include "FilePathSystem.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <vector>
#include <memory>

namespace Settings {

    class PathsSettingsTab : public BaseTabObject {
    public:
        PathsSettingsTab(ECS::EntityManager& entityMgr)
            : BaseTabObject("Paths", "Application"), m_entityManager(entityMgr) {
            InitializePathCategories();
            LoadCurrentPaths();
            CreateBackup();
        }

        void RenderUI() override {
            if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
                for (const auto& category : pathCategories) {
                    RenderCategory(category.first, category.second);
                }

                ImGui::Separator();

                auto modelRootIt = pathMap.find("ModelRoot");
                if (modelRootIt != pathMap.end() && !modelRootIt->second.empty()) {
                    if (ImGui::Button("Reset Model Paths to Root")) {
                        ResetModelPathsToRoot(modelRootIt->second);
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(Will update all model subdirectories)");
                }

                ImGui::Separator();
                RenderActionButtons();
            }
            ImGui::EndChild();
        }

        void RenderFilteredUI(const std::set<std::string>& selectedCategories) override {
            if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
                for (const auto& category : pathCategories) {
                    if (ShouldRenderCategory(category.first, selectedCategories)) {
                        RenderCategory(category.first, category.second);
                    }
                }

                auto modelRootIt = pathMap.find("ModelRoot");
                if (modelRootIt != pathMap.end() && !modelRootIt->second.empty()) {
                    if (ImGui::Button("Reset Model Paths to Root")) {
                        ResetModelPathsToRoot(modelRootIt->second);
                    }
                }

                RenderActionButtons();
            }
            ImGui::EndChild();
        }

        bool SaveSettings() override {
            try {
                auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                if (!fileSys) {
                    std::cerr << "[PathsSettings] FilePathSystem not available!" << std::endl;
                    return false;
                }

                for (const auto& [key, value] : pathMap) {
                    if (key == "LastOpenProject") continue;
                    fileSys->SetPath(key, value);
                }

                auto modelRootIt = pathMap.find("ModelRoot");
                if (modelRootIt != pathMap.end() && !modelRootIt->second.empty()) {
                    // Apply model root subdirectories
                    UpdateModelSubdirectories(modelRootIt->second);
                }

                hasChanges = false;
                CreateBackup();
                return true;
            }
            catch (const std::exception& e) {
                std::cerr << "[PathsSettings] Error saving settings: " << e.what() << std::endl;
                return false;
            }
        }

        bool LoadSettings() override {
            try {
                LoadCurrentPaths();
                hasChanges = false;
                CreateBackup();
                return true;
            }
            catch (const std::exception& e) {
                std::cerr << "[PathsSettings] Error loading settings: " << e.what() << std::endl;
                return false;
            }
        }

        void ResetToDefaults() override {
            for (auto& [key, value] : pathMap) {
                if (key != "LastOpenProject") {
                    value.clear();
                }
            }
            hasChanges = true;
        }

        void CreateBackup() override {
            backupPathMap = pathMap;
        }

        void RestoreFromBackup() override {
            pathMap = backupPathMap;
            hasChanges = false;
        }

        bool HasUnsavedChanges() const override {
            return hasChanges;
        }

    private:
        ECS::EntityManager& m_entityManager;
        std::map<std::string, std::string> pathMap;
        std::map<std::string, std::string> backupPathMap;
        std::map<std::string, std::vector<std::string>> pathCategories;
        bool hasChanges = false;

        void InitializePathCategories() {
            pathCategories["General Paths"] = {
                "DefaultProject",
                "AssetsFolder",
                "OutputFolder",
                "VirtualEnv",
                "Scripts",
                "Plugins",
                "ImguiState"
            };

            pathCategories["Model Paths"] = {
                "ModelRoot",
                "Checkpoint",
                "Encoder",
                "Vae",
                "Unet",
                "Lora",
                "ControlNet",
                "Upscale",
                "Embed"
            };

            pathCategories["CLIP Files"] = {
                "ClipL",
                "ClipG",
                "T5XXL"
            };
        }

        void LoadCurrentPaths() {
            pathMap.clear();

            auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!fileSys) return;

            for (const auto& [category, keys] : pathCategories) {
                for (const auto& key : keys) {
                    pathMap[key] = fileSys->GetPath(key);
                }
            }

            std::string lastProject = fileSys->GetPath("LastOpenProject");
            if (!lastProject.empty()) {
                pathMap["LastOpenProject"] = lastProject;
            }
        }

        void RenderCategory(const std::string& categoryName, const std::vector<std::string>& pathKeys) {
            ImGui::Text("%s", categoryName.c_str());
            ImGui::Spacing();

            if (ImGui::BeginTable((categoryName + "Table").c_str(), 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {

                ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& key : pathKeys) {
                    auto it = pathMap.find(key);
                    if (it != pathMap.end()) {
                        RenderPathRow(key.c_str(), it->second, key);
                    }
                }

                ImGui::EndTable();
            }
            ImGui::Separator();
        }

        void RenderPathRow(const char* label, std::string& path, const std::string& pathKey) {
            if (pathKey == "LastOpenProject") {
                return;
            }

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            std::string displayName = FormatDisplayName(label);
            ImGui::TextUnformatted(displayName.c_str());

            ImGui::TableNextColumn();
            std::string buttonID = std::string("...##") + label;

            bool isFileSelector = (pathKey == "ClipL" || pathKey == "ClipG" || pathKey == "T5XXL");

            if (ImGui::Button(buttonID.c_str())) {
                IGFD::FileDialogConfig config;
                config.path = path.empty() ? "." : path;
                config.flags = ImGuiFileDialogFlags_Modal;
                std::string dialogID = std::string("ChoosePath##") + label;

                if (isFileSelector) {
                    ImGuiFileDialog::Instance()->OpenDialog(dialogID.c_str(),
                        "Select File",
                        "All Files{.*},.safetensors{.safetensors},.ckpt{.ckpt},.pth{.pth}",
                        config);
                }
                else {
                    ImGuiFileDialog::Instance()->OpenDialog(dialogID.c_str(),
                        "Select Directory",
                        nullptr,
                        config);
                }
            }

            std::string dialogID = std::string("ChoosePath##") + label;
            if (ImGuiFileDialog::Instance()->Display(dialogID.c_str(), 32, ImVec2(500, 400))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string selectedPath;
                    if (isFileSelector) {
                        selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
                    }
                    else {
                        selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    }

                    if (!selectedPath.empty() && selectedPath != path) {
                        path = selectedPath;
                        hasChanges = true;

                        if (pathKey == "ModelRoot" && !path.empty()) {
                            UpdateModelSubdirectories(path);
                        }
                    }
                }
                ImGuiFileDialog::Instance()->Close();
            }

            ImGui::SameLine();
            std::string resetID = std::string("R##") + label;
            if (ImGui::Button(resetID.c_str())) {
                if (!path.empty()) {
                    path.clear();
                    hasChanges = true;
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Clear this path");
            }

            ImGui::TableNextColumn();
            if (path.empty()) {
                ImGui::TextDisabled("(not set)");
            }
            else {
                std::string displayPath = path;
                if (displayPath.length() > 60) {
                    displayPath = "..." + displayPath.substr(displayPath.length() - 57);
                }
                ImGui::Text("%s", displayPath.c_str());
                if (ImGui::IsItemHovered() && path.length() > 60) {
                    ImGui::SetTooltip("%s", path.c_str());
                }
            }
        }

        void ResetModelPathsToRoot(const std::string& modelRoot) {
            if (modelRoot.empty()) return;
            UpdateModelSubdirectories(modelRoot);
            hasChanges = true;
        }

        void UpdateModelSubdirectories(const std::string& modelRoot) {
            std::map<std::string, std::string> modelSubdirs = {
                {"Checkpoint", "/checkpoints"},
                {"Encoder", "/clip"},
                {"Vae", "/vae"},
                {"Unet", "/unet"},
                {"Lora", "/loras"},
                {"ControlNet", "/controlnet"},
                {"Upscale", "/upscale_models"},
                {"Embed", "/embeddings"}
            };

            for (const auto& [key, subdir] : modelSubdirs) {
                pathMap[key] = modelRoot + subdir;
            }

            std::string encoderDir = modelRoot + "/clip";
            pathMap["ClipL"] = encoderDir + "/clip_l.safetensors";
            pathMap["ClipG"] = encoderDir + "/clip_g.safetensors";
            pathMap["T5XXL"] = encoderDir + "/t5xxl.safetensors";
        }

        std::string FormatDisplayName(const std::string& key) {
            std::string result;
            bool lastWasLower = false;

            for (char c : key) {
                if (isupper(c) && lastWasLower) {
                    result += ' ';
                    result += c;
                }
                else if (c == '_' || c == '-') {
                    result += ' ';
                }
                else {
                    result += c;
                }
                lastWasLower = islower(c);
            }

            if (!result.empty()) {
                result[0] = toupper(result[0]);
            }

            return result;
        }

        void RenderActionButtons() {
            if (hasChanges) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
                ImGui::SameLine();
            }

            if (ImGui::Button("Save Settings")) {
                SaveSettings();
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset to Defaults")) {
                ResetToDefaults();
            }

            ImGui::SameLine();

            if (hasChanges) {
                if (ImGui::Button("Revert Changes")) {
                    RestoreFromBackup();
                }
            }
        }
    };

} // namespace Settings