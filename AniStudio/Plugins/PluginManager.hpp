#pragma once
#include "PluginLoader.hpp"
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>
#include <filepaths.hpp>

namespace Plugin {

class PluginError : public std::runtime_error {
public:
    explicit PluginError(const std::string &msg) : std::runtime_error(msg) {}
};

class PluginManager {
public:
    PluginManager() {}

    void Init() { 
        pluginDirectory_ = filePaths.pluginPath; 
        appVersion_ = {1,0,0};
    }

    void ScanPlugins() {
        try {
            pluginLoaders_.clear();
            for (const auto &entry : std::filesystem::directory_iterator(pluginDirectory_)) {
                if (entry.is_regular_file() && IsPluginFile(entry.path().string())) {
                    const std::string pluginName = entry.path().filename().string();
                    pluginLoaders_.emplace(pluginName, PluginLoader(entry.path().string()));
                }
            }
        } catch (const std::filesystem::filesystem_error &e) {
            throw PluginError("Failed to scan plugins: " + std::string(e.what()));
        }
    }

    bool LoadPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            throw PluginError("Plugin not found: " + name);
        }
        return LoadPluginWithDependencies(name);
    }

    bool StartPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            throw PluginError("Plugin not found: " + name);
        }
        return it->second.Start();
    }

    void StopPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it != pluginLoaders_.end()) {
            it->second.Stop();
        }
    }

    bool UnloadPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it != pluginLoaders_.end() && it->second.IsLoaded()) {
            it->second.Unload();
            return true;
        }
        return false;
    }

    void Update(float dt) {
        for (auto &[name, loader] : pluginLoaders_) {
            loader.Update(dt);
        }
    }

    const std::map<std::string, PluginLoader> &GetPlugins() const { return pluginLoaders_; }

private:
    bool IsPluginFile(const std::string &path) const {
#ifdef _WIN32
        return path.length() > 4 && path.compare(path.length() - 4, 4, ".dll") == 0;
#else
        return path.length() > 3 && path.compare(path.length() - 3, 3, ".so") == 0;
#endif
    }

    bool LoadPluginWithDependencies(const std::string &name) {
        std::set<std::string> loaded;
        return LoadPluginRecursive(name, loaded);
    }

    bool LoadPluginRecursive(const std::string &name,
                             std::set<std::string> &loaded) {
        // Check for circular dependencies
        if (loaded.find(name) != loaded.end()) {
            return true; // Already loaded
        }

        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            throw PluginError("Plugin not found: " + name);
        }

        // Load dependencies first
        auto *plugin = it->second.Get();
        if (plugin) {
            for (const auto &dep : plugin->GetDependencies()) {
                if (!LoadPluginRecursive(dep, loaded)) {
                    return false;
                }
            }
        }

        // Load the plugin itself
        if (!it->second.Load(appVersion_)) {
            return false;
        }

        loaded.insert(name);
        return true;
    }

    std::string pluginDirectory_;
    Version appVersion_;
    std::map<std::string, PluginLoader> pluginLoaders_;
};
} // namespace Plugin