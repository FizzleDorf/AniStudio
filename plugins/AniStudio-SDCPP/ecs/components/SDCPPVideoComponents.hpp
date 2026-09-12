#pragma once

#include "BaseComponent.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace ECS {

    struct RefVideoComponent : public BaseComponent {
        std::vector<std::string> videoPaths;

        RefVideoComponent() = default;

        const char* GetCompName() const override { return "RefVideo"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
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
            return j;
        }

        RefVideoComponent(const RefVideoComponent& other)
            : BaseComponent(other)
            , videoPaths(other.videoPaths) {
        }

        RefVideoComponent& operator=(const RefVideoComponent& other) {
            if (this != &other) {
                videoPaths = other.videoPaths;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {};
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"videoPaths", videoPaths}
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

            if (componentData.contains("videoPaths") && componentData["videoPaths"].is_array()) {
                videoPaths = componentData["videoPaths"].get<std::vector<std::string>>();
            }
        }
    };

} // namespace ECS