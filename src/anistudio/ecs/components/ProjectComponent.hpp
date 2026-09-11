#pragma once
#include "BaseComponent.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace ECS {

    struct ProjectSettings {
        std::string projectName = "Untitled Project";
        std::string projectVersion = "1.0.0";
        std::string projectDescription;
        std::string createdBy;
        std::string createdDate;
        std::string lastModified;

        nlohmann::json Serialize() const;
        void Deserialize(const nlohmann::json& j);
    };

    class ProjectComponent : public ECS::BaseComponent {
    public:
        bool isOpen = false;
        std::string currentProjectPath;
        ProjectSettings settings;

        ProjectComponent() = default;

        const char* GetCompName() const override { return "ProjectComponent"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = nlohmann::json::object();
            return j;
        }

        ProjectComponent(const ProjectComponent& other) : BaseComponent(other) {
            isOpen = other.isOpen;
            currentProjectPath = other.currentProjectPath;
            settings = other.settings;
        }

        ProjectComponent& operator=(const ProjectComponent& other) {
            if (this != &other) {
                isOpen = other.isOpen;
                currentProjectPath = other.currentProjectPath;
                settings = other.settings;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"isOpen", isOpen},
                {"currentProjectPath", currentProjectPath},
                {"settings", settings.Serialize()}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("isOpen")) isOpen = componentData["isOpen"];
            if (componentData.contains("currentProjectPath")) currentProjectPath = componentData["currentProjectPath"];
            if (componentData.contains("settings")) settings.Deserialize(componentData["settings"]);
        }
    };

} // namespace ECS