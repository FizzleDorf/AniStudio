#include "MissingPathsPopup.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include <imgui.h>
#include <unordered_map>
#include <algorithm>

namespace Utils {

    static std::shared_ptr<ECS::FilePathSystem> s_fileSys;
    static std::vector<std::string> s_missingKeys;
    static bool s_popupOpen = false;
    static std::unordered_map<std::string, std::string> s_tempPaths;
    static std::unordered_map<std::string, std::string> s_defaultPaths;

    void SetDefaultPath(const std::string& key, const std::string& defaultPath) {
        s_defaultPaths[key] = defaultPath;
    }

    static std::string GetDefaultPathForKey(const std::string& key) {
        auto it = s_defaultPaths.find(key);
        return (it != s_defaultPaths.end()) ? it->second : "";
    }

    void CheckMissingPaths(std::shared_ptr<ECS::FilePathSystem> fileSys) {
        if (!fileSys) return;
        s_fileSys = fileSys;
        s_missingKeys.clear();
        auto allKeys = fileSys->GetAllKeys();
        for (const auto& key : allKeys) {
            if (fileSys->GetPath(key).empty()) {
                s_missingKeys.push_back(key);
            }
        }
        if (s_missingKeys.empty()) {
            s_popupOpen = false;
            s_tempPaths.clear();
        }
        else {
            s_popupOpen = true;
            s_tempPaths.clear();
            for (const auto& key : s_missingKeys) {
                s_tempPaths[key] = "";
            }
        }
    }

    void RenderMissingPathsPopup() {
        if (!s_fileSys) return;
        if (!s_popupOpen) return;

        ImGui::OpenPopup("Missing File Paths");
        if (ImGui::BeginPopupModal("Missing File Paths", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("The following file paths are not set:");
            ImGui::Separator();

            for (auto& key : s_missingKeys) {
                ImGui::PushID(key.c_str());
                ImGui::Text("%s", key.c_str());
                ImGui::SameLine(150.0f);
                char buffer[256];
                strcpy(buffer, s_tempPaths[key].c_str());
                if (ImGui::InputText("##path", buffer, sizeof(buffer))) {
                    s_tempPaths[key] = buffer;
                }
                ImGui::SameLine();
                if (ImGui::Button("Browse")) {
                    std::string selected;
                    if (FileDialog::SelectFolder("Select Folder", selected)) {
                        s_tempPaths[key] = selected;
                    }
                }
                ImGui::SameLine();
                std::string defaultPath = GetDefaultPathForKey(key);
                if (!defaultPath.empty()) {
                    if (ImGui::Button("Use Default")) {
                        s_tempPaths[key] = defaultPath;
                    }
                }
                ImGui::PopID();
            }

            ImGui::Separator();
            if (ImGui::Button("Save")) {
                for (const auto& pair : s_tempPaths) {
                    if (!pair.second.empty()) {
                        s_fileSys->SetPath(pair.first, pair.second);
                    }
                }
                std::string dataPath = s_fileSys->GetPath("DataPath");
                if (!dataPath.empty()) {
                    std::string filepathsFile = dataPath + "/filepaths.json";
                    s_fileSys->SaveToFile(filepathsFile);
                }
                s_popupOpen = false;
                s_tempPaths.clear();
                s_missingKeys.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                s_popupOpen = false;
                s_tempPaths.clear();
                s_missingKeys.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        else {
            s_popupOpen = false;
        }
    }
}