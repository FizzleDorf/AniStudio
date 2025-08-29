/*
 * PluginManager.hpp - Simple plugin manager that directly registers everything
 */

#pragma once

#include "PluginAPI.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
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

namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugin {

	class PluginManager {
	public:
		struct PluginInfo {
			std::string name;
			std::string originalPath;
			std::string tempPath;
			std::filesystem::file_time_type lastWriteTime;

			bool isLoaded = false;
			bool hasError = false;
			std::string errorMessage;

			LIBRARY_HANDLE handle = nullptr;
			std::unique_ptr<IPlugin> instance;

			// Function pointers
			IPlugin*(*createFunc)() = nullptr;
			void(*destroyFunc)(IPlugin*) = nullptr;
			const char*(*getNameFunc)() = nullptr;
			const char*(*getVersionFunc)() = nullptr;
			const char*(*getDescFunc)() = nullptr;
		};

	private:
		ECS::EntityManager* entityManager;
		GUI::ViewManager* viewManager;
		PluginContext context; // This line was causing the error due to missing access specifier

		std::unordered_map<std::string, PluginInfo> plugins;
		mutable std::mutex pluginMutex;

		// Hot reload
		std::atomic<bool> hotReloadActive{ false };
		std::atomic<bool> shouldStopWatching{ false };
		std::thread watchThread;
		std::string watchDirectory;
		std::string tempDirectory;

		static std::atomic<int> tempFileCounter;

	public:
		PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager* viewMgr = nullptr);
		~PluginManager();

		// Core functions
		bool LoadPlugin(const std::string& pluginPath);
		bool UnloadPlugin(const std::string& pluginName);
		bool ReloadPlugin(const std::string& pluginName);
		void UnloadAllPlugins();
		void Update(float deltaTime);

		// Plugin info
		bool IsPluginLoaded(const std::string& name) const;
		std::vector<std::string> GetLoadedPluginNames() const;
		PluginInfo* GetPluginInfo(const std::string& name);

		// Hot reload
		void StartHotReload(const std::string& watchDir);
		void StopHotReload();
		bool IsHotReloadActive() const { return hotReloadActive; }
		const std::string& GetWatchDirectory() const { return watchDirectory; }

	private:
		void SetupTempDirectory();
		void CleanupTempDirectory();

		bool LoadPluginInternal(const std::string& pluginPath, bool isReload = false);
		void UnloadPluginInternal(const std::string& name);
		bool ValidatePluginFunctions(PluginInfo& info);

		std::string CreateTempCopy(const std::string& originalPath);
		void WatchForChanges();
		void CheckForPluginChanges();

		std::string GetPluginNameFromPath(const std::string& path) const;
		std::filesystem::file_time_type GetFileWriteTime(const std::string& path) const;
		bool IsPluginFile(const std::string& path) const;
		std::string GetLastSystemError() const;
	};

} // namespace Plugin
