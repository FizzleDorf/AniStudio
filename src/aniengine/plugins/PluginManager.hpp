#pragma once
#include "PluginRegistry.hpp"
#include "BasePlugin.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <functional>

// Forward declaration for ImGui
struct ImGuiContext;

namespace ECS {
	class EntityManager;
}

namespace Plugins {

	// Plugin information structure
	struct PluginInfo {
		std::string name;
		std::string version = "1.0.0"; // Add version field
		std::string path;
		void* handle = nullptr;
		bool loaded = false;
		bool enabled = false;
		BasePlugin* instance = nullptr;
		BasePlugin*(*createFunc)() = nullptr;
		void(*destroyFunc)(BasePlugin*) = nullptr;
		std::filesystem::file_time_type lastWriteTime;
	};

	// Base plugin registry implementation - NOW WITH IMGUI CONTEXT SUPPORT
	class PluginRegistry : public IPluginRegistry {
	public:
		PluginRegistry(const std::string& pluginName, PluginManager* manager, ImGuiContext* imguiContext = nullptr);

		// IPluginRegistry interface implementation
		ECS::ComponentTypeID RegisterComponent(const ComponentDescriptor& desc) override;
		ECS::SystemTypeID RegisterSystem(const SystemDescriptor& desc) override;
		GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) override;
		const std::string& GetCurrentPluginName() const override { return pluginName; }

		// NEW: Provide ImGui context access to plugins
		ImGuiContext* GetImGuiContext() const override { return imguiContext; }

	protected:
		std::string pluginName;
		PluginManager* manager;
		ImGuiContext* imguiContext; // Store the ImGui context
	};

	// Main plugin manager
	class PluginManager {
	public:
		PluginManager(ECS::EntityManager& entityMgr, ImGuiContext* imguiContext = nullptr);
		virtual ~PluginManager();

		// Plugin loading/unloading
		bool loadPlugin(const std::string& dllPath);
		virtual bool enablePlugin(const std::string& pluginName);
		bool disablePlugin(const std::string& pluginName);
		bool unloadPlugin(const std::string& pluginName);
		bool reloadPlugin(const std::string& pluginName);

		// Plugin management
		void updatePlugins(float deltaTime);
		void scanPluginDirectory(const std::string& directory);
		void checkForChanges();

		// Hot reload
		void enableHotReload(bool enable) { hotReloadEnabled = enable; }
		bool isHotReloadEnabled() const { return hotReloadEnabled; }

		// Plugin access
		std::vector<PluginInfo> getLoadedPlugins() const;
		BasePlugin* getPlugin(const std::string& name) const;

		// Registration methods called by PluginRegistry
		ECS::ComponentTypeID registerComponent(const std::string& pluginName, const ComponentDescriptor& desc);
		ECS::SystemTypeID registerSystem(const std::string& pluginName, const SystemDescriptor& desc);
		virtual GUI::ViewTypeID registerView(const std::string& pluginName, const ViewDescriptor& desc) { return 0; }

		// ImGui context access
		ImGuiContext* getImGuiContext() const { return imguiContext; }

	protected:
		ECS::EntityManager& entityManager;
		ImGuiContext* imguiContext; // Store ImGui context in base class
		std::unordered_map<std::string, PluginInfo> plugins;
		bool hotReloadEnabled = false;

		// Plugin registration tracking
		std::unordered_map<std::string, std::vector<ECS::ComponentTypeID>> pluginComponents;
		std::unordered_map<std::string, std::vector<ECS::SystemTypeID>> pluginSystems;

		// Cleanup methods
		void cleanupPluginComponents(const std::string& pluginName);
		void cleanupPluginSystems(const std::string& pluginName);
		virtual void cleanupPluginViews(const std::string& pluginName) {}

		// System library loading helpers
		void* loadDynamicLibrary(const std::string& path);
		void unloadLibrary(void* handle);
		void* getFunction(void* handle, const std::string& name);
	};

} // namespace Plugins