#pragma once
#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <functional>

namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugins {

	struct PluginInfo {
		std::string name;
		std::string version;
		std::string path;
		void* handle = nullptr;
		BasePlugin* instance = nullptr;
		bool loaded = false;
		bool enabled = false;
		std::filesystem::file_time_type lastWriteTime;

		// Function pointers
		BasePlugin*(*createFunc)();
		void(*destroyFunc)(BasePlugin*);
	};

	// Concrete implementation of the registry interface
	class PluginRegistry : public IPluginRegistry {
	public:
		PluginRegistry(const std::string& pluginName, class PluginManager* manager);

		// IPluginRegistry implementation
		ECS::ComponentTypeID RegisterComponent(const ComponentDescriptor& desc) override;
		ECS::SystemTypeID RegisterSystem(const SystemDescriptor& desc) override;
		GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) override;
		const std::string& GetCurrentPluginName() const override { return pluginName; }

	private:
		std::string pluginName;
		class PluginManager* manager;
	};

	class PluginManager {
	public:
		explicit PluginManager(ECS::EntityManager& entityMgr);
		virtual ~PluginManager();

		// Plugin management
		bool loadPlugin(const std::string& dllPath);
		bool unloadPlugin(const std::string& pluginName);
		virtual bool enablePlugin(const std::string& pluginName);
		bool disablePlugin(const std::string& pluginName);
		bool reloadPlugin(const std::string& pluginName);

		// Hot reload
		void enableHotReload(bool enable) { hotReloadEnabled = enable; }
		void checkForChanges();

		// Access
		std::vector<PluginInfo> getLoadedPlugins() const;
		BasePlugin* getPlugin(const std::string& name) const;

		// Update all enabled plugins
		void updatePlugins(float deltaTime);

		// Directory scanning
		void scanPluginDirectory(const std::string& directory);

		// Registration methods for plugins (called by PluginRegistry)
		ECS::ComponentTypeID registerComponent(const std::string& pluginName, const ComponentDescriptor& desc);
		ECS::SystemTypeID registerSystem(const std::string& pluginName, const SystemDescriptor& desc);

		// View registration is empty in base implementation
		virtual GUI::ViewTypeID registerView(const std::string& pluginName, const ViewDescriptor& desc) {
			return 0;
		}

	protected:
		// Cleanup
		virtual void cleanupPluginComponents(const std::string& pluginName);
		virtual void cleanupPluginSystems(const std::string& pluginName);
		virtual void cleanupPluginViews(const std::string& pluginName) {} // Empty in base

		// Platform-specific
		void* loadDynamicLibrary(const std::string& path);
		void unloadLibrary(void* handle);
		void* getFunction(void* handle, const std::string& name);

	protected:
		ECS::EntityManager& entityManager;
		std::unordered_map<std::string, PluginInfo> plugins;
		bool hotReloadEnabled = false;

		// Track which plugin registered what
		std::unordered_map<std::string, std::vector<ECS::ComponentTypeID>> pluginComponents;
		std::unordered_map<std::string, std::vector<ECS::SystemTypeID>> pluginSystems;

		// NO MORE STATIC VARIABLES!

	private:
		friend class PluginRegistry;
	};

} // namespace Plugins