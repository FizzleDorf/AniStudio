/*
 * PluginManager.hpp - Simple plugin system that actually fucking works
 * NO CR BULLSHIT - Just dynamic library loading with hot reload
 */

#pragma once

#include "PluginAPI.hpp"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <filesystem>

 // Platform-specific library loading
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

// Forward declare ECS and GUI classes
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugin {

	class PluginManager {
	public:
		// Plugin info structure for tracking loaded plugins
		struct PluginInfo {
			std::string name;
			std::string path;
			std::string tempPath;
			bool isLoaded = false;
			bool hasError = false;
			std::string errorMessage;
			std::filesystem::file_time_type lastWriteTime;

			// Library handle and function pointers
			LIBRARY_HANDLE handle = nullptr;
			std::shared_ptr<BasePlugin> instance;

			// Function pointer types
			using CreatePluginFunc = BasePlugin * (*)();
			using DestroyPluginFunc = void(*)(BasePlugin*);
			using GetPluginNameFunc = const char*(*)();
			using GetPluginVersionFunc = const char*(*)();

			CreatePluginFunc createFunc = nullptr;
			DestroyPluginFunc destroyFunc = nullptr;
			GetPluginNameFunc getNameFunc = nullptr;
			GetPluginVersionFunc getVersionFunc = nullptr;
		};

		// Constructor for engine-only applications
		explicit PluginManager(ECS::EntityManager& entityMgr)
			: entityManager(entityMgr), viewManager(nullptr) {}

		// Constructor for full studio applications  
		PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
			: entityManager(entityMgr), viewManager(&viewMgr) {}

		~PluginManager() {
			UnloadAllPlugins();
		}

		// Initialize the plugin manager
		void Init();

		// Plugin loading
		bool LoadPlugin(const std::string& pluginPath);
		bool UnloadPlugin(const std::string& pluginName);
		bool ReloadPlugin(const std::string& pluginName);
		void UnloadAllPlugins();

		// Plugin management
		void Update(float deltaTime);
		bool IsPluginLoaded(const std::string& name) const;
		std::vector<std::string> GetLoadedPluginNames() const;
		PluginInfo* GetPluginInfo(const std::string& name);

		// Context management
		bool HasGUISupport() const { return viewManager != nullptr; }

		// Event callbacks
		void SetLoadCallback(std::function<void(const std::string&, bool)> callback) {
			loadCallback = callback;
		}

		void SetUnloadCallback(std::function<void(const std::string&)> callback) {
			unloadCallback = callback;
		}

		void SetErrorCallback(std::function<void(const std::string&, const std::string&)> callback) {
			errorCallback = callback;
		}

		// Hot reload functionality
		void StartHotReload(const std::string& watchDir,
			std::chrono::milliseconds interval = std::chrono::milliseconds(500));
		void StopHotReload();
		bool IsHotReloadActive() const { return hotReloadActive; }

		// Directory management
		void SetWatchDirectory(const std::string& dir) { watchDirectory = dir; }
		const std::string& GetWatchDirectory() const { return watchDirectory; }
		void RefreshPluginDirectory();

	private:
		// Host managers
		ECS::EntityManager& entityManager;
		GUI::ViewManager* viewManager;  // Null for engine-only mode

		// Plugin storage
		std::unordered_map<std::string, PluginInfo> plugins;
		mutable std::mutex pluginMutex;

		// Event callbacks
		std::function<void(const std::string&, bool)> loadCallback;
		std::function<void(const std::string&)> unloadCallback;
		std::function<void(const std::string&, const std::string&)> errorCallback;

		// Hot reload
		bool hotReloadActive = false;
		std::atomic<bool> shouldStopWatching{ false };
		std::thread watchThread;
		std::string watchDirectory;
		std::chrono::milliseconds checkInterval{ 500 };

		// Helper methods
		std::string GetPluginNameFromPath(const std::string& path);
		bool IsPluginFile(const std::string& path);
		std::filesystem::file_time_type GetFileWriteTime(const std::string& path);
		std::string GetLastSystemError();

		// Hot reload helpers
		void WatchForChanges();
		void CheckForPluginChanges();

		// Internal plugin management
		bool LoadPluginInternal(const std::string& pluginPath);
		void UnloadPluginInternal(const std::string& name);
		bool ValidatePluginFunctions(PluginInfo& info);

		// File management for hot reload
		bool IsFileInUse(const std::string& filePath);
		std::string CreateTempCopy(const std::string& originalPath);
		void CleanupTempFile(const std::string& tempPath);
	};
}