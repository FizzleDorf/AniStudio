#pragma once
#include "BaseComponent.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace ANI {

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
        ProjectComponent() {
            compName = "ProjectComponent";
        }

        bool isOpen = false;
        std::string currentProjectPath;
        ProjectSettings settings;

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j["isOpen"] = isOpen;
            j["currentProjectPath"] = currentProjectPath;
            j["settings"] = settings.Serialize();
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains("isOpen")) isOpen = j["isOpen"];
            if (j.contains("currentProjectPath")) currentProjectPath = j["currentProjectPath"];
            if (j.contains("settings")) settings.Deserialize(j["settings"]);
        }
    };

} // namespace ANI