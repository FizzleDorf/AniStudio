#pragma once
#include "PluginLoader.hpp"
#include "filepaths.hpp"
#include <filesystem>
#include <map>
#include <string>

namespace Plugin {

class PluginManager {
public:
    PluginManager(ECS::EntityManager &entityMgr, GUI::ViewManager &viewMgr)
        : entityManager(&entityMgr), viewManager(&viewMgr), appVersion_({1, 0, 0}) {}

    void Init() {
        pluginDirectory_ = filePaths.pluginPath;
        ScanPlugins();
    }

    void ScanPlugins() {
        try {
            for (const auto &entry : std::filesystem::directory_iterator(pluginDirectory_)) {
                if (!IsPluginFile(entry.path().string()))
                    continue;

                const std::string pluginName = entry.path().filename().string();
                if (pluginLoaders_.find(pluginName) == pluginLoaders_.end()) {
                    std::cout << "Found new plugin: " << pluginName << std::endl;
                    pluginLoaders_[pluginName] = std::make_unique<PluginLoader>(entry.path().string());
                }
            }
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Failed to scan plugins: " << e.what() << std::endl;
        }
    }

    bool LoadPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            return false;
        }
        return LoadPluginWithDependencies(name);
    }

    bool StartPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            return false;
        }
        return it->second->Start();
    }

    void StopPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it != pluginLoaders_.end()) {
            it->second->Stop();
        }
    }

    bool UnloadPlugin(const std::string &name) {
        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            return false;
        }
        it->second->Unload();
        return true;
    }

    void Update(float dt) {
        for (auto &[name, loader] : pluginLoaders_) {
            if (loader) {
                loader->Update(dt);
            }
        }
    }

    const std::map<std::string, std::unique_ptr<PluginLoader>> &GetPlugins() const { return pluginLoaders_; }

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

    bool LoadPluginRecursive(const std::string &name, std::set<std::string> &loaded) {
        if (loaded.find(name) != loaded.end()) {
            return true;
        }

        auto it = pluginLoaders_.find(name);
        if (it == pluginLoaders_.end()) {
            return false;
        }

        auto *plugin = it->second->Get();
        if (plugin) {
            for (const auto &dep : plugin->GetDependencies()) {
                if (!LoadPluginRecursive(dep, loaded)) {
                    return false;
                }
            }
        }

        if (!it->second->Load(appVersion_, entityManager, viewManager)) {
            return false;
        }

        loaded.insert(name);
        return true;
    }

    std::string pluginDirectory_;
    Version appVersion_;
    std::map<std::string, std::unique_ptr<PluginLoader>> pluginLoaders_;
    ECS::EntityManager *entityManager;
    GUI::ViewManager *viewManager;
};

} // namespace Plugin