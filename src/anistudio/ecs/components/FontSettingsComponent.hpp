#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ECS {

    class FontSettingsComponent : public BaseComponent {
    public:
        FontSettingsComponent() = default;

        const char* GetCompName() const override { return "FontSettingsComponent"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Font Settings"},
                {"type", "object"},
                {"propertyOrder", { "m_globalFontScale" }},
                {"properties", {
                    {"m_globalFontScale", {
                        {"type", "number"},
                        {"title", "Global Font Scale"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.5f}, {"max", 2.5f}, {"format", "%.2fx"}}}
                    }}
                }}
            };
            return j;
        }

        std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() override {
            return {
                {"m_globalFontScale", &m_globalFontScale}
            };
        }

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;
        void ResetToDefaults();

        struct FontEntry {
            std::string name;
            std::string path;
        };

        std::vector<FontEntry> availableFonts;
        std::vector<FontEntry> availableIconFonts;
        std::string selectedFontName = "Default";
        std::string selectedIconFontName;
        float m_globalFontScale = 1.0f;
    };

}