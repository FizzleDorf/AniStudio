#include "SettingsView.hpp"

#include "EntityManager.hpp"
#include "SettingsSystem.hpp"
#include "BaseSettingsComponent.hpp"
#include "FilePathSystem.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <algorithm>
#include <map>

#include "FileDialogUtil.hpp"

namespace GUI {

    SettingsView::SettingsView() {
        filterBuffer[0] = '\0';
        pathFilterBuffer[0] = '\0';
    }

    void SettingsView::SetEntityManager(ECS::EntityManager& mgr) {
        m_entityManager = &mgr;
        m_settingsSystem = mgr.GetSystem<ECS::SettingsSystem>().get();
        m_filePathSystem = mgr.GetSystem<ECS::FilePathSystem>().get();

        if (m_settingsSystem) {
            if (imguiContext) {
                m_settingsSystem->SetImGuiContext(imguiContext);
            }
            if (!settingsLoaded) {
                m_settingsSystem->LoadAllSettings();
                settingsLoaded = true;
            }
            auto comps = m_settingsSystem->GetAllSettingsComponents();
            if (!comps.empty()) {
                currentActiveTab = comps[0]->GetTabName();
            }
        }
    }

    void SettingsView::SetImGuiContext(ImGuiContext* context) {
        imguiContext = context;
        if (m_settingsSystem && imguiContext) {
            m_settingsSystem->SetImGuiContext(imguiContext);
        }
    }

    void SettingsView::Render() {
        if (!showPopup) return;
        if (!imguiContext) return;

        ImGui::SetCurrentContext(imguiContext);
        ImGui::OpenPopup("Settings##SettingsPopup");

        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_Appearing);

        bool isOpen = true;
        if (ImGui::BeginPopupModal("Settings##SettingsPopup", &isOpen,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_Modal)) {
            RenderMainContent();
            ImGui::EndPopup();
        }

        if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            HandlePopupClose();
        }

        RenderUnsavedChangesDialog();
    }

    void SettingsView::HandlePopupClose() {
        if (m_settingsSystem && m_settingsSystem->HasAnyUnsavedChanges()) {
            showUnsavedChangesDialog = true;
            pendingClose = true;
        }
        else {
            showPopup = false;
        }
    }

    void SettingsView::RenderMainContent() {
        if (!m_settingsSystem) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "SettingsSystem not available!");
            return;
        }

        ImGui::BeginChild("SettingsArea", ImVec2(0, -50), false);

        ImGui::Text("Search");
        ImGui::SameLine();
        ImGui::InputText("##SettingsFilter", filterBuffer, sizeof(filterBuffer));

        if (showSaveNotification) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Settings saved.");
            showSaveNotification = false;
        }

        ImGui::Separator();

        RenderTabsAndContent();

        ImGui::EndChild();

        ImGui::Separator();
        RenderActionButtons();
    }

    void SettingsView::RenderTabsAndContent() {
        auto comps = m_settingsSystem->GetAllSettingsComponents();

        if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
            for (auto* comp : comps) {
                if (!comp) continue;
                std::string tabTitle = comp->GetTabName() + " Settings";
                std::string tabName = comp->GetTabName();

                if (ImGui::BeginTabItem(tabTitle.c_str())) {
                    if (currentActiveTab != tabName) {
                        currentActiveTab = tabName;
                    }
                    ImGui::BeginChild("TabContent", ImVec2(0, 0), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    comp->RenderFilteredUI(std::string(filterBuffer));
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }

            if (m_filePathSystem) {
                if (ImGui::BeginTabItem("Paths Settings")) {
                    if (currentActiveTab != "Paths") {
                        currentActiveTab = "Paths";
                    }
                    ImGui::BeginChild("PathsTabContent", ImVec2(0, 0), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    RenderPathsTab();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }

    void SettingsView::RenderPathsTab() {
        if (!m_filePathSystem) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "FilePathSystem not available.");
            return;
        }

        ImGui::Text("Filter paths");
        ImGui::SameLine();
        ImGui::InputText("##PathFilter", pathFilterBuffer, sizeof(pathFilterBuffer));
        ImGui::Separator();

        std::vector<std::string> allKeys = m_filePathSystem->GetAllKeys();
        if (allKeys.empty()) {
            ImGui::TextDisabled("No paths registered.");
            return;
        }

        std::string filterStr = pathFilterBuffer;
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        if (ImGui::BeginTable("PathsTable", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY)) {

            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& key : allKeys) {
                if (!filterStr.empty()) {
                    std::string lowerKey = key;
                    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                    if (lowerKey.find(filterStr) == std::string::npos) continue;
                }

                std::string currentPath = m_filePathSystem->GetPath(key);
                bool isFileSelector = (key == "ClipL" || key == "ClipG" || key == "T5XXL");

                ImGui::TableNextRow();

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
                                m_filePathSystem->SetPath(key, newPath);
                            }
                        }
                    }
                    else {
                        if (FileDialog::SelectFolder("Select Directory", newPath, defaultPath)) {
                            if (!newPath.empty() && newPath != currentPath) {
                                m_filePathSystem->SetPath(key, newPath);
                            }
                        }
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::Button("X", ImVec2(30, 0))) {
                    m_filePathSystem->SetPath(key, "");
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
            }

            ImGui::EndTable();
        }

        ImGui::Separator();
        std::string modelRoot = m_filePathSystem->GetPath("ModelRoot");
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
                    m_filePathSystem->SetPath(key, modelRoot + subdir);
                }
                std::string clipDir = modelRoot + "/clip";
                m_filePathSystem->SetPath("ClipL", clipDir + "/clip_l.safetensors");
                m_filePathSystem->SetPath("ClipG", clipDir + "/clip_g.safetensors");
                m_filePathSystem->SetPath("T5XXL", clipDir + "/t5xxl.safetensors");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(Updates all model-related paths)");
        }
    }

    void SettingsView::RenderActionButtons() {
        if (!m_settingsSystem) return;

        if (m_settingsSystem->HasAnyUnsavedChanges()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "* Unsaved changes");
            ImGui::SameLine();
        }

        if (ImGui::Button("Apply")) {
            if (m_settingsSystem->SaveAllSettings()) {
                if (m_filePathSystem) {
                    m_filePathSystem->SaveToFile("../data/settings/paths.json");
                }
                showSaveNotification = true;
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Save and Close")) {
            if (m_settingsSystem->SaveAllSettings()) {
                if (m_filePathSystem) {
                    m_filePathSystem->SaveToFile("../data/settings/paths.json");
                }
                showPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }

        if (m_settingsSystem->HasAnyUnsavedChanges()) {
            ImGui::SameLine();
            if (ImGui::Button("Cancel Changes")) {
                m_settingsSystem->RestoreAllFromBackups();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset to Defaults")) {
                m_settingsSystem->ResetAllToDefaults();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            HandlePopupClose();
        }
    }

    void SettingsView::RenderUnsavedChangesDialog() {
        if (showUnsavedChangesDialog) {
            ImGui::OpenPopup("Unsaved Changes");
        }

        if (ImGui::BeginPopupModal("Unsaved Changes", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. What would you like to do?");
            ImGui::Separator();

            if (ImGui::Button("Apply and Close", ImVec2(120, 0))) {
                if (m_settingsSystem && m_settingsSystem->SaveAllSettings()) {
                    if (m_filePathSystem) {
                        m_filePathSystem->SaveToFile("../data/settings/paths.json");
                    }
                    showUnsavedChangesDialog = false;
                    showPopup = false;
                    pendingClose = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();

            if (ImGui::Button("Discard Changes", ImVec2(120, 0))) {
                if (m_settingsSystem) {
                    m_settingsSystem->RestoreAllFromBackups();
                }
                showUnsavedChangesDialog = false;
                showPopup = false;
                pendingClose = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showUnsavedChangesDialog = false;
                pendingClose = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

} // namespace GUI