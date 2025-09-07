#pragma once
#include "PluginRegistry.hpp"
#include "BasePlugin.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <functional>
#include <chrono>
#include <atomic>

// Forward declaration for ImGui
struct ImGuiContext;

namespace ECS {
	class EntityManager;
}

namespace Plugins {

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

	protected:
		std::string pluginName;
		PluginManager* manager;
		ImGuiContext* imguiContext;
	};

	// Enhanced plugin manager with versioned DLL hot reload support
	class PluginManager {
	public:
		PluginManager(ECS::EntityManager& entityMgr, ImGuiContext* imguiContext = nullptr);
		virtual ~PluginManager();

		// Plugin loading/unloading
		bool loadPlugin(const std::string& pluginDirPath);
		virtual bool enablePlugin(const std::string& pluginName);
		bool enablePluginWithExistingRegistrations(const std::string& pluginName);
		bool disablePlugin(const std::string& pluginName);
		bool unloadPlugin(const std::string& pluginName);
		bool reloadPlugin(const std::string& pluginName);

		// Hot reload management
		void enableHotReload(bool enable);
		bool isHotReloadEnabled() const { return hotReloadEnabled; }
		void checkForChanges();
		void processPendingReloads();

		// Plugin management
		void updatePlugins(float deltaTime);
		void scanPluginDirectory(const std::string& directory);

		// Plugin access
		std::vector<PluginInfo> getLoadedPlugins() const;
		BasePlugin* getPlugin(const std::string& name) const;

		// Registration methods called by PluginRegistry
		ECS::ComponentTypeID registerComponent(const std::string& pluginName, const ComponentDescriptor& desc);
		ECS::SystemTypeID registerSystem(const std::string& pluginName, const SystemDescriptor& desc);
		virtual GUI::ViewTypeID registerView(const std::string& pluginName, const ViewDescriptor& desc) { return 0; }

		// ImGui context access
		ImGuiContext* getImGuiContext() const { return imguiContext; }

		// Staging directory management
		void setStagingDirectory(const std::string& stagingDir);
		std::string getStagingDirectory() const { return stagingDirectory; }

		// Hot reload configuration
		void setHotReloadCheckInterval(float seconds) { hotReloadCheckInterval = seconds; }
		float getHotReloadCheckInterval() const { return hotReloadCheckInterval; }

	protected:
		ECS::EntityManager& entityManager;
		ImGuiContext* imguiContext;
		std::unordered_map<std::string, PluginInfo> plugins;

		// Hot reload configuration
		std::atomic<bool> hotReloadEnabled{ false };
		std::string stagingDirectory;
		float hotReloadCheckInterval = 1.0f;
		float timeSinceLastCheck = 0.0f;

		// Plugin registration tracking
		std::unordered_map<std::string, std::vector<ECS::ComponentTypeID>> pluginComponents;
		std::unordered_map<std::string, std::vector<ECS::SystemTypeID>> pluginSystems;

		// Versioned DLL hot reload methods
		bool checkStagingForUpdates(const std::string& pluginName);
		bool createVersionedDllFromStaging(const std::string& pluginName);
		bool safeReloadPlugin(const std::string& pluginName);
		void setupPluginDirectories(const std::string& pluginName);
		std::string findPluginDll(const std::string& pluginDir, const std::string& pluginName);
		std::string getVersionedDllName(const std::string& pluginName, uint32_t version);
		std::string findNewestVersionedDll(const std::string& pluginDir, const std::string& pluginName);
		uint32_t extractVersionFromDllName(const std::string& dllPath, const std::string& pluginName);
		void cleanupOldVersionedDlls(const std::string& pluginName, uint32_t keepVersionsCount = 3);

		// Cleanup methods
		void cleanupPluginComponents(const std::string& pluginName);
		void cleanupPluginSystems(const std::string& pluginName);
		virtual void cleanupPluginViews(const std::string& pluginName) {}

		// System library loading helpers
		void* loadDynamicLibrary(const std::string& path);
		void unloadLibrary(void* handle);
		void* getFunction(void* handle, const std::string& name);
		bool copyFile(const std::string& source, const std::string& destination);
	};

} // namespace Plugins