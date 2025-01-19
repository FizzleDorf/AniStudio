#pragma once
#include "Components/ImageComponent.hpp"
#include "ECS.h"
#include "Events/Events.hpp"
#include "FilePaths.hpp"
#include "LoadedMedia.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace ECS {

class ProjectSystem : public BaseSystem {
public:
    ProjectSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) { sysName = "Project_System"; }

    void Start() override {
        AddComponentSignature<ImageComponent>();
    }

    bool SaveProject(const std::string &projectPath) {
        nlohmann::json projectData;
        projectData["version"] = "1.0.0";
        projectData["entities"] = nlohmann::json::array();
        for (const auto &entity : entities) {
            projectData["entities"].push_back(SerializeEntity(entity));
        }
        projectData["loadedMedia"] = SerializeLoadedMedia();
        std::filesystem::create_directories(projectPath);
        std::string projectFile = projectPath + "/project.json";
        std::ofstream file(projectFile);
        if (!file.is_open()) {
            return false;
        }
        file << projectData.dump(4);
        return true;
    }

    bool LoadProject(const std::string &projectPath) {
        std::string projectFile = projectPath + "/project.json";
        std::ifstream file(projectFile);
        if (!file.is_open()) {
            return false;
        }

        try {
            nlohmann::json projectData;
            file >> projectData;
            mgr.Reset();
            for (const auto &entityData : projectData["entities"]) {
                DeserializeEntity(entityData);
            }
            if (projectData.contains("loadedMedia")) {
                DeserializeLoadedMedia(projectData["loadedMedia"]);
            }
            filePaths.lastOpenProjectPath = projectPath;
            filePaths.SaveFilepathDefaults();

            return true;
        } catch (const std::exception &e) {
            std::cerr << "Error loading project: " << e.what() << std::endl;
            return false;
        }
    }

    bool CreateNewProject(const std::string &projectPath) {       
        if (!std::filesystem::create_directories(projectPath)) {
            return false;
        }

        // Create subdirectories
        std::filesystem::create_directories(projectPath + "/images");
        std::filesystem::create_directories(projectPath + "/cache");

        mgr.Reset();

        return SaveProject(projectPath);
    }

private:
    nlohmann::json SerializeEntity(EntityID entity) {
        nlohmann::json entityData;
        entityData["id"] = entity;
        entityData["components"] = nlohmann::json::array();

        // Serialize ImageComponent if present
        if (mgr.HasComponent<ImageComponent>(entity)) {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entity);
            nlohmann::json componentData;
            componentData["type"] = "ImageComponent";
            componentData["data"] = imageComp.Serialize();
            entityData["components"].push_back(componentData);
        }

        return entityData;
    }

    void DeserializeEntity(const nlohmann::json &entityData) {
        EntityID entity = mgr.AddNewEntity();

        for (const auto &componentData : entityData["components"]) {
            std::string type = componentData["type"];

            if (type == "ImageComponent") {
                ImageComponent imageComp;
                imageComp.Deserialize(componentData["data"]);
                mgr.AddComponent<ImageComponent>(entity, imageComp);
            }
        }
    }

    nlohmann::json SerializeLoadedMedia() {
        nlohmann::json mediaData;

        // Serialize loaded images
        mediaData["images"] = nlohmann::json::array();
        for (const auto &image : loadedMedia.GetImages()) {
            mediaData["images"].push_back(image.Serialize());
        }

        return mediaData;
    }

    void DeserializeLoadedMedia(const nlohmann::json &mediaData) {
        // Clear existing loaded media
        while (!loadedMedia.GetImages().empty()) {
            loadedMedia.RemoveImage(0);
        }

        // Load images
        if (mediaData.contains("images")) {
            for (const auto &imageData : mediaData["images"]) {
                ImageComponent image;
                image.Deserialize(imageData);
                loadedMedia.AddImage(image);
            }
        }
    }
};

} // namespace ECS