#include "CustomPlugin.hpp"

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
    void InitializePlugin(ECS::EntityManager &entityMgr, GUI::ViewManager &viewMgr) {
    try {
        // Register component
        entityMgr.RegisterBuiltInComponent<CustomPlugin::CustomComponent>();

        // Register system
        entityMgr.RegisterSystem<CustomPlugin::CustomSystem>();

        // Register view
        viewMgr.RegisterView<CustomPlugin::CustomView>("CustomView");

        // Create a test entity
        auto entity = entityMgr.AddNewEntity();
        entityMgr.AddComponent<CustomPlugin::CustomComponent>(entity);

        std::cout << "Custom plugin initialized successfully!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Failed to initialize plugin: " << e.what() << std::endl;
    }
}
}