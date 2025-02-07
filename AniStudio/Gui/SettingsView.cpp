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

                    // "Style" tab
                    if (ImGui::BeginTabItem("Style")) {
                        ShowStyleEditor(nullptr);
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
        config.path = path.empty() ? "." : path;
        config.flags = ImGuiFileDialogFlags_Modal;                  // Since these are all directory paths
        std::string dialogID = std::string("ChoosePath##") + label; // Unique ID for each row
        ImGuiFileDialog::Instance()->OpenDialog(dialogID.c_str(), "Select New Path", nullptr, config);
    }

    // Handle File Dialog Result
    std::string dialogID = std::string("ChoosePath##") + label;
    if (ImGuiFileDialog::Instance()->Display(dialogID.c_str(), 32, ImVec2(500, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            if (!selectedPath.empty()) {
                path = selectedPath;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();

    // Reset Path Button
    std::string resetID = std::string("R##") + label;
    if (ImGui::Button(resetID.c_str())) {
        path.clear();
    }

    // Full Path Column
    ImGui::TableNextColumn();
    ImGui::Text("%s", path.c_str());
}

void SettingsView::ShowFontSelector(const char *label) {
    ImGuiIO &io = ImGui::GetIO();
    ImFont *font_current = ImGui::GetFont();
    if (ImGui::BeginCombo(label, font_current->GetDebugName())) {
        for (ImFont *font : io.Fonts->Fonts) {
            ImGui::PushID((void *)font);
            if (ImGui::Selectable(font->GetDebugName(), font == font_current))
                io.FontDefault = font;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
}

bool SettingsView::ShowStyleSelector(const char *label) {
    static int style_idx = -1;
    if (ImGui::Combo(label, &style_idx, "Dark\0Light\0Classic\0")) {
        switch (style_idx) {
        case 0:
            ImGui::StyleColorsDark();
            break;
        case 1:
            ImGui::StyleColorsLight();
            break;
        case 2:
            ImGui::StyleColorsClassic();
            break;
        }
        return true;
    }
    return false;
}

void SettingsView::ShowStyleEditor(ImGuiStyle *ref) {
    ImGuiStyle &style = ImGui::GetStyle();
    static ImGuiStyle ref_saved_style;

    // Default to using internal storage as reference
    static bool init = true;
    if (init && ref == NULL)
        ref_saved_style = style;
    init = false;
    if (ref == NULL)
        ref = &ref_saved_style;

    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

    if (ShowStyleSelector("Colors##Selector"))
        ref_saved_style = style;
    ShowFontSelector("Fonts##Selector");

    // Save/Revert button
    if (ImGui::Button("Save Style")) {
        SaveStyleToFile(style, "../data/defaults/style.json");
        *ref = ref_saved_style = style;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Style")) {
        LoadStyleFromFile(style, "../data/defaults/style.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Revert Style"))
        style = *ref;

    ImGui::Separator();

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Sizes")) {
            ImGui::Text("Main");
            ImGui::SliderFloat2("WindowPadding", (float *)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("FramePadding", (float *)&style.FramePadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("ItemSpacing", (float *)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("ItemInnerSpacing", (float *)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
            ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
            ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");

            ImGui::Text("Borders");
            ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");

            ImGui::Text("Rounding");
            ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Colors")) {
            static ImGuiTextFilter filter;
            filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

            static ImGuiColorEditFlags alpha_flags = 0;
            if (ImGui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None))
                alpha_flags = ImGuiColorEditFlags_None;
            ImGui::SameLine();
            if (ImGui::RadioButton("Alpha", alpha_flags == ImGuiColorEditFlags_AlphaPreview))
                alpha_flags = ImGuiColorEditFlags_AlphaPreview;
            ImGui::SameLine();
            if (ImGui::RadioButton("Both", alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf))
                alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf;

            ImGui::BeginChild("##colors", ImVec2(0, 0), true);
            ImGui::PushItemWidth(-160);
            for (int i = 0; i < ImGuiCol_COUNT; i++) {
                const char *name = ImGui::GetStyleColorName(i);
                if (!filter.PassFilter(name))
                    continue;
                ImGui::PushID(i);
                ImGui::ColorEdit4("##color", (float *)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
                if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0) {
                    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                    if (ImGui::Button("Save"))
                        ref->Colors[i] = style.Colors[i];
                    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                    if (ImGui::Button("Revert"))
                        style.Colors[i] = ref->Colors[i];
                }
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::TextUnformatted(name);
                ImGui::PopID();
            }
            ImGui::PopItemWidth();
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::PopItemWidth();
}


} // namespace GUI