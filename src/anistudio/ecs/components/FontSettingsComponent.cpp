#include "FontSettingsComponent.hpp"
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "TextEditorFontUtil.hpp"

namespace ECS {

    FontSettingsComponent::FontRebuildCallback FontSettingsComponent::s_fontRebuildCallback = nullptr;

    void FontSettingsComponent::EnsureInitialized() {
        if (isInitialized) return;
        ScanFontsDirectory();
        ScanIconFontsDirectory();
        if (selectedFontName.empty()) {
            selectedFontName = "Default";
        }
        if (selectedIconFontName.empty() && !availableIconFonts.empty()) {
            selectedIconFontName = availableIconFonts[0].name;
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

    void FontSettingsComponent::ScanIconFontsDirectory() {
        availableIconFonts.clear();
        const std::string iconFontsDir = "assets/icon_fonts";
        try {
            if (std::filesystem::exists(iconFontsDir) && std::filesystem::is_directory(iconFontsDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(iconFontsDir)) {
                    if (!entry.is_regular_file()) continue;
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".ttf" || ext == ".otf") {
                        FontEntry fe;
                        fe.name = entry.path().stem().string();
                        fe.path = std::filesystem::absolute(entry.path()).string();
                        availableIconFonts.push_back(fe);
                    }
                }
            }
        }
        catch (...) {
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
        pendingFontPath = path;

        std::string iconPath;
        for (const auto& entry : availableIconFonts) {
            if (entry.name == selectedIconFontName) {
                iconPath = entry.path;
                break;
            }
        }
        pendingIconFontPath = iconPath;

        fontsNeedRebuild = true;
        hasChanges = true;

        if (imguiContext) {
            CheckAndRebuildFonts();
        }
    }

    void FontSettingsComponent::RefreshFontList() {
        fontsScanned = false;
        ScanFontsDirectory();
        ScanIconFontsDirectory();
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

        TextEditorUtil::clearEditorFontCache();

        ImGui_ImplOpenGL3_DestroyDeviceObjects();

        ImGuiIO& io = ImGui::GetIO();

        io.Fonts->Clear();

        const float fontSize = 16.0f;

        // 1. Load the main font
        ImFont* mainFont = nullptr;
        if (!pendingFontPath.empty() && std::filesystem::exists(pendingFontPath)) {
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 1;
            config.PixelSnapH = true;
            mainFont = io.Fonts->AddFontFromFileTTF(pendingFontPath.c_str(), fontSize, &config);
            std::cout << "[FontSettings] Loaded main font: " << pendingFontPath << std::endl;
        }

        if (!mainFont) {
            mainFont = io.Fonts->AddFontDefault();
            std::cout << "[FontSettings] Using default font" << std::endl;
        }

        // 2. Load the icon font with MergeMode
        if (!pendingIconFontPath.empty() && std::filesystem::exists(pendingIconFontPath)) {
            // IMPORTANT: The glyph ranges MUST be specified for older ImGui versions
            // Font Awesome uses the Private Use Area (PUA) from 0xf000 to 0xf8ff
            static const ImWchar iconRanges[] = {
                0xf000, 0xf8ff,  // Font Awesome 5/6/7 Private Use Area
                0
            };

            ImFontConfig config;
            config.MergeMode = true;
            config.PixelSnapH = true;
            config.OversampleH = 3;
            config.OversampleV = 1;
            config.GlyphMinAdvanceX = fontSize * 0.8f;
            config.GlyphMaxAdvanceX = fontSize * 0.8f;

            io.Fonts->AddFontFromFileTTF(
                pendingIconFontPath.c_str(),
                fontSize,
                &config,
                iconRanges  // This is critical - MUST specify the ranges
            );

            std::cout << "[FontSettings] Loaded icon font: " << pendingIconFontPath << std::endl;
        }
        else {
            std::cout << "[FontSettings] No icon font loaded" << std::endl;
        }

        // 3. Build the font atlas
        if (!io.Fonts->Build()) {
            std::cerr << "[FontSettings] Failed to build font atlas!" << std::endl;
            io.Fonts->Clear();
            mainFont = io.Fonts->AddFontDefault();
            io.Fonts->Build();
        }

        // 4. Recreate device objects
        if (!ImGui_ImplOpenGL3_CreateDeviceObjects()) {
            std::cerr << "[FontSettings] Failed to create OpenGL device objects!" << std::endl;
            fontsNeedRebuild = true;
            return;
        }

        io.FontDefault = mainFont;
        io.FontGlobalScale = m_globalFontScale;

        if (s_fontRebuildCallback) {
            s_fontRebuildCallback();
        }

        fontsNeedRebuild = false;

        std::cout << "[FontSettings] Font rebuild complete!" << std::endl;
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
            j["iconFontName"] = selectedIconFontName;
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
            if (!availableIconFonts.empty()) {
                selectedIconFontName = availableIconFonts[0].name;
            }
            m_globalFontScale = 1.0f;
            ApplyFont();
            hasChanges = false;
            return true;
        }
        try {
            nlohmann::json j;
            file >> j;
            selectedFontName = j.value("fontName", std::string("Default"));
            selectedIconFontName = j.value("iconFontName", std::string(""));
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

            bool iconFound = false;
            for (const auto& entry : availableIconFonts) {
                if (entry.name == selectedIconFontName) {
                    iconFound = true;
                    break;
                }
            }
            if (!iconFound && !availableIconFonts.empty()) {
                selectedIconFontName = availableIconFonts[0].name;
            }

            ApplyFont();
            hasChanges = false;
            return true;
        }
        catch (...) {
            selectedFontName = "Default";
            if (!availableIconFonts.empty()) {
                selectedIconFontName = availableIconFonts[0].name;
            }
            m_globalFontScale = 1.0f;
            ApplyFont();
            return false;
        }
    }

    void FontSettingsComponent::ResetToDefaults() {
        selectedFontName = "Default";
        if (!availableIconFonts.empty()) {
            selectedIconFontName = availableIconFonts[0].name;
        }
        m_globalFontScale = 1.0f;
        ApplyFont();
        hasChanges = true;
    }

    void FontSettingsComponent::CreateBackup() {
        backupSelectedFontName = selectedFontName;
        backupSelectedIconFontName = selectedIconFontName;
        backupGlobalFontScale = m_globalFontScale;
    }

    void FontSettingsComponent::RestoreFromBackup() {
        selectedFontName = backupSelectedFontName;
        selectedIconFontName = backupSelectedIconFontName;
        m_globalFontScale = backupGlobalFontScale;
        ApplyFont();
        hasChanges = false;
    }

    void FontSettingsComponent::SetFontRebuildCallback(FontRebuildCallback callback) {
        s_fontRebuildCallback = callback;
    }

}