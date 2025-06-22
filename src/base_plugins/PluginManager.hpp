//============================================================================
// PluginManager.hpp - FIXED - No Engine Dependencies
//============================================================================

#pragma once

#include "BasePlugin.hpp"
#include "PluginAPI.hpp"
#include "FilePaths.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#define LOAD_LIBRARY(path) LoadLibraryA(path)
#define GET_PROC_ADDRESS(handle, name) GetProcAddress(handle, name)
#define UNLOAD_LIBRARY(handle) FreeLibrary(handle)
#define LIBRARY_HANDLE HMODULE
#else
#include <dlfcn.h>
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
#define UNLOAD_LIBRARY(handle) dlclose(handle)
#define LIBRARY_HANDLE void*
#endif

namespace Plugin {

	// Plugin info structure
	struct PluginInfo {
		std::string name;
		std::string pluginDir;        // build/plugins/PluginName/
		std::string stagingPath;      // build/plugins/PluginName/staging/Plugin.dll
		std::string activePath;       // build/plugins/PluginName/Plugin.dll
		bool isLoaded = false;

		LIBRARY_HANDLE handle = nullptr;
		std::shared_ptr<BasePlugin> instance;

		CreatePluginFunc createFunc = nullptr;
		DestroyPluginFunc destroyFunc = nullptr;
		GetPluginNameFunc getNameFunc = nullptr;
		GetPluginVersionFunc getVersionFunc = nullptr;
	};

	class PluginManager {
	public:
		explicit PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr);
		~PluginManager();

		// Basic operations
		void Initialize();
		void Init() { Initialize(); }
		void Shutdown();
		void Update(float deltaTime);

		// Host function setup - called by Engine during initialization
		void SetHostFunctions(
			GetEntityManagerFunc entityGetter,
			GetViewManagerFunc viewGetter,
			GetImGuiContextFunc contextGetter,
			GetImGuiAllocFunc allocGetter,
			GetImGuiFreeFunc freeGetter,
			GetImGuiUserDataFunc userDataGetter
		);

		// Plugin management
		bool ScanForPlugins();
		bool LoadPlugin(const std::string& pluginName);
		bool UnloadPlugin(const std::string& pluginName);
		bool ReloadPlugin(const std::string& pluginName);

		// Queries
		bool IsPluginLoaded(const std::string& pluginName) const;
		std::vector<std::string> GetLoadedPlugins() const;
		std::vector<std::string> GetAvailablePlugins() const;

		// Error handling
		std::string GetLastError() const { return lastError; }

	private:
		ECS::EntityManager& entityManager;
		GUI::ViewManager& viewManager;

		std::unordered_map<std::string, PluginInfo> plugins;
		mutable std::mutex pluginMutex;

		std::string pluginsDirectory;
		std::string lastError;

		// Host function pointers - set by Engine
		GetEntityManagerFunc hostGetEntityManager;
		GetViewManagerFunc hostGetViewManager;
		GetImGuiContextFunc hostGetImGuiContext;
		GetImGuiAllocFunc hostGetImGuiAlloc;
		GetImGuiFreeFunc hostGetImGuiFree;
		GetImGuiUserDataFunc hostGetImGuiUserData;

		bool LoadPluginLibrary(PluginInfo& plugin, const std::string& libraryPath);
		void UnloadPluginLibrary(PluginInfo& plugin);
		void SetError(const std::string& error);
	};

} // namespace Plugin