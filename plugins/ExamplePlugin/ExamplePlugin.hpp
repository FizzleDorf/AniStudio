#pragma once
#include "ExampleComponent.hpp"
#include "ExampleSystem.hpp"
#include "ExampleView.hpp"
#include "../../../AniStudio/Plugins/IPlugin.hpp"

namespace ExamplePlugin {

class ExamplePlugin : public Plugin::IPlugin {
public:
    const char *GetName() const override { return "Counter Plugin"; }

    Plugin::Version GetVersion() const override {
        return {1, 0, 0}; // major, minor, patch
    }

    std::vector<std::string> GetDependencies() const override {
        return {}; // No dependencies
    }

    bool OnLoad() override {
        try {
            // Register system
            ECS::mgr.RegisterSystem<ExampleSystem>();

            // Create and register view
            GUI::ViewID viewID = GUI::viewMgr.AddNewView();
            GUI::viewMgr.AddView<ExampleView>(viewID);

            return true;
        } catch (const std::exception &e) {
            std::cerr << "Failed to load Counter Plugin: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool OnStart() override { return true; }

    void OnStop() override {}

    void OnUnload() override {}

    void OnUpdate(float dt) override {}
};

} // namespace CounterPlugin

// Export plugin creation/destruction functions
extern "C" {
PLUGIN_EXPORT Plugin::IPlugin *CreatePlugin() { return new ExamplePlugin::ExamplePlugin(); }

PLUGIN_EXPORT void DestroyPlugin(Plugin::IPlugin *plugin) { delete plugin; }
}