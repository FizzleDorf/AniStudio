#include "ExamplePlugin.hpp"

namespace ExamplePlugin {
PluginConfig config;
}

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
    void InitializePlugin(ECS::EntityManager &entityMgr) {
    try {
        // Register component
        entityMgr.RegisterBuiltInComponent<ExamplePlugin::ExampleComponent>();

        // Register system
        entityMgr.RegisterSystem<ExamplePlugin::ExampleSystem>();

        // Create a test entity
        auto entity = entityMgr.AddNewEntity();
        entityMgr.AddComponent<ExamplePlugin::ExampleComponent>(entity);

        std::cout << "Example plugin initialized successfully!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Failed to initialize plugin: " << e.what() << std::endl;
    }
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    void ShutdownPlugin() {
    std::cout << "Example plugin shutting down" << std::endl;
}
}