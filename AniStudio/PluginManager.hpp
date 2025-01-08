// PluginManager.hpp
#pragma once
#include "ECS.h"
#include "pch.h"
#include <filesystem>
#include <windows.h> // Add Windows header for DLL functions

namespace ANI {

struct PluginInfo {
    std::string name;         // Plugin name (filename without extension)
    std::string path;         // Full path to DLL
    bool isActive = false;    // Whether plugin is currently loaded
    HMODULE handle = nullptr; // DLL handle
};

class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager() {
        // Unload all plugins
        for (auto &plugin : plugins) {
            if (plugin.second.isActive) {
                UnloadPlugin(plugin.second);
            }
        }
    }

    void ScanPlugins(const std::filesystem::path &pluginDir) {
        plugins.clear();
        try {
            for (const auto &entry : std::filesystem::directory_iterator(pluginDir)) {
                if (entry.path().extension() == ".dll") {
                    PluginInfo info;
                    info.name = entry.path().stem().string();
                    info.path = entry.path().string();
                    info.isActive = false;
                    info.handle = nullptr;
                    plugins[info.name] = info;
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error scanning plugins: " << e.what() << std::endl;
        }
    }

    bool LoadPlugin(const std::string &name) {
        auto it = plugins.find(name);
        if (it == plugins.end() || it->second.isActive)
            return false;

        auto &info = it->second;
        try {
            info.handle = LoadLibraryA(info.path.c_str());
            if (!info.handle)
                return false;

            // Get plugin init function
            using InitFunc = void (*)(ECS::EntityManager &);
            auto initFunc = (InitFunc)GetProcAddress(info.handle, "InitializePlugin");

            if (initFunc) {
                initFunc(ECS::mgr);
                info.isActive = true;
                return true;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error loading plugin " << name << ": " << e.what() << std::endl;
        }
        return false;
    }

    void UnloadPlugin(PluginInfo &info) {
        if (!info.isActive)
            return;

        if (info.handle) {
            FreeLibrary(info.handle);
            info.handle = nullptr;
        }
        info.isActive = false;
    }

    const std::unordered_map<std::string, PluginInfo> &GetPlugins() const { return plugins; }

private:
    std::unordered_map<std::string, PluginInfo> plugins;
};

extern PluginManager pluginMgr;
} // namespace ANI