/*
 * PluginManager.cpp - Fixed to work with YOUR actual PluginAPI.hpp
 */

#include "PluginManager.hpp"
#include "PluginAPI.hpp"
#include "utils.h"
#include <iostream>
#include <filesystem>

namespace Plugin {

	void PluginManager::Init() {
		std::cout << "[PluginManager] Initializing plugin system..." << std::endl;

		// Set the watch directory to the plugins path
		SetWatchDirectory(Utils::FilePaths::pluginPath);

		// Scan for existing plugins in the directory
		RefreshPluginDirectory();

		std::cout << "[PluginManager] Plugin system initialized" << std::endl;
	}

	bool PluginManager::LoadPlugin(const std::string& pluginPath) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		return LoadPluginInternal(pluginPath);
	}

	bool PluginManager::LoadPluginInternal(const std::string& pluginPath) {
		std::string pluginName = GetPluginNameFromPath(pluginPath);

		// Check if already loaded
		if (plugins.find(pluginName) != plugins.end()) {
			std::cout << "[PluginManager] Plugin already loaded: " << pluginName << std::endl;
			return true;
		}

		PluginInfo info;
		info.name = pluginName;
		info.path = pluginPath;

		std::cout << "[PluginManager] Loading plugin: " << pluginName << std::endl;

		// Load the library
		info.handle = LOAD_LIBRARY(pluginPath.c_str());
		if (!info.handle) {
			info.hasError = true;
			info.errorMessage = "Failed to load library: " + GetLastSystemError();
			std::cerr << "[PluginManager] " << info.errorMessage << std::endl;

			if (errorCallback) {
				errorCallback(pluginName, info.errorMessage);
			}
			return false;
		}

		// Get function pointers
		if (!ValidatePluginFunctions(info)) {
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
			return false;
		}

		// Create plugin instance
		BasePlugin* rawPlugin = info.createFunc();
		if (!rawPlugin) {
			info.hasError = true;
			info.errorMessage = "Failed to create plugin instance";
			std::cerr << "[PluginManager] " << info.errorMessage << std::endl;
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
			return false;
		}

		info.instance = std::shared_ptr<BasePlugin>(rawPlugin, [destroyFunc = info.destroyFunc](BasePlugin* p) {
			if (p && destroyFunc) {
				destroyFunc(p);
			}
		});

		// Initialize the plugin using YOUR BasePlugin interface
		bool initSuccess = false;
		try {
			if (viewManager) {
				initSuccess = info.instance->Initialize(entityManager, viewManager);
			}
			else {
				initSuccess = info.instance->Initialize(entityManager, nullptr);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Exception during plugin initialization: " << e.what() << std::endl;
			initSuccess = false;
		}

		if (!initSuccess) {
			info.hasError = true;
			info.errorMessage = "Plugin initialization failed";
			std::cerr << "[PluginManager] " << info.errorMessage << std::endl;
			info.instance.reset();
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
			return false;
		}

		info.isLoaded = true;
		info.lastWriteTime = GetFileWriteTime(pluginPath);

		// Store the plugin
		plugins[pluginName] = std::move(info);

		std::cout << "[PluginManager] Plugin loaded successfully: " << pluginName << std::endl;

		// Call load callback
		if (loadCallback) {
			loadCallback(pluginName, false);
		}

		return true;
	}

	bool PluginManager::UnloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		UnloadPluginInternal(pluginName);
		return true;
	}

	void PluginManager::UnloadPluginInternal(const std::string& name) {
		auto it = plugins.find(name);
		if (it == plugins.end()) {
			return;
		}

		PluginInfo& info = it->second;

		std::cout << "[PluginManager] Unloading plugin: " << name << std::endl;

		// Shutdown the plugin
		if (info.instance) {
			try {
				info.instance->Shutdown();
			}
			catch (const std::exception& e) {
				std::cerr << "[PluginManager] Exception during plugin shutdown: " << e.what() << std::endl;
			}
			info.instance.reset();
		}

		// Unload the library
		if (info.handle) {
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
		}

		// Clean up temp file if exists
		if (!info.tempPath.empty()) {
			CleanupTempFile(info.tempPath);
		}

		// Call unload callback
		if (unloadCallback) {
			unloadCallback(name);
		}

		// Remove from map
		plugins.erase(it);

		std::cout << "[PluginManager] Plugin unloaded: " << name << std::endl;
	}

	bool PluginManager::ReloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			return false;
		}

		std::string pluginPath = it->second.path;
		UnloadPluginInternal(pluginName);
		return LoadPluginInternal(pluginPath);
	}

	void PluginManager::UnloadAllPlugins() {
		std::lock_guard<std::mutex> lock(pluginMutex);

		std::cout << "[PluginManager] Unloading all plugins..." << std::endl;

		// Create a copy of plugin names to avoid iterator invalidation
		std::vector<std::string> pluginNames;
		for (const auto& pair : plugins) {
			pluginNames.push_back(pair.first);
		}

		// Unload each plugin
		for (const std::string& name : pluginNames) {
			UnloadPluginInternal(name);
		}

		plugins.clear();
		std::cout << "[PluginManager] All plugins unloaded" << std::endl;
	}

	void PluginManager::Update(float deltaTime) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		// Update all loaded plugins
		for (auto& pair : plugins) {
			PluginInfo& info = pair.second;
			if (info.isLoaded && info.instance) {
				try {
					info.instance->Update(deltaTime);
				}
				catch (const std::exception& e) {
					info.hasError = true;
					info.errorMessage = "Exception in plugin update: " + std::string(e.what());

					if (errorCallback) {
						errorCallback(info.name, info.errorMessage);
					}

					std::cerr << "[PluginManager] " << info.errorMessage << std::endl;
				}
			}
		}
	}

	bool PluginManager::IsPluginLoaded(const std::string& name) const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		auto it = plugins.find(name);
		return (it != plugins.end()) && it->second.isLoaded;
	}

	std::vector<std::string> PluginManager::GetLoadedPluginNames() const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		std::vector<std::string> names;
		for (const auto& pair : plugins) {
			if (pair.second.isLoaded) {
				names.push_back(pair.first);
			}
		}
		return names;
	}

	PluginManager::PluginInfo* PluginManager::GetPluginInfo(const std::string& name) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		auto it = plugins.find(name);
		return (it != plugins.end()) ? &it->second : nullptr;
	}

	void PluginManager::StartHotReload(const std::string& watchDir, std::chrono::milliseconds interval) {
		if (hotReloadActive) {
			StopHotReload();
		}

		watchDirectory = watchDir;
		checkInterval = interval;
		shouldStopWatching.store(false);
		hotReloadActive = true;

		watchThread = std::thread(&PluginManager::WatchForChanges, this);

		std::cout << "[PluginManager] Hot reload started for directory: " << watchDir << std::endl;
	}

	void PluginManager::StopHotReload() {
		if (!hotReloadActive) return;

		shouldStopWatching.store(true);
		hotReloadActive = false;

		if (watchThread.joinable()) {
			watchThread.join();
		}

		std::cout << "[PluginManager] Hot reload stopped" << std::endl;
	}

	void PluginManager::RefreshPluginDirectory() {
		if (watchDirectory.empty() || !std::filesystem::exists(watchDirectory)) {
			return;
		}

		std::cout << "[PluginManager] Refreshing plugin directory: " << watchDirectory << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(watchDirectory)) {
			if (entry.is_regular_file() && IsPluginFile(entry.path().string())) {
				std::string pluginName = GetPluginNameFromPath(entry.path().string());

				// Only load if not already loaded
				if (plugins.find(pluginName) == plugins.end()) {
					std::cout << "[PluginManager] Found new plugin: " << entry.path().string() << std::endl;
					LoadPlugin(entry.path().string());
				}
			}
		}
	}

	void PluginManager::WatchForChanges() {
		while (!shouldStopWatching.load()) {
			CheckForPluginChanges();
			std::this_thread::sleep_for(checkInterval);
		}
	}

	void PluginManager::CheckForPluginChanges() {
		if (!std::filesystem::exists(watchDirectory)) {
			return;
		}

		std::lock_guard<std::mutex> lock(pluginMutex);

		for (const auto& entry : std::filesystem::directory_iterator(watchDirectory)) {
			if (entry.is_regular_file() && IsPluginFile(entry.path().string())) {
				std::string pluginName = GetPluginNameFromPath(entry.path().string());
				auto currentWriteTime = GetFileWriteTime(entry.path().string());

				auto it = plugins.find(pluginName);
				if (it != plugins.end()) {
					// Check if file has been modified
					if (currentWriteTime > it->second.lastWriteTime) {
						std::cout << "[PluginManager] Plugin modified, reloading: " << pluginName << std::endl;

						// Wait a bit to ensure file is fully written
						std::this_thread::sleep_for(std::chrono::milliseconds(100));

						// Skip if file is still in use
						if (IsFileInUse(entry.path().string())) {
							continue;
						}

						// Reload the plugin
						std::string pluginPath = it->second.path;
						UnloadPluginInternal(pluginName);
						LoadPluginInternal(pluginPath);
					}
				}
				else {
					// New plugin found
					std::cout << "[PluginManager] New plugin detected: " << pluginName << std::endl;
					LoadPluginInternal(entry.path().string());
				}
			}
		}
	}

	bool PluginManager::ValidatePluginFunctions(PluginInfo& info) {
		// Get required function pointers
		info.createFunc = (PluginInfo::CreatePluginFunc)GET_PROC_ADDRESS(info.handle, "CreatePlugin");
		info.destroyFunc = (PluginInfo::DestroyPluginFunc)GET_PROC_ADDRESS(info.handle, "DestroyPlugin");
		info.getNameFunc = (PluginInfo::GetPluginNameFunc)GET_PROC_ADDRESS(info.handle, "GetPluginName");
		info.getVersionFunc = (PluginInfo::GetPluginVersionFunc)GET_PROC_ADDRESS(info.handle, "GetPluginVersion");

		if (!info.createFunc || !info.destroyFunc || !info.getNameFunc || !info.getVersionFunc) {
			info.hasError = true;
			info.errorMessage = "Missing required plugin functions";
			std::cerr << "[PluginManager] " << info.errorMessage << std::endl;
			return false;
		}

		return true;
	}

	std::string PluginManager::GetPluginNameFromPath(const std::string& path) {
		std::filesystem::path p(path);
		return p.stem().string();
	}

	bool PluginManager::IsPluginFile(const std::string& path) {
		std::filesystem::path p(path);
		std::string ext = p.extension().string();

#ifdef _WIN32
		return ext == ".dll";
#else
		return ext == ".so";
#endif
	}

	std::filesystem::file_time_type PluginManager::GetFileWriteTime(const std::string& path) {
		try {
			return std::filesystem::last_write_time(path);
		}
		catch (...) {
			return std::filesystem::file_time_type{};
		}
	}

	std::string PluginManager::GetLastSystemError() {
#ifdef _WIN32
		DWORD error = GetLastError();
		if (error == 0) return "Unknown error";

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);
		LocalFree(messageBuffer);
		return message;
#else
		return dlerror() ? dlerror() : "Unknown error";
#endif
	}

	bool PluginManager::IsFileInUse(const std::string& filePath) {
#ifdef _WIN32
		HANDLE handle = CreateFileA(
			filePath.c_str(),
			GENERIC_READ,
			0,  // No sharing
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (handle == INVALID_HANDLE_VALUE) {
			return GetLastError() == ERROR_SHARING_VIOLATION;
		}

		CloseHandle(handle);
		return false;
#else
		// On Unix systems, we can typically load the same file multiple times
		return false;
#endif
	}

	std::string PluginManager::CreateTempCopy(const std::string& originalPath) {
		try {
			std::filesystem::path original(originalPath);
			std::filesystem::path temp = original.parent_path() / (original.stem().string() + "_temp" + original.extension().string());

			std::filesystem::copy_file(original, temp, std::filesystem::copy_options::overwrite_existing);
			return temp.string();
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Failed to create temp copy: " << e.what() << std::endl;
			return "";
		}
	}

	void PluginManager::CleanupTempFile(const std::string& tempPath) {
		try {
			if (std::filesystem::exists(tempPath)) {
				std::filesystem::remove(tempPath);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Failed to cleanup temp file: " << e.what() << std::endl;
		}
	}

} // namespace Plugin