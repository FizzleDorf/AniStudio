#pragma once
#include "IPlugin.hpp"
#include <filesystem>
#include <iostream>
#include <memory>

#ifdef _WIN32
#include <windows.h>
using LibHandle = HMODULE;
#else
#include <dlfcn.h>
using LibHandle = void *;
#endif

namespace Plugin {

class PluginLoader {
public:
    explicit PluginLoader(const std::string &path) : path_(path), handle_(nullptr), plugin_(nullptr) {
        lastWriteTime_ = std::filesystem::last_write_time(path);
    }

    ~PluginLoader() { Unload(); }

    bool Load(const Version &appVersion, ECS::EntityManager *entityMgr, GUI::ViewManager *viewMgr) {
        std::cout << "Loading plugin: " << path_ << std::endl;

        if (IsLoaded())
            return false;

        if (!LoadPluginLibrary()) {
            std::cerr << "Failed to load plugin library" << std::endl;
            return false;
        }

        if (!LoadPluginFunctions()) {
            std::cerr << "Failed to load plugin functions" << std::endl;
            Unload();
            return false;
        }

        plugin_ = createFn_();
        if (!plugin_ || !plugin_->OnLoad(entityMgr, viewMgr)) {
            std::cerr << "Failed to initialize plugin" << std::endl;
            Unload();
            return false;
        }

        plugin_->SetState(PluginState::Loaded);
        return true;
    }

    bool CheckForHotReload(ECS::EntityManager *entityMgr, GUI::ViewManager *viewMgr) {
        if (!IsLoaded())
            return false;

        try {
            auto currentWriteTime = std::filesystem::last_write_time(path_);
            if (currentWriteTime == lastWriteTime_)
                return false;

            std::cout << "Hot reloading plugin: " << path_ << std::endl;

            // Save current plugin state
            std::unique_ptr<IPluginData> savedState;
            PluginState currentState = plugin_->GetState();

            if (plugin_) {
                savedState.reset(plugin_->SaveState());
                if (currentState == PluginState::Started) {
                    plugin_->OnStop();
                }
            }

            // Unload current plugin
            Unload();

            // Load new version
            if (!Load(Version{1, 0, 0}, entityMgr, viewMgr)) {
                std::cerr << "Failed to reload plugin" << std::endl;
                return false;
            }

            // Restore state
            if (savedState && !plugin_->LoadState(savedState.get())) {
                std::cerr << "Failed to restore plugin state" << std::endl;
                return false;
            }

            // Restart if it was running
            if (currentState == PluginState::Started) {
                if (!Start()) {
                    std::cerr << "Failed to restart plugin after reload" << std::endl;
                    return false;
                }
            }

            lastWriteTime_ = currentWriteTime;
            return true;
        } catch (const std::exception &e) {
            std::cerr << "Error during hot reload: " << e.what() << std::endl;
            return false;
        }
    }

    bool Start() {
        if (!plugin_ || plugin_->GetState() != PluginState::Loaded) {
            return false;
        }

        if (!plugin_->OnStart()) {
            return false;
        }

        plugin_->SetState(PluginState::Started);
        return true;
    }

    void Stop() {
        if (plugin_ && plugin_->GetState() == PluginState::Started) {
            plugin_->OnStop();
            plugin_->SetState(PluginState::Stopped);
        }
    }

    void Update(float dt) {
        if (plugin_ && plugin_->GetState() == PluginState::Started) {
            plugin_->OnUpdate(dt);
        }
    }

    void Unload() {
        if (plugin_) {
            if (plugin_->GetState() == PluginState::Started) {
                Stop();
            }
            plugin_->OnUnload();
            if (destroyFn_) {
                destroyFn_(plugin_);
            }
            plugin_ = nullptr;
        }

        if (handle_) {
#ifdef _WIN32
            FreeLibrary(handle_);
#else
            dlclose(handle_);
#endif
            handle_ = nullptr;
        }

        createFn_ = nullptr;
        destroyFn_ = nullptr;
    }

    bool IsLoaded() const { return handle_ != nullptr; }
    IPlugin *Get() const { return plugin_; }

private:
    bool LoadPluginLibrary() {
#ifdef _WIN32
        handle_ = LoadLibraryA(path_.c_str());
        if (!handle_) {
            std::cerr << "Failed to load plugin: " << path_ << " (Error: " << GetLastError() << ")" << std::endl;
            return false;
        }
#else
        handle_ = dlopen(path_.c_str(), RTLD_LAZY);
        if (!handle_) {
            std::cerr << "Failed to load plugin: " << path_ << " (Error: " << dlerror() << ")" << std::endl;
            return false;
        }
#endif
        return true;
    }

    bool LoadPluginFunctions() {
#ifdef _WIN32
        createFn_ = reinterpret_cast<CreatePluginFn>(GetProcAddress(handle_, PLUGIN_CREATE));
        destroyFn_ = reinterpret_cast<DestroyPluginFn>(GetProcAddress(handle_, PLUGIN_DESTROY));
#else
        createFn_ = reinterpret_cast<CreatePluginFn>(dlsym(handle_, PLUGIN_CREATE));
        destroyFn_ = reinterpret_cast<DestroyPluginFn>(dlsym(handle_, PLUGIN_DESTROY));
#endif
        return createFn_ && destroyFn_;
    }

    std::string path_;
    std::filesystem::file_time_type lastWriteTime_;
    LibHandle handle_;
    IPlugin *plugin_;
    CreatePluginFn createFn_ = nullptr;
    DestroyPluginFn destroyFn_ = nullptr;
};

} // namespace Plugin