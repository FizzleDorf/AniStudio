#pragma once
#include "ECS.h"
#include "GUI.h"
#include <string>
#include <vector>

namespace Plugin {

struct Version {
    int major;
    int minor;
    int patch;

    bool IsCompatibleWith(const Version &other) const { return major == other.major && minor <= other.minor; }
    std::string ToString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

enum class PluginState { Created, Loaded, Started, Stopped, Unloaded };

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Core interface methods
    virtual const char *GetName() const = 0;
    virtual Version GetVersion() const = 0;
    virtual std::vector<std::string> GetDependencies() const = 0;

    // Lifecycle methods - now taking manager references
    virtual bool OnLoad(ECS::EntityManager &entityMgr, GUI::ViewManager &viewMgr) = 0;
    virtual bool OnStart() = 0;
    virtual void OnStop() = 0;
    virtual void OnUnload() = 0;
    virtual void OnUpdate(float dt) = 0;

    virtual bool IsCompatibleWith(const Version &appVersion) const { return GetVersion().IsCompatibleWith(appVersion); }

    PluginState GetState() const { return state_; }
    void SetState(PluginState state) { state_ = state; }

protected:
    PluginState state_ = PluginState::Created;
};

using CreatePluginFn = IPlugin *(*)();
using DestroyPluginFn = void (*)(IPlugin *);

#define PLUGIN_API extern "C"
#define PLUGIN_CREATE "CreatePlugin"
#define PLUGIN_DESTROY "DestroyPlugin"

#ifdef _WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT
#endif

} // namespace Plugin