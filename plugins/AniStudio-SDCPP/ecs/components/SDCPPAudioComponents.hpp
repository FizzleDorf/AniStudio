#pragma once

#include "BaseComponent.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace ECS {

    struct RefAudioComponent : public BaseComponent {
        RefAudioComponent() {
            compName = "RefAudio";

            schema = {
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
        }

        std::vector<std::string> audioPaths;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {};
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[compName]["audioPaths"] = audioPaths;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);

            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
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

        RefAudioComponent& operator=(const RefAudioComponent& other) {
            if (this != &other) {
                audioPaths = other.audioPaths;
            }
            return *this;
        }
    };

    struct RefVideoAudioComponent : public BaseComponent {
        RefVideoAudioComponent() {
            compName = "RefVideoAudio";

            schema = {
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
        }

        std::vector<std::string> audioPaths;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {};
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[compName]["audioPaths"] = audioPaths;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
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