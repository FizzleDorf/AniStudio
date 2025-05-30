// // PluginInterface.hpp
// #pragma once

// #include <string>

// // Forward declarations to avoid circular dependencies
// namespace ECS {
// 	class EntityManager;
// }

// namespace GUI {
// 	class ViewManager;
// }

// namespace Plugin {

// 	class IPlugin {
// 	public:
// 		virtual ~IPlugin() = default;

// 		// Called when the plugin is first loaded
// 		virtual void OnLoad(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) = 0;

// 		// Called when the plugin is unloaded (release resources)
// 		virtual void OnUnload() = 0;

// 		// Update function called each frame
// 		virtual void OnUpdate(float deltaTime) = 0;

// 		// Get plugin information
// 		virtual std::string GetName() const = 0;
// 		virtual std::string GetVersion() const = 0;
// 		virtual std::string GetDescription() const = 0;
// 	};

// 	// Define the entry points that each plugin DLL must export
// 	extern "C" {
// 		typedef IPlugin* (*CreatePluginFn)();
// 		typedef void(*DestroyPluginFn)(IPlugin*);
// 	}

// } // namespace Plugin