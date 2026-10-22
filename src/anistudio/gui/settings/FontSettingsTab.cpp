#include "FontSettingsTab.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "UISchema.hpp"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <filesystem>

namespace ECS {

    void FontSettingsTab::ScanFontsDirectories(FontSettingsComponent& comp) {
        comp.availableFonts.clear();
        comp.availableFonts.push_back({ "Default", "" });

        const std::string fontsDir = "assets/fonts";
        try {
            if (std::filesystem::exists(fontsDir) && std::filesystem::is_directory(fontsDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(fontsDir)) {
                    if (!entry.is_regular_file()) continue;
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".ttf" || ext == ".otf") {
                        FontSettingsComponent::FontEntry fe;
                        fe.name = entry.path().stem().string();
                        fe.path = std::filesystem::absolute(entry.path()).string();
                        comp.availableFonts.push_back(fe);
                    }
                }
            }
        }
        catch (...) {}

        comp.availableIconFonts.clear();
        const std::string iconDir = "assets/icon_fonts";
        try {
            if (std::filesystem::exists(iconDir) && std::filesystem::is_directory(iconDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(iconDir)) {
                    if (!entry.is_regular_file()) continue;
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".ttf" || ext == ".otf") {
                        FontSettingsComponent::FontEntry fe;
                        fe.name = entry.path().stem().string();
                        fe.path = std::filesystem::absolute(entry.path()).string();
                        comp.availableIconFonts.push_back(fe);
                    }
                }
            }
        }
        catch (...) {}

        if (comp.selectedFontName.empty()) comp.selectedFontName = "Default";
        if (comp.selectedIconFontName.empty() && !comp.availableIconFonts.empty()) {
            comp.selectedIconFontName = comp.availableIconFonts[0].name;
        }
    }

    void FontSettingsTab::ApplyFontToImGui(FontSettingsComponent& comp, ImGuiContext* ctx) {
        if (!ctx) return;
        ImGuiContext* prev = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(ctx);

        ImGui_ImplOpenGL3_DestroyDeviceObjects();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        const float fontSize = 16.0f;

        std::string mainPath;
        for (const auto& e : comp.availableFonts) {
            if (e.name == comp.selectedFontName) { mainPath = e.path; break; }
        }
        std::string iconPath;
        for (const auto& e : comp.availableIconFonts) {
            if (e.name == comp.selectedIconFontName) { iconPath = e.path; break; }
        }

        ImFont* mainFont = nullptr;
        if (!mainPath.empty() && std::filesystem::exists(mainPath)) {
            ImFontConfig cfg;
            cfg.OversampleH = 3;
            cfg.OversampleV = 1;
            cfg.PixelSnapH = true;
            mainFont = io.Fonts->AddFontFromFileTTF(mainPath.c_str(), fontSize, &cfg);
        }
        if (!mainFont) mainFont = io.Fonts->AddFontDefault();

        if (!iconPath.empty() && std::filesystem::exists(iconPath)) {
            static const ImWchar iconRanges[] = { 0xf000, 0xf8ff, 0 };
            ImFontConfig cfg;
            cfg.MergeMode = true;
            cfg.PixelSnapH = true;
            cfg.OversampleH = 3;
            cfg.OversampleV = 1;
            cfg.GlyphMinAdvanceX = fontSize * 0.8f;
            cfg.GlyphMaxAdvanceX = fontSize * 0.8f;
            io.Fonts->AddFontFromFileTTF(iconPath.c_str(), fontSize, &cfg, iconRanges);
        }

        if (!io.Fonts->Build()) {
            io.Fonts->Clear();
            mainFont = io.Fonts->AddFontDefault();
            io.Fonts->Build();
        }
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        io.FontDefault = mainFont;
        io.FontGlobalScale = comp.m_globalFontScale;

        ImGui::SetCurrentContext(prev);
    }

    FontSettingsTab::FontSettingsTab(SettingsSystem& sys, FontSettingsComponent& comp)
        : m_settingsSystem(sys), m_comp(comp) {
    }

    void FontSettingsTab::Render() {
        if (!m_imguiContext) return;
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
        if (ImGui::BeginChild("FontSettings", ImVec2(0, 0), false)) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Font Family")) RenderFontFamily();

            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Icon Font")) RenderIconFontFamily();

            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Global Scale")) RenderScaleSchema();
        }
        ImGui::EndChild();
    }

    void FontSettingsTab::RenderFontFamily() {
        ImGui::TextUnformatted("Font Family");
        if (ImGui::BeginCombo("##FontFamily", m_comp.selectedFontName.c_str())) {
            for (const auto& entry : m_comp.availableFonts) {
                bool isSelected = (entry.name == m_comp.selectedFontName);
                if (ImGui::Selectable(entry.name.c_str(), isSelected)) {
                    if (m_comp.selectedFontName != entry.name) {
                        m_comp.selectedFontName = entry.name;
                        ApplyFontToImGui(m_comp, static_cast<ImGuiContext*>(m_imguiContext));
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Font List")) {
            ScanFontsDirectories(m_comp);
        }
    }

    void FontSettingsTab::RenderIconFontFamily() {
        ImGui::TextUnformatted("Icon Font");

        if (m_comp.availableIconFonts.empty()) {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "No icon fonts found in assets/icon_fonts/");
            ImGui::TextDisabled("Place your Font Awesome .ttf or .otf file in:");
            ImGui::TextDisabled("  assets/icon_fonts/fa-solid-900.otf");
            ImGui::TextDisabled("  assets/icon_fonts/fa-solid-900.ttf");
            return;
        }

        if (ImGui::BeginCombo("##IconFontFamily", m_comp.selectedIconFontName.c_str())) {
            for (const auto& entry : m_comp.availableIconFonts) {
                bool isSelected = (entry.name == m_comp.selectedIconFontName);
                if (ImGui::Selectable(entry.name.c_str(), isSelected)) {
                    if (m_comp.selectedIconFontName != entry.name) {
                        m_comp.selectedIconFontName = entry.name;
                        ApplyFontToImGui(m_comp, static_cast<ImGuiContext*>(m_imguiContext));
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Icon fonts should contain glyphs in the Unicode Private Use Area (PUA)");
            ImGui::Text("Common ranges: f000-f8ff for Font Awesome");
            ImGui::EndTooltip();
        }

        std::string iconPath;
        for (const auto& entry : m_comp.availableIconFonts) {
            if (entry.name == m_comp.selectedIconFontName) { iconPath = entry.path; break; }
        }
        if (!iconPath.empty()) ImGui::TextDisabled("Path: %s", iconPath.c_str());

        ImGui::Text("Preview: ");
        ImGui::SameLine();
        ImGui::Text("%s %s %s %s %s",
            Icon::Play().c_str(),
            Icon::Stop().c_str(),
            Icon::Save().c_str(),
            Icon::Refresh().c_str(),
            Icon::Trash().c_str());
    }

    void FontSettingsTab::RenderScaleSchema() {
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

        if (changed) {
            ImGui::GetIO().FontGlobalScale = m_comp.m_globalFontScale;
        }
    }

}