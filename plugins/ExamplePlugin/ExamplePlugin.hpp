/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "PluginInterface.hpp"
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

	class PluginManager {
	public:
		// Plugin loading callbacks
		using LoadCallback = std::function<void(const std::string&, bool)>;
		using UnloadCallback = std::function<void(const std::string&)>;

		void Init();

		// Plugin information structure
		struct PluginInfo {
			std::string name;
			std::string path;
			std::string tempPath; // For hot reload temporary copies
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

		// CRITICAL FIX: File management for hot reload
		bool IsFileInUse(const std::string& filePath);
		std::string CreateTempCopy(const std::string& originalPath);
		void CleanupTempFile(const std::string& tempPath);
	};

} // namespace Plugin