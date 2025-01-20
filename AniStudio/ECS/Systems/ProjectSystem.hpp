#pragma once
#include "ECS.h"
#include "Events/Events.hpp"
#include "FileSystem.hpp"
#include "ImageComponent.hpp"
#include "LoadedMedia.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace ECS {

class ProjectSystem : public BaseSystem {

public:
    ProjectSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) { 
        sysName = "Project_System"; 
        AddComponentSignature<ImageComponent>();
        AddComponentSignature<FileComponent>();
    }
    ~ProjectSystem() {}
    
    void Start() override { fileSystem = mgr.GetSystem<FileSystem>();}

    bool SaveProject(const std::string &projectPath) {
        if (!fileSystem)
            return false;

        nlohmann::json projectData;
        projectData["version"] = "1.0.0";
        projectData["entities"] = nlohmann::json::array();

        // Save filesystem configuration
        fileSystem->SetProjectPaths(projectPath);

        // Save entities
        for (const auto &entity : entities) {
            projectData["entities"].push_back(SerializeEntity(entity));
        }

        // Save loaded media state
        projectData["loadedMedia"] = SerializeLoadedMedia();

        // Write project file
        std::string projectFile = projectPath + "/project.json";
        std::ofstream file(projectFile);
        if (!file.is_open()) {
            return false;
        }
        file << projectData.dump(4);
        return true;
    }

    bool LoadProject(const std::string &projectPath) {
        if (!fileSystem)
            return false;

        std::string projectFile = projectPath + "/project.json";
        std::ifstream file(projectFile);
        if (!file.is_open()) {
            return false;
        }

        try {
            nlohmann::json projectData;
            file >> projectData;

            // Reset current state
            mgr.Reset();

            // Set up filesystem for the project
            fileSystem->SetProjectPaths(projectPath);

            // Load entities
            for (const auto &entityData : projectData["entities"]) {
                DeserializeEntity(entityData);
            }

            // Load media state
            if (projectData.contains("loadedMedia")) {
                DeserializeLoadedMedia(projectData["loadedMedia"]);
            }

            return true;
        } catch (const std::exception &e) {
            std::cerr << "Error loading project: " << e.what() << std::endl;
            return false;
        }
    }

    bool CreateNewProject(const std::string &projectPath) {
        if (!fileSystem)
            return false;

        try {
            // Set up project directory structure
            fileSystem->SetProjectPaths(projectPath);

            // Save initial empty project state
            return SaveProject(projectPath);
        } catch (const std::exception &e) {
            std::cerr << "Error creating project: " << e.what() << std::endl;
            return false;
        }
    }

private:
    std::shared_ptr<FileSystem> fileSystem;

    nlohmann::json SerializeEntity(EntityID entity) {
        nlohmann::json entityData;
        entityData["id"] = entity;
        entityData["components"] = nlohmann::json::array();

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
        mediaData["images"] = nlohmann::json::array();
        for (const auto &image : loadedMedia.GetImages()) {
            mediaData["images"].push_back(image.Serialize());
        }
        return mediaData;
    }

    void DeserializeLoadedMedia(const nlohmann::json &mediaData) {
        while (!loadedMedia.GetImages().empty()) {
            loadedMedia.RemoveImage(0);
        }

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