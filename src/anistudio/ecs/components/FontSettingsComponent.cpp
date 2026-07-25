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

    void FontSettingsComponent::ApplyFont() {
        EnsureInitialized();
        std::string path;
        for (const auto& entry : availableFonts) {
            if (entry.name == selectedFontName) {
                path = entry.path;
                break;
            }
        }
        pendingFontPath = path;
        fontsNeedRebuild = true;
        hasChanges = true;
    }

    void FontSettingsComponent::RefreshFontList() {
        fontsScanned = false;
        ScanFontsDirectory();
    }

    void FontSettingsComponent::CheckAndRebuildFonts() {
        if (!fontsNeedRebuild) return;
        if (!imguiContext) {
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
            fontsNeedRebuild = true;
            return;
        }

        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();

        ImGui_ImplOpenGL3_DestroyDeviceObjects();

        io.Fonts->Clear();

        ImFont* newFont = nullptr;
        const float fontSize = 16.0f;

        if (!pendingFontPath.empty() && std::filesystem::exists(pendingFontPath)) {
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 1;
            config.PixelSnapH = true;
            newFont = io.Fonts->AddFontFromFileTTF(pendingFontPath.c_str(), fontSize, &config);
        }

        if (!newFont) {
            newFont = io.Fonts->AddFontDefault();
        }

        if (!io.Fonts->Build()) {
            io.Fonts->Clear();
            newFont = io.Fonts->AddFontDefault();
            io.Fonts->Build();
        }

        if (!ImGui_ImplOpenGL3_CreateDeviceObjects()) {
            fontsNeedRebuild = true;
            return;
        }

        io.FontDefault = newFont;
        io.FontGlobalScale = m_globalFontScale;

        if (s_fontRebuildCallback) {
            s_fontRebuildCallback();
        }

        fontsNeedRebuild = false;
    }

    bool FontSettingsComponent::SaveSettings() {
        try {
            std::filesystem::path settingsPath = "./data/settings/fonts.json";
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
        const std::string settingsPath = "./data/settings/fonts.json";
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

}