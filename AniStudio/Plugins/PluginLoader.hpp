#pragma once
#include "IPlugin.hpp"
#include <memory>
#include <string>

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
    explicit PluginLoader(const std::string &path) : path_(path), handle_(nullptr), plugin_(nullptr) {}
    ~PluginLoader() { Unload(); }

    bool Load(const Version &appVersion) {
        if (IsLoaded())
            return false;

        if (!LoadPluginLibrary()) {
            return false;
        }

        if (!LoadPluginFunctions()) {
            Unload();
            return false;
        }

        plugin_ = createFn_();
        if (!plugin_ || !plugin_->OnLoad()) {
            Unload();
            return false;
        }

        plugin_->SetState(PluginState::Loaded);
        return true;
    }

    bool Start() {
        if (!plugin_ || plugin_->GetState() != PluginState::Loaded)
            return false;

        if (!plugin_->OnStart())
            return false;

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
            if (destroyFn_)
                destroyFn_(plugin_);
            plugin_ = nullptr;
        }

        if (handle_) {
#ifdef _WIN32
            ::FreeLibrary(handle_);
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
        handle_ = ::LoadLibraryA(path_.c_str());
        if (!handle_) {
            std::cerr << "Failed to load plugin: " << path_ << " (Error: " << GetLastError() << ")\n";
            return false;
        }
#else
        handle_ = dlopen(path_.c_str(), RTLD_LAZY);
        if (!handle_) {
            std::cerr << "Failed to load plugin: " << path_ << " (Error: " << dlerror() << ")\n";
            return false;
        }
#endif
        return true;
    }

    bool LoadPluginFunctions() {
#ifdef _WIN32
        createFn_ = (CreatePluginFn)GetProcAddress(handle_, PLUGIN_CREATE);
        destroyFn_ = (DestroyPluginFn)GetProcAddress(handle_, PLUGIN_DESTROY);
#else
        createFn_ = (CreatePluginFn)dlsym(handle_, PLUGIN_CREATE);
        destroyFn_ = (DestroyPluginFn)dlsym(handle_, PLUGIN_DESTROY);
#endif
        return createFn_ && destroyFn_;
    }

    std::string path_;
    LibHandle handle_;
    IPlugin *plugin_;
    CreatePluginFn createFn_ = nullptr;
    DestroyPluginFn destroyFn_ = nullptr;
};

} // namespace Plugin