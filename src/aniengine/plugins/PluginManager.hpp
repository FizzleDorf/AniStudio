#pragma once
#include "PluginRegistry.hpp"
#include "BasePlugin.hpp"
#include "PluginState.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <functional>
#include <chrono>
#include <atomic>
#include <memory>

// Forward declaration for ImGui
struct ImGuiContext;

namespace ECS {
	class EntityManager;
}

namespace Plugins {

	// Forward declarations
	class PluginManager;

	// Enhanced plugin information structure with versioned DLL support
	struct PluginInfo {
		std::string name;
		std::string version = "1.0.0";
		std::string path;                    // Main plugin directory path
		std::string activeDllPath;           // Currently loaded DLL path (versioned)
		std::string stagingPath;             // Staging directory path
		uint32_t currentVersion = 0;        // Current DLL version number
		uint32_t nextVersion = 1;           // Next DLL version to use
		void* handle = nullptr;
		bool loaded = false;
		bool enabled = false;
		bool hotReloadPending = false;
		BasePlugin* instance = nullptr;
		BasePlugin*(*createFunc)() = nullptr;
		void(*destroyFunc)(BasePlugin*) = nullptr;
		std::filesystem::file_time_type lastScanTime;
		std::filesystem::file_time_type stagingWriteTime;
	};

	// Base plugin registry implementation - WITH IMGUI CONTEXT SUPPORT
	class PluginRegistry : public IPluginRegistry {
	public:
		PluginRegistry(const std::string& pluginName, PluginManager* manager, ImGuiContext* imguiContext = nullptr);

		// IPluginRegistry interface implementation
		ECS::ComponentTypeID RegisterComponent(const ComponentDescriptor& desc) override;
		ECS::SystemTypeID RegisterSystem(const SystemDescriptor& desc) override;
		GUI::ViewTypeID RegisterView(const ViewDescriptor& desc) override;
		const std::string& GetCurrentPluginName() const override { return pluginName; }

		// Provide ImGui context access to plugins
		ImGuiContext* GetImGuiContext() const override { return imguiContext; }

	private:
		std::string pluginName;
		PluginManager* manager;
		ImGuiContext* imguiContext;
	};

	// Main plugin manager class (ENGINE VERSION)
	class PluginManager {
	public:
		PluginManager(ECS::EntityManager& entityMgr, ImGuiContext* imguiContext = nullptr);
		virtual ~PluginManager();

		// Plugin loading and management
		bool loadPlugin(const std::string& pluginPath);
		virtual bool enablePlugin(const std::string& pluginName);
		bool disablePlugin(const std::string& pluginName);
		bool unloadPlugin(const std::string& pluginName);
		bool reloadPlugin(const std::string& pluginName);

		// Hot reload functionality
		void enableHotReload(bool enable = true);
		void setHotReloadForce(bool force = true); // For developers to force enable
		bool isHotReloadEnabled() const { return hotReloadEnabled; }
		void checkForChanges();
		void updatePlugins(float deltaTime);

		// Directory management
		void setStagingDirectory(const std::string& basePluginsDir);
		void scanPluginDirectory(const std::string& directory);
		void scanPluginDirectoryWithoutAutoLoad(const std::string& directory);

		// Plugin information
		std::vector<PluginInfo> getLoadedPlugins() const;
		BasePlugin* getPlugin(const std::string& name) const;

		// Plugin state management
		void SetGlobalDataPath(const std::string& dataPath);
		void LoadGlobalPluginState();
		void SaveGlobalPluginState();
		void SetProjectContext(const std::string& projectPath);
		void SaveProjectPluginState();
		void UseGlobalPluginState();

		// Registration methods (called by PluginRegistry)
		virtual ECS::ComponentTypeID registerComponent(const std::string& pluginName, const ComponentDescriptor& desc);
		virtual ECS::SystemTypeID registerSystem(const std::string& pluginName, const SystemDescriptor& desc);
		virtual GUI::ViewTypeID registerView(const std::string& pluginName, const ViewDescriptor& desc);

	protected:
		// Cleanup methods for when plugins are disabled/unloaded
		virtual void cleanupPluginComponents(const std::string& pluginName);
		virtual void cleanupPluginSystems(const std::string& pluginName);
		virtual void cleanupPluginViews(const std::string& pluginName);

		// Core references
		ECS::EntityManager& entityManager;
		ImGuiContext* imguiContext = nullptr;

		// Plugin storage
		std::unordered_map<std::string, PluginInfo> plugins;

		// Component/System tracking for cleanup
		std::unordered_map<std::string, std::vector<ECS::ComponentTypeID>> pluginComponents;
		std::unordered_map<std::string, std::vector<ECS::SystemTypeID>> pluginSystems;

		// Hot reload state
		bool hotReloadEnabled = false; // Changed: Default to false
		bool hotReloadForced = false;  // New: Allow developers to force enable
		float timeSinceLastCheck = 0.0f;
		float hotReloadCheckInterval = 1.0f; // Check every second

		// Directory management
		std::string stagingDirectory = "../plugins";

		// Plugin state management
		std::unique_ptr<PluginState> pluginState;

	private:
		// Helper methods for plugin loading
		void setupPluginDirectories(const std::string& pluginName);
		std::string findPluginDll(const std::string& pluginDir, const std::string& pluginName);
		std::string getVersionedDllName(const std::string& pluginName, uint32_t version);
		std::string findNewestVersionedDll(const std::string& pluginDir, const std::string& pluginName);
		uint32_t extractVersionFromDllName(const std::string& dllPath, const std::string& pluginName);
		void cleanupOldVersionedDlls(const std::string& pluginName, uint32_t keepVersionsCount = 3);

		// Hot reload internals
		bool checkStagingForUpdates(const std::string& pluginName);
		bool createVersionedDllFromStaging(const std::string& pluginName);
		bool safeReloadPlugin(const std::string& pluginName);
		void processPendingReloads();

		// Plugin state management helpers
		void InitializePluginStateManager();
		void LoadPluginsFromState();
		void SaveCurrentPluginState();

		// Platform-specific dynamic library loading
		void* loadDynamicLibrary(const std::string& path);
		void unloadLibrary(void* handle);
		void* getFunction(void* handle, const std::string& functionName);
		bool copyFile(const std::string& source, const std::string& destination);

		// Make PluginRegistry a friend so it can access our registration methods
		friend class PluginRegistry;
	};

} // namespace Plugins