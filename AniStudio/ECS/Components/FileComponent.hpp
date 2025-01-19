#pragma once
#include "Base/BaseComponent.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace ECS {

struct PathEntry {
    std::string name;     // Identifier for the path
    std::string path;     // Actual filesystem path
    std::string pluginId; // ID of plugin that added this path (empty for core paths)
    bool isModelPath;     // Whether this is a model directory path

    nlohmann::json Serialize() const {
        nlohmann::json j;
        j["name"] = name;
        j["path"] = path;
        j["pluginId"] = pluginId;
        j["isModelPath"] = isModelPath;
        return j;
    }

    void Deserialize(const nlohmann::json &j) {
        name = j["name"];
        path = j["path"];
        pluginId = j["pluginId"];
        isModelPath = j["isModelPath"];
    }
};

// Component to store filesystem paths
struct FileComponent : public BaseComponent {
    std::vector<PathEntry> paths;

    // Core project paths
    std::string projectRoot;
    std::string imagesDir;
    std::string cacheDir;

    FileComponent() { compName = "Filesystem_Component"; }

    void AddPath(const std::string &name, const std::string &path, const std::string &pluginId = "",
                 bool isModelPath = false) {
        paths.push_back({name, path, pluginId, isModelPath});
    }

    std::string GetPath(const std::string &name) const {
        auto it =
            std::find_if(paths.begin(), paths.end(), [&name](const PathEntry &entry) { return entry.name == name; });
        return it != paths.end() ? it->path : "";
    }

    void RemovePluginPaths(const std::string &pluginId) {
        paths.erase(std::remove_if(paths.begin(), paths.end(),
                                   [&pluginId](const PathEntry &entry) { return entry.pluginId == pluginId; }),
                    paths.end());
    }

    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseComponent::Serialize();
        j["projectRoot"] = projectRoot;
        j["imagesDir"] = imagesDir;
        j["cacheDir"] = cacheDir;

        j["paths"] = nlohmann::json::array();
        for (const auto &path : paths) {
            j["paths"].push_back(path.Serialize());
        }
        return j;
    }

    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        projectRoot = j["projectRoot"];
        imagesDir = j["imagesDir"];
        cacheDir = j["cacheDir"];

        paths.clear();
        for (const auto &pathData : j["paths"]) {
            PathEntry entry;
            entry.Deserialize(pathData);
            paths.push_back(entry);
        }
    }
};
} // namespace ECS