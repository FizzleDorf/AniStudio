#pragma once
#include "ExampleComponent.hpp"
#include "ExampleSystem.hpp"
#include "ExampleView.hpp"
#include "IPlugin.hpp"

namespace ExamplePlugin {

class ExamplePlugin : public Plugin::IPlugin {
public:
    const char *GetName() const override { return "Example Plugin"; }

    Plugin::Version GetVersion() const override { return {1, 0, 0}; }

    std::vector<std::string> GetDependencies() const override { return {}; }

    bool OnLoad(ECS::EntityManager &entityMgr, GUI::ViewManager &viewMgr) override {
        try {
            // Register system with entity manager reference
            entityMgr.RegisterSystem<ExampleSystem>(entityMgr);

            // Create and register view with both manager references
            GUI::ViewID viewID = viewMgr.AddNewView();
            viewMgr.AddView<ExampleView>(viewID, entityMgr);

            return true;
        } catch (const std::exception &e) {
            std::cerr << "Failed to load Example Plugin: " << e.what() << std::endl;
            return false;
        }
    }

    bool OnStart() override { return true; }
    void OnStop() override {}
    void OnUnload() override {}
    void OnUpdate(float dt) override {}
};

} // namespace ExamplePlugin

// Export plugin creation/destruction functions
extern "C" {
PLUGIN_EXPORT Plugin::IPlugin *CreatePlugin() { return new ExamplePlugin::ExamplePlugin(); }

PLUGIN_EXPORT void DestroyPlugin(Plugin::IPlugin *plugin) { delete plugin; }
}