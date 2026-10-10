#include "MissingPathsPopup.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "FileDialogUtil.hpp"
#include <imgui.h>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace Utils {

    static ECS::SettingsSystem* s_settings = nullptr;
    static std::vector<std::string> s_missingKeys;
    static bool s_popupOpen = false;
    static std::unordered_map<std::string, std::string> s_tempPaths;

    void CheckMissingPaths(ECS::SettingsSystem* settingsSystem) {
        if (!settingsSystem) return;
        s_settings = settingsSystem;
        s_missingKeys.clear();

        auto* fc = settingsSystem->GetFilePathComponent();
        if (!fc) return;

        for (const auto& key : fc->GetAllKeys()) {
            if (ECS::FilePathComponent::IsPathHidden(key)) continue;
            if (fc->GetPath(key).empty()) s_missingKeys.push_back(key);
        }

        if (s_missingKeys.empty()) {
            s_popupOpen = false;
            s_tempPaths.clear();
        }
        else {
            s_popupOpen = true;
            s_tempPaths.clear();
            for (const auto& key : s_missingKeys) {
                s_tempPaths[key] = fc->GetDefaultPath(key);
            }
        }
    }

    void RenderMissingPathsPopup() {
        if (!s_settings) return;
        if (!s_popupOpen) return;

        ImGui::OpenPopup("Missing File Paths");
        if (ImGui::BeginPopupModal("Missing File Paths", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("The following file paths are not set:");
            ImGui::Separator();

            auto* fc = s_settings->GetFilePathComponent();

            for (auto& key : s_missingKeys) {
                ImGui::PushID(key.c_str());

                ImGui::Text("%s", key.c_str());
                ImGui::SameLine(150.0f);

                char buffer[512];
                std::strncpy(buffer, s_tempPaths[key].c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
                ImGui::PushItemWidth(200.0f);
                if (ImGui::InputText("##path", buffer, sizeof(buffer))) {
                    s_tempPaths[key] = buffer;
                }
                ImGui::PopItemWidth();

                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
                    std::string selected;
                    if (FileDialog::SelectFolder("Select Folder", selected)) {
                        s_tempPaths[key] = selected;
                    }
                }

                ImGui::SameLine();
                std::string defaultPath = fc ? fc->GetDefaultPath(key) : "";
                if (!defaultPath.empty()) {
                    if (ImGui::Button("Use Default")) {
                        s_tempPaths[key] = defaultPath;
                    }
                }

                ImGui::PopID();
            }

            ImGui::Separator();

            if (ImGui::Button("Save")) {
                bool allFilled = true;
                for (const auto& pair : s_tempPaths) {
                    if (pair.second.empty()) { allFilled = false; break; }
                }

                if (allFilled) {
                    for (const auto& pair : s_tempPaths) {
                        if (!pair.second.empty()) {
                            s_settings->SetPath(pair.first, pair.second);
                        }
                    }
                    s_settings->SaveAllSettings();
                    s_popupOpen = false;
                    s_tempPaths.clear();
                    s_missingKeys.clear();
                    ImGui::CloseCurrentPopup();
                }
                else {
                    ImGui::OpenPopup("MissingValuesWarning");
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Set All to Default")) {
                if (fc) {
                    for (const auto& key : s_missingKeys) {
                        std::string defaultPath = fc->GetDefaultPath(key);
                        if (!defaultPath.empty()) {
                            s_settings->SetPath(key, defaultPath);
                            s_tempPaths[key] = defaultPath;
                        }
                    }
                }
                s_settings->SaveAllSettings();
                s_popupOpen = false;
                s_tempPaths.clear();
                s_missingKeys.clear();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::BeginPopupModal("MissingValuesWarning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Some paths are still empty. Please fill all paths or use defaults.");
                if (ImGui::Button("OK")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::EndPopup();
        }
        else {
            s_popupOpen = false;
        }
    }
}