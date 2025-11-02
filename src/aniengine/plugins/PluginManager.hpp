#pragma once
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

namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugins {

	struct PluginInfo {
		std::string name;
		std::string version = "1.0.0";
		std::string path;
		std::string activeDllPath;
		std::string stagingPath;
		uint32_t currentVersion = 0;
		uint32_t nextVersion = 1;
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

	class PluginManager {
	public:
		PluginManager(ECS::EntityManager& entityMgr);
		virtual ~PluginManager();

		bool loadPlugin(const std::string& pluginPath);
		virtual bool enablePlugin(const std::string& pluginName);
		bool disablePlugin(const std::string& pluginName);
		bool unloadPlugin(const std::string& pluginName);
		bool reloadPlugin(const std::string& pluginName);

		void enableHotReload(bool enable = true);
		void setHotReloadForce(bool force = true);
		bool isHotReloadEnabled() const { return hotReloadEnabled; }
		void checkForChanges();
		void updatePlugins(float deltaTime);

		void setStagingDirectory(const std::string& basePluginsDir);
		void scanPluginDirectory(const std::string& directory);

		std::vector<PluginInfo> getLoadedPlugins() const;
		BasePlugin* getPlugin(const std::string& name) const;

		void SetGlobalDataPath(const std::string& dataPath);
		void LoadGlobalPluginState();
		void SaveGlobalPluginState();
		void SetProjectContext(const std::string& projectPath);
		void SaveProjectPluginState();
		void UseGlobalPluginState();

		void cleanupPluginEntities(const std::string& pluginName);
		void cleanupPluginSystems(const std::string& pluginName);
		void cleanupPluginComponents(const std::string& pluginName);
		void SetViewManager(GUI::ViewManager* viewMgr) { viewManager = viewMgr; }

	protected:
		virtual void cleanupPluginViews(const std::string& pluginName);

		ECS::EntityManager& entityManager;
		GUI::ViewManager* viewManager = nullptr;

		std::unordered_map<std::string, PluginInfo> plugins;

		bool hotReloadEnabled = false;
		bool hotReloadForced = false;
		float timeSinceLastCheck = 0.0f;
		float hotReloadCheckInterval = 1.0f;

		std::string stagingDirectory = "../plugins";

		std::unique_ptr<PluginState> pluginState;

	private:
		void setupPluginDirectories(const std::string& pluginName);
		std::string findPluginDll(const std::string& pluginDir, const std::string& pluginName);
		bool moveFile(const std::string& source, const std::string& destination);

		std::string getVersionedDllName(const std::string& pluginName, uint32_t version);
		std::string findNewestVersionedDll(const std::string& pluginDir, const std::string& pluginName);
		uint32_t extractVersionFromDllName(const std::string& dllPath, const std::string& pluginName);
		void cleanupOldVersionedDlls(const std::string& pluginName, uint32_t keepVersionsCount = 3);

		bool checkStagingForUpdates(const std::string& pluginName);
		bool createVersionedDllFromStaging(const std::string& pluginName);
		bool safeReloadPlugin(const std::string& pluginName);
		void processPendingReloads();

		void InitializePluginStateManager();
		void LoadPluginsFromState();
		void SaveCurrentPluginState();

		void* loadDynamicLibrary(const std::string& path);
		void unloadLibrary(void* handle);
		void* getFunction(void* handle, const std::string& functionName);
		bool copyFile(const std::string& source, const std::string& destination);
	};

} // namespace Plugins