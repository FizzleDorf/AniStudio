#pragma once
#include "pch.h"
#include <filesystem>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#define EXPORT __declspec(dllexport)
#else
#include <dlfcn.h>
#define EXPORT
#endif

namespace ANI {

class DynamicLibrary {
public:
    DynamicLibrary(const std::string &path) {
#ifdef _WIN32
        handle = LoadLibraryA(path.c_str());
#else
        handle = dlopen(path.c_str(), RTLD_NOW);
#endif
        if (!handle) {
            throw std::runtime_error("Failed to load library: " + path);
        }
    }

    ~DynamicLibrary() {
        if (handle) {
#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
        }
    }

    template <typename T>
    T GetFunction(const std::string &name) {
#ifdef _WIN32
        return (T)GetProcAddress((HMODULE)handle, name.c_str());
#else
        return (T)dlsym(handle, name.c_str());
#endif
    }

private:
    void *handle = nullptr;
};

class PluginManager {
public:
    using InitPluginFunc = void (*)(ECS::EntityManager &, GUI::ViewManager &);

    PluginManager() = default;
    ~PluginManager() { StopWatching(); }

    void LoadPlugin(const std::string &path) {
        try {
            auto lib = std::make_shared<DynamicLibrary>(path);
            if (auto initFunc = lib->GetFunction<InitPluginFunc>("InitializePlugin")) {
                initFunc(ECS::mgr, GUI::viewMgr);
                libraries[path] = lib;
            }
        } catch (const std::exception &e) {
            std::cerr << "Failed to load plugin " << path << ": " << e.what() << std::endl;
        }
    }

    void UnloadPlugin(const std::string &path) { libraries.erase(path); }

    void WatchDirectory(const std::string &pluginDir) {
        stopWatching = false;
        watcherThread = std::thread([this, pluginDir]() {
            while (!stopWatching) {
                try {
                    for (const auto &entry : std::filesystem::directory_iterator(pluginDir)) {
                        if (entry.path().extension() == ".dll" || entry.path().extension() == ".so") {
                            auto path = entry.path().string();
                            auto lastWriteTime = std::filesystem::last_write_time(entry.path());

                            if (auto it = lastModifiedTimes.find(path); it != lastModifiedTimes.end()) {
                                if (lastWriteTime > it->second) {
                                    UnloadPlugin(path);
                                    LoadPlugin(path);
                                    lastModifiedTimes[path] = lastWriteTime;
                                }
                            } else {
                                LoadPlugin(path);
                                lastModifiedTimes[path] = lastWriteTime;
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    std::cerr << "Error in plugin watcher: " << e.what() << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    void StopWatching() {
        stopWatching = true;
        if (watcherThread.joinable()) {
            watcherThread.join();
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<DynamicLibrary>> libraries;
    std::unordered_map<std::string, std::filesystem::file_time_type> lastModifiedTimes;
    std::thread watcherThread;
    std::atomic<bool> stopWatching{false};
};

} // namespace ANI