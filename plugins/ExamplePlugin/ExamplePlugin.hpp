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

    bool OnLoad(ECS::EntityManager *entityMgr, GUI::ViewManager *viewMgr) override {
        // Store pointers
        entityManager = entityMgr;
        viewManager = viewMgr;

        // Register views and systems using the stored pointers
        auto viewID = viewManager.AddNewView();
        viewManager->AddView<ExampleView>(viewID, ExampleView(entityManager));

        // Register systems
        entityManager->RegisterSystem<ExampleSystem>();

        return true;
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