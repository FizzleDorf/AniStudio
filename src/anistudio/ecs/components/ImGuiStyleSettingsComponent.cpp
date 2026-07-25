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
                if (dir.back() != '/' && dir.back() != '\\') dir += '/';
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

}