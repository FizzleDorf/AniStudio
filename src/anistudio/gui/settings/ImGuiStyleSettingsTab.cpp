#include "ImGuiStyleSettingsTab.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "UISchema.hpp"
#include "FileDialogUtil.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>

namespace ECS {

    ImGuiStyleSettingsTab::ImGuiStyleSettingsTab(SettingsSystem& sys, ImGuiStyleSettingsComponent& comp)
        : m_settingsSystem(sys), m_comp(comp) {
    }

    std::string ImGuiStyleSettingsTab::GetStylesDirectory() const {
        std::string dir = m_settingsSystem.GetPath("styles");
        if (dir.empty()) dir = "./data/styles";
        if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') dir += '/';
        return dir;
    }

    std::vector<std::string> ImGuiStyleSettingsTab::ScanStyleNames() const {
        std::vector<std::string> names = { "Dark", "Light", "Classic" };
        std::string dirPath = GetStylesDirectory();
        if (!std::filesystem::exists(dirPath)) return names;
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".json") files.push_back(entry.path().stem().string());
        }
        std::sort(files.begin(), files.end());
        names.insert(names.end(), files.begin(), files.end());
        return names;
    }

    std::vector<std::string> ImGuiStyleSettingsTab::ScanStylePaths() const {
        std::vector<std::string> paths;
        std::string dirPath = GetStylesDirectory();
        if (!std::filesystem::exists(dirPath)) return paths;
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".json") files.push_back(entry.path().string());
        }
        std::sort(files.begin(), files.end());
        return files;
    }

    void ImGuiStyleSettingsTab::ApplyBuiltInStyle(int index) {
        if (!m_imguiContext) return;
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
        switch (index) {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
        default: ImGui::StyleColorsDark(); break;
        }
        m_comp.currentStyle = ImGui::GetStyle();
    }

    void ImGuiStyleSettingsTab::ApplyStyleFromFile(const std::string& path) {
        try {
            std::ifstream f(path);
            if (!f.is_open()) return;
            nlohmann::json j;
            f >> j;
            m_comp.Deserialize(j);
            if (m_imguiContext) {
                ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
                ImGui::GetStyle() = m_comp.currentStyle;
            }
        }
        catch (const std::exception&) {}
    }

    void ImGuiStyleSettingsTab::SaveStyleAs(const ImGuiStyle& style, const std::string& filename) {
        try {
            std::string dir = GetStylesDirectory();
            std::filesystem::create_directories(dir);
            std::string fullPath = dir + filename;
            ImGuiStyleSettingsComponent temp;
            temp.currentStyle = style;
            nlohmann::json j = temp.Serialize();
            std::ofstream f(fullPath);
            if (f.is_open()) f << j.dump(4);
        }
        catch (const std::exception&) {}
    }

    void ImGuiStyleSettingsTab::SaveCurrentStyle(const std::string& styleName) {
        if (styleName.empty()) return;
        std::string filename = styleName;
        if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json")
            filename += ".json";
        SaveStyleAs(m_comp.currentStyle, filename);
    }

    void ImGuiStyleSettingsTab::DeleteStyleFile(const std::string& styleName) {
        if (styleName.empty()) return;
        if (styleName == "Dark" || styleName == "Light" || styleName == "Classic") return;

        std::string path = GetStylesDirectory() + styleName + ".json";
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    void ImGuiStyleSettingsTab::Render() {
        if (!m_imguiContext) return;
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));

        if (ImGui::BeginChild("ImGuiStyleSettings", ImVec2(0, 0), false)) {
            ImGui::Checkbox("Show Preview", &m_showPreview);
            ImGui::Separator();

            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Style Presets")) RenderStylePresets();

            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Metrics")) RenderMetricSchema();

            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Colors")) RenderColors();
        }
        ImGui::EndChild();

        if (m_showPreview) RenderPreviewWindow();
    }

    void ImGuiStyleSettingsTab::RenderStylePresets() {
        std::vector<std::string> displayNames = ScanStyleNames();
        std::vector<std::string> stylePaths = ScanStylePaths();

        if (m_selectedStyleIndex < 0 || m_selectedStyleIndex >= (int)displayNames.size())
            m_selectedStyleIndex = 0;

        ImGui::TextUnformatted("Theme");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(240.0f);
        const char* preview = displayNames[m_selectedStyleIndex].c_str();
        if (ImGui::BeginCombo("##ThemeCombo", preview)) {
            for (int i = 0; i < (int)displayNames.size(); ++i) {
                bool isSelected = (i == m_selectedStyleIndex);
                if (ImGui::Selectable(displayNames[i].c_str(), isSelected)) {
                    m_selectedStyleIndex = i;
                    if (i < 3) ApplyBuiltInStyle(i);
                    else {
                        int fileIdx = i - 3;
                        if (fileIdx < (int)stylePaths.size())
                            ApplyStyleFromFile(stylePaths[fileIdx]);
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (ImGui::Button("Import Style...")) {
            std::string path;
            std::string filter = "Style Files{.json},All Files{.*}";
            std::string start = GetStylesDirectory();
            if (FileDialog::OpenFile("Import Style", filter, path, start)) {
                ApplyStyleFromFile(path);
                std::error_code ec;
                std::filesystem::copy_file(path,
                    GetStylesDirectory() + std::filesystem::path(path).filename().string(),
                    std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Pick a .json style file and copy it into the styles folder");
        }

        ImGui::SameLine();
        if (ImGui::Button("Open Styles Folder")) {
            std::string dir = GetStylesDirectory();
            std::filesystem::create_directories(dir);
#ifdef _WIN32
            std::system(("explorer \"" + dir + "\"").c_str());
#elif __APPLE__
            std::system(("open \"" + dir + "\"").c_str());
#else
            std::system(("xdg-open \"" + dir + "\"").c_str());
#endif
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open the folder that holds .json style files");
        }

        ImGui::Separator();

        ImGui::TextUnformatted("Save Current Style As:");
        ImGui::SetNextItemWidth(240.0f);
        ImGui::InputText("##StyleSaveName", m_saveAsFilename,
            IM_ARRAYSIZE(m_saveAsFilename));
        ImGui::SameLine();

        const bool nameEmpty = m_saveAsFilename[0] == '\0';

        ImGui::BeginDisabled(nameEmpty);
        if (ImGui::Button("Save")) {
            std::string filename(m_saveAsFilename);
            if (filename.size() >= 5 &&
                filename.substr(filename.size() - 5) == ".json") {
                filename = filename.substr(0, filename.size() - 5);
            }
            SaveCurrentStyle(filename);
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Save Over Selected")) {
            if (m_selectedStyleIndex >= 3 &&
                m_selectedStyleIndex < (int)displayNames.size()) {
                SaveCurrentStyle(displayNames[m_selectedStyleIndex]);
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Overwrite the file for the selected theme.\n"
                "Only enabled for user styles (not Dark/Light/Classic).");
        }
        if (m_selectedStyleIndex < 3) {
            ImGui::SameLine();
            ImGui::TextDisabled("(select a user theme above)");
        }

        ImGui::Separator();

        const bool isUserStyle =
            m_selectedStyleIndex >= 3 &&
            m_selectedStyleIndex < (int)displayNames.size();

        ImGui::BeginDisabled(!isUserStyle);
        if (ImGui::Button("Delete Selected Style")) {
            ImGui::OpenPopup("ConfirmDeleteStyle");
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered() && !isUserStyle) {
            ImGui::SetTooltip("Cannot delete built-in themes");
        }

        if (ImGui::BeginPopupModal("ConfirmDeleteStyle", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Delete style \"%s\"?",
                displayNames[m_selectedStyleIndex].c_str());
            ImGui::TextDisabled("This removes the .json file from the styles folder.");
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(120, 0))) {
                DeleteStyleFile(displayNames[m_selectedStyleIndex]);
                m_selectedStyleIndex = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        if (ImGui::Button("Refresh List")) {
            if (m_selectedStyleIndex >= (int)displayNames.size())
                m_selectedStyleIndex = 0;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Styles folder: %s", GetStylesDirectory().c_str());
    }

    void ImGuiStyleSettingsTab::RenderMetricSchema() {
        auto props = m_comp.GetPropertyMap();
        auto* filePathComp = m_settingsSystem.GetFilePathComponent();
        const std::unordered_map<std::string, std::string>* pathMap =
            filePathComp ? &filePathComp->GetCurrentMap() : nullptr;

        bool changed = UISchema::RenderSchema(
            m_comp.GetSchema(),
            props,
            nullptr,
            m_comp.GetCompName(),
            (int)m_settingsSystem.GetSettingsEntity(),
            pathMap);

        if (changed) ImGui::GetStyle() = m_comp.currentStyle;
    }

    void ImGuiStyleSettingsTab::RenderColors() {
        ImGuiStyle& style = m_comp.currentStyle;
        static ImGuiTextFilter colorFilter;
        colorFilter.Draw("Filter Colors", ImGui::GetFontSize() * 16);
        static ImGuiColorEditFlags alphaFlags = ImGuiColorEditFlags_AlphaPreview;
        if (ImGui::RadioButton("Opaque", alphaFlags == ImGuiColorEditFlags_None)) alphaFlags = ImGuiColorEditFlags_None;
        ImGui::SameLine();
        if (ImGui::RadioButton("Alpha", alphaFlags == ImGuiColorEditFlags_AlphaPreview)) alphaFlags = ImGuiColorEditFlags_AlphaPreview;
        ImGui::SameLine();
        if (ImGui::RadioButton("Both", alphaFlags == ImGuiColorEditFlags_AlphaPreviewHalf)) alphaFlags = ImGuiColorEditFlags_AlphaPreviewHalf;

        bool changed = false;
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            const char* name = ImGui::GetStyleColorName(i);
            if (!colorFilter.PassFilter(name)) continue;
            ImGui::PushID(i);
            if (ImGui::ColorEdit4("##color", (float*)&style.Colors[i],
                ImGuiColorEditFlags_AlphaBar | alphaFlags)) {
                changed = true;
            }
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            ImGui::TextUnformatted(name);
            ImGui::PopID();
        }
        if (changed) ImGui::GetStyle() = m_comp.currentStyle;
    }

    void ImGuiStyleSettingsTab::RenderPreviewWindow() {
        ImGui::ShowDemoWindow(&m_showPreview);
    }

}