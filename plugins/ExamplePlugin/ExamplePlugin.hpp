#pragma once
#include "ExampleComponent.hpp"
#include "ExampleSystem.hpp"
#include "ExampleView.hpp"
#include "IPlugin.hpp"

namespace Plugin {

class ExamplePlugin : public IPlugin {
public:
    const char *GetName() const override { return "Example Plugin"; }
    Version GetVersion() const override { return {1, 0, 0}; }
    std::vector<std::string> GetDependencies() const override { return {}; }

    bool OnStart() override { 
        // Register plugin views to the AniStudio ViewManager
        auto viewID = viewManager->CreateView();
        std::cout << "Plugin registering view with AniStudio ViewManager, ID: " << viewID << std::endl;
        
        // Register systems
        entityManager->RegisterSystem<ECS::ExampleSystem>();
        
        // Create view instance
        viewManager->AddView<GUI::ExampleView>(viewID, GUI::ExampleView(*entityManager));
        viewManager->GetView<GUI::ExampleView>(viewID).Init();

        return true; 
    }

    bool OnLoad(ECS::EntityManager *entityMgr, GUI::ViewManager *viewMgr) override {
        if (!entityMgr || !viewMgr)
            return false;
        entityManager = entityMgr;
        viewManager = viewMgr;
        SetState(PluginState::Loaded);
        return true;
    }

    void OnStop() override {}
    void OnUnload() override {}
    void OnUpdate(float dt) override {}

private:
    // We don't own these pointers, just use them
    ECS::EntityManager *entityManager = nullptr;
    GUI::ViewManager *viewManager = nullptr;
};
} // namespace ExamplePlugin

// Export plugin creation/destruction functions
extern "C" {
PLUGIN_EXPORT Plugin::IPlugin *CreatePlugin() { return new Plugin::ExamplePlugin(); }

PLUGIN_EXPORT void DestroyPlugin(Plugin::IPlugin *plugin) { delete plugin; }
}