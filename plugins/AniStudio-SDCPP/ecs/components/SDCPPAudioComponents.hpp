#pragma once

#include "BaseComponent.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace ECS {

    struct RefAudioComponent : public BaseComponent {
        std::vector<std::string> audioPaths;

        RefAudioComponent() = default;

        const char* GetCompName() const override { return "RefAudio"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Reference Audio"},
                {"type", "object"},
                {"properties", {
                    {"audioPaths", {
                        {"type", "array"},
                        {"title", "Audio Files"},
                        {"description", "List of WAV audio files for reference conditioning."},
                        {"items", {
                            {"type", "string"},
                            {"ui:widget", "file_selector"},
                            {"ui:options", {
                                {"mode", "file"},
                                {"filters", ".wav"},
                                {"filterName", "WAV Audio Files"},
                                {"buttonText", "Browse..."},
                                {"resetButtonText", "Clear"},
                                {"browseTooltip", "Select a WAV audio file"}
                            }}
                        }},
                        {"ui:widget", "list"},
                        {"ui:options", {
                            {"addButtonText", "Add Audio"},
                            {"removeButtonText", "Remove"}
                        }}
                    }}
                }}
            };
            return j;
        }

        RefAudioComponent(const RefAudioComponent& other)
            : BaseComponent(other)
            , audioPaths(other.audioPaths) {
        }

        RefAudioComponent& operator=(const RefAudioComponent& other) {
            if (this != &other) {
                audioPaths = other.audioPaths;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {};
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"audioPaths", audioPaths}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();

            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == key) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("audioPaths") && componentData["audioPaths"].is_array()) {
                audioPaths = componentData["audioPaths"].get<std::vector<std::string>>();
            }
        }
    };

    struct RefVideoAudioComponent : public BaseComponent {
        std::vector<std::string> audioPaths;

        RefVideoAudioComponent() = default;

        const char* GetCompName() const override { return "RefVideoAudio"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Paired Video Audio"},
                {"type", "object"},
                {"properties", {
                    {"audioPaths", {
                        {"type", "array"},
                        {"title", "Audio Files (paired with videos by index)"},
                        {"items", {
                            {"type", "string"},
                            {"ui:widget", "file_selector"},
                            {"ui:options", {
                                {"mode", "file"},
                                {"filters", ".wav"},
                                {"filterName", "WAV Audio Files"}
                            }}
                        }},
                        {"ui:widget", "list"}
                    }}
                }}
            };
            return j;
        }

        RefVideoAudioComponent(const RefVideoAudioComponent& other)
            : BaseComponent(other)
            , audioPaths(other.audioPaths) {
        }

        RefVideoAudioComponent& operator=(const RefVideoAudioComponent& other) {
            if (this != &other) {
                audioPaths = other.audioPaths;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {};
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"audioPaths", audioPaths}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                componentData = j;
            }
            if (componentData.contains("audioPaths") && componentData["audioPaths"].is_array()) {
                audioPaths = componentData["audioPaths"].get<std::vector<std::string>>();
            }
        }
    };

} // namespace ECS