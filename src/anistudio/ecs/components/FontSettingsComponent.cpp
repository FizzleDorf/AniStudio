#include "FontSettingsComponent.hpp"

namespace ECS {

    nlohmann::json FontSettingsComponent::Serialize() const {
        nlohmann::json j;
        j["fontName"] = selectedFontName;
        j["iconFontName"] = selectedIconFontName;
        j["globalScale"] = m_globalFontScale;
        return j;
    }

    void FontSettingsComponent::Deserialize(const nlohmann::json& j) {
        selectedFontName = j.value("fontName", std::string("Default"));
        selectedIconFontName = j.value("iconFontName", std::string(""));
        m_globalFontScale = j.value("globalScale", 1.0f);
        if (m_globalFontScale < 0.5f) m_globalFontScale = 0.5f;
        if (m_globalFontScale > 2.5f) m_globalFontScale = 2.5f;

        bool found = false;
        for (const auto& entry : availableFonts) {
            if (entry.name == selectedFontName) { found = true; break; }
        }
        if (!found) selectedFontName = "Default";

        bool iconFound = false;
        for (const auto& entry : availableIconFonts) {
            if (entry.name == selectedIconFontName) { iconFound = true; break; }
        }
        if (!iconFound && !availableIconFonts.empty()) {
            selectedIconFontName = availableIconFonts[0].name;
        }
    }

    void FontSettingsComponent::ResetToDefaults() {
        selectedFontName = "Default";
        selectedIconFontName.clear();
        m_globalFontScale = 1.0f;
        if (!availableIconFonts.empty()) {
            selectedIconFontName = availableIconFonts[0].name;
        }
    }

}