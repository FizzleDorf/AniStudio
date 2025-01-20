#pragma once
#include "Base/BaseSystem.hpp"
#include "ProjectSystem.hpp"
#include "ImageSystem.hpp"
#include "FileComponent.hpp"

namespace ECS {

    class FileSystem : public BaseSystem {
public:
    FileSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) { 
        sysName = "Filesystem_System"; 
    }

    void Start() override {
        AddComponentSignature<FileComponent>();
        EntityID fsEntity = mgr.AddNewEntity();
        mgr.AddComponent<FileComponent>(fsEntity);
        auto &fs = mgr.GetComponent<FileComponent>(fsEntity);
        // Initialize with core paths
        fs.AddPath("dataPath", "../data/defaults", "", false);
        fs.AddPath("plugins", "../plugins", "", false);
        fs.AddPath("defaultProject", "../projects/new_project", "", false);
        fs.AddPath("defaultModels", "../models", "", false);

        // Add default model paths
        fs.AddPath("checkpoints", "../models/checkpoints", "", true);
        fs.AddPath("encoders", "../models/clip", "", true);
        fs.AddPath("vae", "../models/vae", "", true);
        fs.AddPath("unet", "../models/unet", "", true);
        fs.AddPath("lora", "../models/loras", "", true);
        fs.AddPath("controlnet", "../models/controlnet", "", true);
        fs.AddPath("upscale", "../models/upscale_models", "", true);

        // Create necessary directories
        for (const auto &path : fs.paths) {
            std::filesystem::create_directories(path.path);
        }

        // Load saved paths from configuration
        LoadPathsFromConfig();
    }

    void SavePathsToConfig() {
        if (entities.empty())
            return;

        auto &fs = mgr.GetComponent<FileComponent>(*entities.begin());
        nlohmann::json config;
        config["paths"] = fs.Serialize();

        std::filesystem::path configPath = fs.GetPath("dataPath") + "/paths.json";
        std::ofstream file(configPath);
        if (file.is_open()) {
            file << config.dump(4);
        }
    }

    void LoadPathsFromConfig() {
        if (entities.empty())
            return;

        auto &fs = mgr.GetComponent<FileComponent>(*entities.begin());
        std::filesystem::path configPath = fs.GetPath("dataPath") + "/paths.json";

        std::ifstream file(configPath);
        if (file.is_open()) {
            try {
                nlohmann::json config;
                file >> config;
                if (config.contains("paths")) {
                    fs.Deserialize(config["paths"]);
                }
            } catch (const std::exception &e) {
                std::cerr << "Error loading paths config: " << e.what() << std::endl;
            }
        }
    }

    // Method for plugins to register their paths
    void RegisterPluginPaths(const std::string &pluginId,
                             const std::vector<std::pair<std::string, std::string>> &pluginPaths,
                             bool areModelPaths = false) {
        if (entities.empty())
            return;

        auto &fs = mgr.GetComponent<FileComponent>(*entities.begin());
        for (const auto &[name, path] : pluginPaths) {
            fs.AddPath(name, path, pluginId, areModelPaths);
            std::filesystem::create_directories(path);
        }
        SavePathsToConfig();
    }

    // Method to get a path by name
    std::string GetPath(const std::string &name) {
        if (entities.empty())
            return "";
        auto &fs = mgr.GetComponent<FileComponent>(*entities.begin());
        return fs.GetPath(name);
    }

    // Method to set project paths when creating/loading a project
    void SetProjectPaths(const std::string &projectRoot) {
        if (entities.empty())
            return;

        auto &fs = mgr.GetComponent<FileComponent>(*entities.begin());
        fs.projectRoot = projectRoot;
        fs.imagesDir = projectRoot + "/images";
        fs.cacheDir = projectRoot + "/cache";

        // Create project directories
        std::filesystem::create_directories(fs.imagesDir);
        std::filesystem::create_directories(fs.cacheDir);
    }
};

} // namespace ECS