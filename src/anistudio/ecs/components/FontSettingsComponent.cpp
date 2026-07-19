// FontSettingsComponent.cpp
#include "FontSettingsComponent.hpp"

#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace ECS {

    FontSettingsComponent::FontRebuildCallback FontSettingsComponent::s_fontRebuildCallback = nullptr;
    FontSettingsComponent* FontSettingsComponent::s_instance = nullptr;

    FontSettingsComponent::FontSettingsComponent() {
        s_instance = this;
    }

    void FontSettingsComponent::EnsureInitialized() {
        if (isInitialized) return;
        ScanFontsDirectory();
        if (selectedFontName.empty()) {
            selectedFontName = "Default";
        }
        isInitialized = true;
    }

    void FontSettingsComponent::ScanFontsDirectory() {
        availableFonts.clear();
        availableFonts.push_back(FontEntry{ "Default", "" });

        const std::string fontsDir = "assets/fonts";

        try {
            if (std::filesystem::exists(fontsDir) && std::filesystem::is_directory(fontsDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(fontsDir)) {
                    if (!entry.is_regular_file()) continue;

                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    if (ext == ".ttf" || ext == ".otf") {
                        FontEntry fe;
                        fe.name = entry.path().stem().string();
                        fe.path = std::filesystem::absolute(entry.path()).string();
                        availableFonts.push_back(fe);
                    }
                }
            }
        }
        catch (...) {
        }

        fontsScanned = true;
    }

    bool FontSettingsComponent::FilterPass(const std::string& section, const std::string& filter) const {
        if (filter.empty()) return true;

        std::string lowerSection = section;
        std::transform(lowerSection.begin(), lowerSection.end(), lowerSection.begin(), ::tolower);

        std::string lowerFilter = filter;
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

        return lowerSection.find(lowerFilter) != std::string::npos;
    }

    void FontSettingsComponent::RenderActionButtons() {
        if (ImGui::Button("Apply Font")) {
            ApplyFont();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Font List")) {
            RefreshFontList();
        }
    }

    void FontSettingsComponent::RenderUI() {
        EnsureInitialized();

        ImGui::TextUnformatted("Font Family");
        if (ImGui::BeginCombo("##FontFamily", selectedFontName.c_str())) {
            for (const auto& entry : availableFonts) {
                bool isSelected = (entry.name == selectedFontName);
                if (ImGui::Selectable(entry.name.c_str(), isSelected)) {
                    if (selectedFontName != entry.name) {
                        selectedFontName = entry.name;
                        hasChanges = true;
                        ApplyFont();
                    }
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TextUnformatted("Global Scale");
        float scaleValue = m_globalFontScale;
        if (ImGui::SliderFloat("##GlobalFontScale", &scaleValue, 0.5f, 2.5f, "%.2fx")) {
            m_globalFontScale = scaleValue;
            hasChanges = true;
            if (imguiContext) {
                ImGui::SetCurrentContext(imguiContext);
                ImGui::GetIO().FontGlobalScale = m_globalFontScale;
            }
        }

        ImGui::Spacing();
        RenderActionButtons();
    }

    void FontSettingsComponent::RenderFilteredUI(const std::string& filter) {
        EnsureInitialized();

        bool showAll = filter.empty();

        if (showAll || FilterPass("Font Family", filter)) {
            ImGui::TextUnformatted("Font Family");
            if (ImGui::BeginCombo("##FontFamilyFiltered", selectedFontName.c_str())) {
                for (const auto& entry : availableFonts) {
                    bool isSelected = (entry.name == selectedFontName);
                    if (ImGui::Selectable(entry.name.c_str(), isSelected)) {
                        if (selectedFontName != entry.name) {
                            selectedFontName = entry.name;
                            hasChanges = true;
                            ApplyFont();
                        }
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        if (showAll || FilterPass("Global Scale", filter)) {
            ImGui::TextUnformatted("Global Scale");
            float scaleValue = m_globalFontScale;
            if (ImGui::SliderFloat("##GlobalFontScaleFiltered", &scaleValue, 0.5f, 2.5f, "%.2fx")) {
                m_globalFontScale = scaleValue;
                hasChanges = true;
                if (imguiContext) {
                    ImGui::SetCurrentContext(imguiContext);
                    ImGui::GetIO().FontGlobalScale = m_globalFontScale;
                }
            }
        }

        if (showAll) {
            ImGui::Spacing();
            RenderActionButtons();
        }
    }

    void FontSettingsComponent::ApplyFont() {
        EnsureInitialized();
        std::string path;
        for (const auto& entry : availableFonts) {
            if (entry.name == selectedFontName) {
                path = entry.path;
                break;
            }
        }
        std::cerr << "[Font] ApplyFont: selected = '" << selectedFontName
            << "', resolved path = '" << path << "'\n";
        pendingFontPath = path;
        fontsNeedRebuild = true;
        hasChanges = true;
        std::cerr << "[Font] fontsNeedRebuild set to true\n";

        CheckAndRebuildFonts();
    }

    void FontSettingsComponent::RefreshFontList() {
        fontsScanned = false;
        ScanFontsDirectory();
    }

    void FontSettingsComponent::CheckAndRebuildFonts() {
        std::cerr << "[Font] CheckAndRebuildFonts: flag = " << fontsNeedRebuild
            << ", context = " << (imguiContext ? "set" : "null") << "\n";
        if (!fontsNeedRebuild) return;
        if (!imguiContext) {
            std::cerr << "[Font] CheckAndRebuild: context is null, deferring\n";
            fontsNeedRebuild = true;
            return;
        }
        ImGuiContext* previousContext = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(imguiContext);
        RebuildFonts();
        ImGui::SetCurrentContext(previousContext);
        fontsNeedRebuild = false;
    }

    void FontSettingsComponent::RebuildFonts() {
        if (!imguiContext) {
            std::cerr << "[Font] Rebuild skipped: no context\n";
            fontsNeedRebuild = true;
            return;
        }

        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();

        std::cerr << "[Font] Starting rebuild. pendingPath = '" << pendingFontPath << "'\n";

        ImGui_ImplOpenGL3_DestroyDeviceObjects();

        io.Fonts->Clear();

        ImFont* newFont = nullptr;
        const float fontSize = 16.0f;

        if (!pendingFontPath.empty()) {
            bool exists = std::filesystem::exists(pendingFontPath);
            std::cerr << "[Font] File exists? " << exists << "\n";
            if (exists) {
                ImFontConfig config;
                config.OversampleH = 3;
                config.OversampleV = 1;
                config.PixelSnapH = true;
                newFont = io.Fonts->AddFontFromFileTTF(pendingFontPath.c_str(), fontSize, &config);
                if (newFont)
                    std::cerr << "[Font] Successfully loaded font file.\n";
                else
                    std::cerr << "[Font] AddFontFromFileTTF returned nullptr.\n";
            }
            else {
                std::cerr << "[Font] File does not exist.\n";
            }
        }

        if (!newFont) {
            std::cerr << "[Font] Using default font.\n";
            newFont = io.Fonts->AddFontDefault();
        }

        if (!io.Fonts->Build()) {
            std::cerr << "[Font] Font atlas build failed!\n";
            io.Fonts->Clear();
            newFont = io.Fonts->AddFontDefault();
            io.Fonts->Build();
        }

        std::cerr << "[Font] Atlas built with " << io.Fonts->Fonts.Size << " fonts.\n";

        if (!ImGui_ImplOpenGL3_CreateDeviceObjects()) {
            std::cerr << "[Font] CreateDeviceObjects FAILED!\n";
            fontsNeedRebuild = true;
            return;
        }

        io.FontDefault = newFont;
        io.FontGlobalScale = m_globalFontScale;

        std::cerr << "[Font] Rebuild complete. FontDefault set to: "
            << (newFont->GetDebugName() ? newFont->GetDebugName() : "unknown") << "\n";

        if (s_fontRebuildCallback)
            s_fontRebuildCallback();

        fontsNeedRebuild = false;
    }

    bool FontSettingsComponent::SaveSettings() {
        try {
            std::filesystem::path settingsPath = "../data/settings/fonts.json";
            std::filesystem::path dir = settingsPath.parent_path();
            if (!dir.empty() && !std::filesystem::exists(dir)) {
                std::filesystem::create_directories(dir);
            }

            nlohmann::json j;
            j["fontName"] = selectedFontName;
            j["globalScale"] = m_globalFontScale;

            std::ofstream file(settingsPath);
            if (!file.is_open()) {
                return false;
            }
            file << j.dump(4);

            CreateBackup();
            hasChanges = false;
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool FontSettingsComponent::LoadSettings() {
        EnsureInitialized();

        const std::string settingsPath = "../data/settings/fonts.json";
        std::ifstream file(settingsPath);

        if (!file.is_open()) {
            selectedFontName = "Default";
            m_globalFontScale = 1.0f;
            ApplyFont();
            hasChanges = false;
            return true;
        }

        try {
            nlohmann::json j;
            file >> j;

            selectedFontName = j.value("fontName", std::string("Default"));
            m_globalFontScale = j.value("globalScale", 1.0f);

            m_globalFontScale = std::clamp(m_globalFontScale, 0.5f, 2.5f);

            bool found = false;
            for (const auto& entry : availableFonts) {
                if (entry.name == selectedFontName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                selectedFontName = "Default";
            }

            ApplyFont();
            hasChanges = false;
            return true;
        }
        catch (...) {
            selectedFontName = "Default";
            m_globalFontScale = 1.0f;
            ApplyFont();
            return false;
        }
    }

    void FontSettingsComponent::ResetToDefaults() {
        selectedFontName = "Default";
        m_globalFontScale = 1.0f;
        ApplyFont();
        hasChanges = true;
    }

    void FontSettingsComponent::CreateBackup() {
        backupSelectedFontName = selectedFontName;
        backupGlobalFontScale = m_globalFontScale;
    }

    void FontSettingsComponent::RestoreFromBackup() {
        selectedFontName = backupSelectedFontName;
        m_globalFontScale = backupGlobalFontScale;
        ApplyFont();
        hasChanges = false;
    }

    void FontSettingsComponent::SetFontRebuildCallback(FontRebuildCallback callback) {
        s_fontRebuildCallback = callback;
    }

} // namespace ECS