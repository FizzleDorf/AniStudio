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

    bool Load(const Version &appVersion, ECS::EntityManager &entityMgr, GUI::ViewManager &viewMgr) {
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
        if (!plugin_ || !plugin_->OnLoad(entityMgr, viewMgr)) {
            Unload();
            return false;
        }

        plugin_->SetState(PluginState::Loaded);
        return true;
    }

    // ... rest of the implementation stays the same ...
private:
    std::string path_;
    LibHandle handle_;
    IPlugin *plugin_;
    CreatePluginFn createFn_ = nullptr;
    DestroyPluginFn destroyFn_ = nullptr;
};

} // namespace Plugin