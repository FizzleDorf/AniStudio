#include "ImGuiStyleSettingsComponent.hpp"
#include "FilePathSystem.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace ECS {

    ImGuiStyleSettingsComponent::ImGuiStyleSettingsComponent() {
        compName = "ImGuiStyleSettingsComponent";
    }

    std::string ImGuiStyleSettingsComponent::GetStylesDirectory() const {
        if (m_filePathSystem) {
            std::string dir = m_filePathSystem->GetPath("styles");
            if (!dir.empty()) {
                if (dir.back() != '/' && dir.back() != '\\')
                    dir += '/';
                return dir;
            }
        }
        return "./data/styles/";
    }

    void ImGuiStyleSettingsComponent::EnsureInitialized() {
        if (!isInitialized && imguiContext) {
            ImGui::SetCurrentContext(imguiContext);
            currentStyle = ImGui::GetStyle();
            backupStyle = currentStyle;
            isInitialized = true;
            ScanStylesDirectory();
            RebuildDisplayList();
            selectedStyleIndex = 0;
        }
    }

    void ImGuiStyleSettingsComponent::ScanStylesDirectory() {
        availableStyles.clear();
        std::string dirPath = GetStylesDirectory();
        if (!std::filesystem::exists(dirPath)) {
            std::filesystem::create_directories(dirPath);
        }
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".json") {
                StyleFileEntry sfe;
                sfe.name = entry.path().stem().string();
                sfe.path = entry.path().string();
                availableStyles.push_back(sfe);
            }
        }
        std::sort(availableStyles.begin(), availableStyles.end(),
            [](const StyleFileEntry& a, const StyleFileEntry& b) { return a.name < b.name; });
    }

    void ImGuiStyleSettingsComponent::RebuildDisplayList() {
        displayNames.clear();
        displayNames.reserve(4 + availableStyles.size());
        displayNames.push_back("Dark");
        displayNames.push_back("Light");
        displayNames.push_back("Classic");
        displayNames.push_back("Custom Dark");
        for (const auto& entry : availableStyles) {
            displayNames.push_back(entry.name);
        }
        if (selectedStyleIndex < 0 || selectedStyleIndex >= (int)displayNames.size())
            selectedStyleIndex = 0;
    }

    void ImGuiStyleSettingsComponent::ApplyBuiltInStyle(int index) {
        switch (index) {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
        case 3: SetCustomDarkTheme(); break;
        default: ImGui::StyleColorsDark(); break;
        }
        currentStyle = ImGui::GetStyle();
        currentStyleFile.clear();
        hasChanges = true;
    }

    void ImGuiStyleSettingsComponent::ApplyFileStyle(const std::string& path) {
        ImGuiStyle loadedStyle;
        if (LoadStyleFromFile(loadedStyle, path)) {
            currentStyle = loadedStyle;
            ImGui::GetStyle() = currentStyle;
            currentStyleFile = path;
            hasChanges = true;
        }
    }

    bool ImGuiStyleSettingsComponent::FilterPass(const std::string& section, const std::string& filter) const {
        if (filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void ImGuiStyleSettingsComponent::RenderUI() {
        RenderFilteredUI("");
    }

    void ImGuiStyleSettingsComponent::RenderFilteredUI(const std::string& filter) {
        if (!imguiContext) return;
        EnsureInitialized();
        ImGui::SetCurrentContext(imguiContext);

        if (ImGui::BeginChild("ImGuiStyleSettings", ImVec2(0, 0), false)) {
            ImGuiStyle& style = currentStyle;

            if (FilterPass("Style Presets", filter)) {
                const char* preview = displayNames[selectedStyleIndex].c_str();
                if (ImGui::BeginCombo("Theme", preview)) {
                    for (int i = 0; i < (int)displayNames.size(); ++i) {
                        bool isSelected = (i == selectedStyleIndex);
                        if (ImGui::Selectable(displayNames[i].c_str(), isSelected)) {
                            selectedStyleIndex = i;
                            if (i < 4) {
                                ApplyBuiltInStyle(i);
                            }
                            else {
                                int fileIdx = i - 4;
                                if (fileIdx < (int)availableStyles.size()) {
                                    ApplyFileStyle(availableStyles[fileIdx].path);
                                }
                            }
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                if (ImGui::Button("Refresh List")) {
                    ScanStylesDirectory();
                    RebuildDisplayList();
                    if (selectedStyleIndex >= (int)displayNames.size())
                        selectedStyleIndex = 0;
                }

                ImGui::Separator();
                ImGui::Text("Save current style as:");
                ImGui::InputText("Filename", saveAsFilename, IM_ARRAYSIZE(saveAsFilename));
                ImGui::SameLine();
                if (ImGui::Button("Save As")) {
                    std::string filename(saveAsFilename);
                    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json")
                        filename += ".json";
                    std::string fullPath = GetStylesDirectory() + filename;
                    std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
                    SaveStyleToFile(currentStyle, fullPath);
                    ScanStylesDirectory();
                    RebuildDisplayList();
                    for (int i = 0; i < (int)availableStyles.size(); ++i) {
                        if (availableStyles[i].name == filename) {
                            selectedStyleIndex = 4 + i;
                            break;
                        }
                    }
                }
                ImGui::Separator();
            }

            if (FilterPass("Size Settings", filter)) {
                if (ImGui::SliderFloat2("Window Padding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat2("Frame Padding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat2("Item Spacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat2("Item Inner Spacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Indent Spacing", &style.IndentSpacing, 0.0f, 30.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Scrollbar Size", &style.ScrollbarSize, 1.0f, 20.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Grab Min Size", &style.GrabMinSize, 1.0f, 20.0f, "%.1f")) hasChanges = true;
            }

            if (FilterPass("Border Settings", filter)) {
                if (ImGui::SliderFloat("Window Border Size", &style.WindowBorderSize, 0.0f, 1.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Child Border Size", &style.ChildBorderSize, 0.0f, 1.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Popup Border Size", &style.PopupBorderSize, 0.0f, 1.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Frame Border Size", &style.FrameBorderSize, 0.0f, 1.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Tab Border Size", &style.TabBorderSize, 0.0f, 1.0f, "%.1f")) hasChanges = true;
            }

            if (FilterPass("Rounding Settings", filter)) {
                if (ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Child Rounding", &style.ChildRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Popup Rounding", &style.PopupRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
                if (ImGui::SliderFloat("Tab Rounding", &style.TabRounding, 0.0f, 12.0f, "%.1f")) hasChanges = true;
            }

            if (FilterPass("Color Settings", filter)) {
                static ImGuiTextFilter colorFilter;
                colorFilter.Draw("Filter Colors", ImGui::GetFontSize() * 16);
                static ImGuiColorEditFlags alphaFlags = ImGuiColorEditFlags_AlphaPreview;
                if (ImGui::RadioButton("Opaque", alphaFlags == ImGuiColorEditFlags_None)) alphaFlags = ImGuiColorEditFlags_None;
                ImGui::SameLine();
                if (ImGui::RadioButton("Alpha", alphaFlags == ImGuiColorEditFlags_AlphaPreview)) alphaFlags = ImGuiColorEditFlags_AlphaPreview;
                ImGui::SameLine();
                if (ImGui::RadioButton("Both", alphaFlags == ImGuiColorEditFlags_AlphaPreviewHalf)) alphaFlags = ImGuiColorEditFlags_AlphaPreviewHalf;
                if (ImGui::BeginChild("colors", ImVec2(0, 300), true)) {
                    ImGui::PushItemWidth(-160);
                    for (int i = 0; i < ImGuiCol_COUNT; i++) {
                        const char* name = ImGui::GetStyleColorName(i);
                        if (!colorFilter.PassFilter(name)) continue;
                        ImGui::PushID(i);
                        if (ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alphaFlags)) {
                            hasChanges = true;
                        }
                        if (memcmp(&style.Colors[i], &backupStyle.Colors[i], sizeof(ImVec4)) != 0) {
                            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                            if (ImGui::Button("Revert")) {
                                style.Colors[i] = backupStyle.Colors[i];
                                hasChanges = true;
                            }
                        }
                        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                        ImGui::TextUnformatted(name);
                        ImGui::PopID();
                    }
                    ImGui::PopItemWidth();
                }
                ImGui::EndChild();
            }

            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void ImGuiStyleSettingsComponent::RenderActionButtons() {
        if (ImGui::Button("Save Settings")) SaveSettings();
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) ResetToDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) RestoreFromBackup();
        if (hasChanges) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
        }
    }

    bool ImGuiStyleSettingsComponent::SaveSettings() {
        if (!imguiContext) return false;
        EnsureInitialized();
        ImGui::SetCurrentContext(imguiContext);
        std::string filePath = GetSettingsDirectory() + "/imgui_style.json";
        SaveStyleToFile(currentStyle, filePath);
        ImGui::GetStyle() = currentStyle;
        hasChanges = false;
        CreateBackup();
        return true;
    }

    bool ImGuiStyleSettingsComponent::LoadSettings() {
        if (!imguiContext) return false;
        EnsureInitialized();
        ImGui::SetCurrentContext(imguiContext);
        std::string filePath = GetSettingsDirectory() + "/imgui_style.json";
        bool loaded = false;
        if (std::filesystem::exists(filePath)) {
            loaded = LoadStyleFromFile(currentStyle, filePath);
        }
        if (!loaded) {
            SetCustomDarkTheme();
            currentStyle = ImGui::GetStyle();
        }
        ImGui::GetStyle() = currentStyle;
        hasChanges = false;
        CreateBackup();
        currentStyleFile.clear();
        selectedStyleIndex = 0;
        RebuildDisplayList();
        return true;
    }

    void ImGuiStyleSettingsComponent::ResetToDefaults() {
        if (!imguiContext) return;
        EnsureInitialized();
        SetCustomDarkTheme();
        currentStyle = ImGui::GetStyle();
        currentStyleFile.clear();
        selectedStyleIndex = 3;
        hasChanges = true;
    }

    void ImGuiStyleSettingsComponent::CreateBackup() {
        if (!imguiContext) return;
        EnsureInitialized();
        backupStyle = currentStyle;
    }

    void ImGuiStyleSettingsComponent::RestoreFromBackup() {
        if (!imguiContext) return;
        EnsureInitialized();
        currentStyle = backupStyle;
        ImGui::GetStyle() = currentStyle;
        currentStyleFile.clear();
        selectedStyleIndex = 0;
        hasChanges = false;
    }

    void ImGuiStyleSettingsComponent::SaveStyleToFile(const ImGuiStyle& style, const std::string& filename) {
        nlohmann::json j;
        j["WindowPadding"] = { style.WindowPadding.x, style.WindowPadding.y };
        j["FramePadding"] = { style.FramePadding.x, style.FramePadding.y };
        j["ItemSpacing"] = { style.ItemSpacing.x, style.ItemSpacing.y };
        j["ItemInnerSpacing"] = { style.ItemInnerSpacing.x, style.ItemInnerSpacing.y };
        j["IndentSpacing"] = style.IndentSpacing;
        j["ScrollbarSize"] = style.ScrollbarSize;
        j["GrabMinSize"] = style.GrabMinSize;
        j["WindowBorderSize"] = style.WindowBorderSize;
        j["ChildBorderSize"] = style.ChildBorderSize;
        j["PopupBorderSize"] = style.PopupBorderSize;
        j["FrameBorderSize"] = style.FrameBorderSize;
        j["TabBorderSize"] = style.TabBorderSize;
        j["WindowRounding"] = style.WindowRounding;
        j["ChildRounding"] = style.ChildRounding;
        j["FrameRounding"] = style.FrameRounding;
        j["PopupRounding"] = style.PopupRounding;
        j["ScrollbarRounding"] = style.ScrollbarRounding;
        j["GrabRounding"] = style.GrabRounding;
        j["TabRounding"] = style.TabRounding;
        j["Colors"] = nlohmann::json::array();
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            const ImVec4& col = style.Colors[i];
            j["Colors"].push_back({ {"name", ImGui::GetStyleColorName(i)}, {"r", col.x}, {"g", col.y}, {"b", col.z}, {"a", col.w} });
        }
        std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
        std::ofstream file(filename);
        if (file.is_open()) file << j.dump(4);
    }

    bool ImGuiStyleSettingsComponent::LoadStyleFromFile(ImGuiStyle& style, const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        nlohmann::json j;
        file >> j;
        file.close();
        if (j.contains("WindowPadding")) style.WindowPadding = ImVec2(j["WindowPadding"][0], j["WindowPadding"][1]);
        if (j.contains("FramePadding")) style.FramePadding = ImVec2(j["FramePadding"][0], j["FramePadding"][1]);
        if (j.contains("ItemSpacing")) style.ItemSpacing = ImVec2(j["ItemSpacing"][0], j["ItemSpacing"][1]);
        if (j.contains("ItemInnerSpacing")) style.ItemInnerSpacing = ImVec2(j["ItemInnerSpacing"][0], j["ItemInnerSpacing"][1]);
        if (j.contains("IndentSpacing")) style.IndentSpacing = j["IndentSpacing"];
        if (j.contains("ScrollbarSize")) style.ScrollbarSize = j["ScrollbarSize"];
        if (j.contains("GrabMinSize")) style.GrabMinSize = j["GrabMinSize"];
        if (j.contains("WindowBorderSize")) style.WindowBorderSize = j["WindowBorderSize"];
        if (j.contains("ChildBorderSize")) style.ChildBorderSize = j["ChildBorderSize"];
        if (j.contains("PopupBorderSize")) style.PopupBorderSize = j["PopupBorderSize"];
        if (j.contains("FrameBorderSize")) style.FrameBorderSize = j["FrameBorderSize"];
        if (j.contains("TabBorderSize")) style.TabBorderSize = j["TabBorderSize"];
        if (j.contains("WindowRounding")) style.WindowRounding = j["WindowRounding"];
        if (j.contains("ChildRounding")) style.ChildRounding = j["ChildRounding"];
        if (j.contains("FrameRounding")) style.FrameRounding = j["FrameRounding"];
        if (j.contains("PopupRounding")) style.PopupRounding = j["PopupRounding"];
        if (j.contains("ScrollbarRounding")) style.ScrollbarRounding = j["ScrollbarRounding"];
        if (j.contains("GrabRounding")) style.GrabRounding = j["GrabRounding"];
        if (j.contains("TabRounding")) style.TabRounding = j["TabRounding"];
        if (j.contains("Colors")) {
            for (const auto& color : j["Colors"]) {
                for (int i = 0; i < ImGuiCol_COUNT; i++) {
                    if (color["name"] == ImGui::GetStyleColorName(i)) {
                        style.Colors[i] = ImVec4(color["r"], color["g"], color["b"], color["a"]);
                        break;
                    }
                }
            }
        }
        return true;
    }

    void ImGuiStyleSettingsComponent::SetCustomDarkTheme() {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;
    }

}