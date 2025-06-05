#pragma once

#include "BasePlugin.hpp"
#include "PluginAPI.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
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

// Forward declarations to avoid circular includes
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugin {

	/**
	 * Manages loading, unloading, and hot-reloading of plugins
	 * Supports dependency resolution and error handling
	 */
	class PluginManager {
	public:
		// Plugin loading callbacks
		using LoadCallback = std::function<void(const std::string&, bool)>;
		using UnloadCallback = std::function<void(const std::string&)>;

		// Plugin information structure
		struct PluginInfo {
			std::string name;
			std::string path;
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
			using GetPluginNameFunc = const char* (*)();
			using GetPluginVersionFunc = const char* (*)();

			CreatePluginFunc createFunc = nullptr;
			DestroyPluginFunc destroyFunc = nullptr;
			GetPluginNameFunc getNameFunc = nullptr;
			GetPluginVersionFunc getVersionFunc = nullptr;
		};

		// Statistics structure
		struct Stats {
			size_t totalPlugins = 0;
			size_t loadedPlugins = 0;
			size_t errorPlugins = 0;
			std::chrono::milliseconds lastCheckTime{};
		};

		// Constructor/Destructor
		PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr);
		~PluginManager();

		// Disable copy/move
		PluginManager(const PluginManager&) = delete;
		PluginManager& operator=(const PluginManager&) = delete;

		// Plugin management
		bool LoadPlugin(const std::string& pluginPath);
		bool UnloadPlugin(const std::string& pluginName);
		bool ReloadPlugin(const std::string& pluginName);
		void UnloadAllPlugins();

		// Hot reload functionality
		void StartHotReload(const std::string& watchDir,
			std::chrono::milliseconds interval = std::chrono::milliseconds(500));
		void StopHotReload();
		bool IsHotReloadActive() const { return hotReloadActive; }

		// Plugin updates
		void Update(float deltaTime);
		void RefreshPluginDirectory();

		// Plugin queries
		std::vector<std::string> GetLoadedPluginNames() const;
		std::vector<PluginInfo> GetAllPluginInfo() const;
		PluginInfo* GetPluginInfo(const std::string& name);
		bool IsPluginLoaded(const std::string& name) const;

		// Statistics
		Stats GetStats() const;

		// Callbacks
		void SetLoadCallback(const LoadCallback& callback) { loadCallback = callback; }
		void SetUnloadCallback(const UnloadCallback& callback) { unloadCallback = callback; }

		// Directory management
		void SetWatchDirectory(const std::string& dir) { watchDirectory = dir; }
		const std::string& GetWatchDirectory() const { return watchDirectory; }

	private:
		// Core references
		ECS::EntityManager& entityManager;
		GUI::ViewManager& viewManager;

		// Plugin storage
		std::unordered_map<std::string, PluginInfo> plugins;
		mutable std::mutex pluginMutex;

		// Hot reload
		bool hotReloadActive;
		std::atomic<bool> shouldStopWatching;
		std::thread watchThread;
		std::string watchDirectory;
		std::chrono::milliseconds checkInterval;

		// Callbacks
		LoadCallback loadCallback;
		UnloadCallback unloadCallback;

		// Internal methods
		bool LoadPluginInternal(const std::string& pluginPath, bool isReload);
		void UnloadPluginInternal(const std::string& name);
		bool ValidatePluginFunctions(PluginInfo& info);

		// Hot reload helpers
		void WatchForChanges();
		void CheckForPluginChanges();

		// Utility methods
		std::string GetPluginNameFromPath(const std::string& path);
		bool IsPluginFile(const std::string& path);
		std::filesystem::file_time_type GetFileWriteTime(const std::string& path);
		std::string GetLastSystemError();

		// File management for hot reload
		void CopyPluginForReload(const std::string& originalPath, std::string& tempPath);
		void CleanupTempFile(const std::string& tempPath);
	};

} // namespace Plugin