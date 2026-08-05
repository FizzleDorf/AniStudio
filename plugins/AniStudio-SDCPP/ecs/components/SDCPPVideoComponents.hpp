#pragma once

#include "BaseComponent.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace ECS {

    struct RefVideoComponent : public BaseComponent {
        RefVideoComponent() {
            compName = "RefVideo";

            schema = {
                {"title", "Reference Videos"},
                {"type", "object"},
                {"properties", {
                    {"videoPaths", {
                        {"type", "array"},
                        {"title", "Video Directories"},
                        {"description", "List of directories containing video frames (sorted lexicographically)."},
                        {"items", {
                            {"type", "string"},
                            {"ui:widget", "directory_selector"},
                            {"ui:options", {
                                {"buttonText", "Browse..."},
                                {"resetButtonText", "Clear"},
                                {"browseTooltip", "Select a folder containing frame images"}
                            }}
                        }},
                        {"ui:widget", "list"},
                        {"ui:options", {
                            {"addButtonText", "Add Video"},
                            {"removeButtonText", "Remove"}
                        }}
                    }}
                }}
            };
        }

        std::vector<std::string> videoPaths;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {}; // vector cannot be mapped via simple pointer
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[compName]["videoPaths"] = videoPaths;
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

            if (componentData.contains("videoPaths") && componentData["videoPaths"].is_array()) {
                videoPaths = componentData["videoPaths"].get<std::vector<std::string>>();
            }
        }

        RefVideoComponent& operator=(const RefVideoComponent& other) {
            if (this != &other) {
                videoPaths = other.videoPaths;
            }
            return *this;
        }
    };

} // namespace ECS