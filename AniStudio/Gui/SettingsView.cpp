#include "SettingsView.hpp"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include "Events/Events.hpp"

using json = nlohmann::json;

namespace GUI {

void SettingsView::Render() {
        ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings");

        if (ImGui::BeginTabBar("Startup Options"))
        {
            // AniStudio Settings
            if (ImGui::BeginTabItem("AniStudio Settings"))
            {
                if (ImGui::BeginTabBar("AniStudio Options")) {
                    // "Paths" tab
                    if (ImGui::BeginTabItem("Paths")) {
                        RenderSettingsWindow();
                        ImGui::EndTabItem();
                    }

                    // "General" tab
                    if (ImGui::BeginTabItem("General")) {
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
                ImGui::EndTabItem();
            }

            // SDCPP Settings
            if (ImGui::BeginTabItem("SDCPP Settings"))
            {
                ImGui::NewLine();

                if (ImGui::Button("Save Configuration")) {
                    filePaths.SaveFilepathDefaults();
                }

                ImGui::NewLine();

                if (ImGui::BeginTabBar("SDCPP Options"))
                {
                    // "General" tab
                    if (ImGui::BeginTabItem("General"))
                    {
                        
                        ImGui::EndTabItem();
                    }

                    // "Paths" tab
                    if (ImGui::BeginTabItem("Paths"))
                    {
                        
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    
}

void SettingsView::RenderSettingsWindow() {
    // Define the table column headers

    static bool showSavePopup = false;

    // General Paths Section
    if (ImGui::CollapsingHeader("General Paths", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("GeneralPathsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
            ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            RenderPathRow("Last Open Project Path", filePaths.lastOpenProjectPath);
            RenderPathRow("Default Project Path", filePaths.defaultProjectPath);
            RenderPathRow("Assets Folder Path", filePaths.assetsFolderPath);

            ImGui::EndTable();
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Reset Model Paths")) {
        filePaths.SetByModelRoot();
    }
    // Model Paths Section
    if (ImGui::CollapsingHeader("Model Paths", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("ModelPathsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
            ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            RenderPathRow("Default Model Root Path", filePaths.defaultModelRootPath);
            RenderPathRow("Checkpoint Directory", filePaths.checkpointDir);
            RenderPathRow("Encoder Directory", filePaths.encoderDir);
            RenderPathRow("VAE Directory", filePaths.vaeDir);
            RenderPathRow("UNet Directory", filePaths.unetDir);
            RenderPathRow("LORA Directory", filePaths.loraDir);
            RenderPathRow("ControlNet Directory", filePaths.controlnetDir);
            RenderPathRow("Upscale Directory", filePaths.upscaleDir);

            ImGui::EndTable();
        }
    }

    // Action Buttons
    ImGui::Separator();
    if (ImGui::Button("Save Defaults")) {
        filePaths.SaveFilepathDefaults();
        showSavePopup = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Defaults")) {
        filePaths.LoadFilePathDefaults();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        ANI::Event event;
        event.type = ANI::EventType::CloseSettings;
        ANI::Events::Ref().QueueEvent(event);
    }

    // Save Confirmation Popup
    if (showSavePopup) {
        ImGui::OpenPopup("Settings Saved");
    }

    if (ImGui::BeginPopupModal("Settings Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Settings have been successfully saved.");
        if (ImGui::Button("OK")) {
            showSavePopup = false;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            ANI::Event event;
            event.type = ANI::EventType::CloseSettings;
            ANI::Events::Ref().QueueEvent(event);
        }
    }
}

void SettingsView::RenderPathRow(const char *label, std::string &path) {
    ImGui::TableNextRow();

    // Path Name Column
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);

    // Select New Path Button Column
    ImGui::TableNextColumn();
    std::string buttonID = std::string("...##") + label;
    if (ImGui::Button(buttonID.c_str())) {
        IGFD::FileDialogConfig config;
        config.path = path;
        ImGuiFileDialog::Instance()->OpenDialog("Choose Path", "Select New Path", nullptr, config);
    }

    // Handle File Dialog Result
    if (ImGuiFileDialog::Instance()->Display("Choose Path",32,ImVec2(500,400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            path = ImGuiFileDialog::Instance()->GetCurrentPath();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();

    // Reset Path Button
    std::string resetID = std::string("R##") + label;
    if (ImGui::Button(resetID.c_str())) {
        path.clear(); // Or reset to a default value if needed
    }

    // Full Path Column
    ImGui::TableNextColumn();
    ImGui::Text("%s", path.c_str());
}

} // namespace GUI