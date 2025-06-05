#include "PluginManager.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>

namespace Plugin {

	PluginManager::PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
		: entityManager(entityMgr), viewManager(viewMgr), hotReloadActive(false), shouldStopWatching(false),
		checkInterval(std::chrono::milliseconds(500)) {

		std::cout << "PluginManager initialized" << std::endl;
	}

	PluginManager::~PluginManager() {
		StopHotReload();
		UnloadAllPlugins();
	}

	bool PluginManager::LoadPlugin(const std::string& pluginPath) {
		return LoadPluginInternal(pluginPath, false);
	}

	bool PluginManager::LoadPluginInternal(const std::string& pluginPath, bool isReload) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		if (!std::filesystem::exists(pluginPath)) {
			std::cerr << "Plugin file does not exist: " << pluginPath << std::endl;
			return false;
		}

		std::string pluginName = GetPluginNameFromPath(pluginPath);

		// If reloading, unload existing plugin first
		if (isReload && plugins.find(pluginName) != plugins.end()) {
			UnloadPluginInternal(pluginName);
		}

		PluginInfo info;
		info.name = pluginName;
		info.path = pluginPath;
		info.lastWriteTime = GetFileWriteTime(pluginPath);

		// Create a temporary copy for hot reload to avoid file locking issues
		std::string actualPath = pluginPath;
		std::string tempPath;
		if (isReload) {
			CopyPluginForReload(pluginPath, tempPath);
			actualPath = tempPath;
		}

		// Load the library
		info.handle = LOAD_LIBRARY(actualPath.c_str());
		if (!info.handle) {
			info.hasError = true;
			info.errorMessage = "Failed to load library: " + GetLastSystemError();
			std::cerr << "Error loading plugin " << pluginName << ": " << info.errorMessage << std::endl;

			if (!tempPath.empty()) {
				CleanupTempFile(tempPath);
			}

			plugins[pluginName] = info;
			return false;
		}

		// Get function pointers
		info.createFunc = (PluginInfo::CreatePluginFunc)GET_PROC_ADDRESS(info.handle, "CreatePlugin");
		info.destroyFunc = (PluginInfo::DestroyPluginFunc)GET_PROC_ADDRESS(info.handle, "DestroyPlugin");
		info.getNameFunc = (PluginInfo::GetPluginNameFunc)GET_PROC_ADDRESS(info.handle, "GetPluginName");
		info.getVersionFunc = (PluginInfo::GetPluginVersionFunc)GET_PROC_ADDRESS(info.handle, "GetPluginVersion");

		if (!ValidatePluginFunctions(info)) {
			info.hasError = true;
			info.errorMessage = "Missing required plugin functions";
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;

			if (!tempPath.empty()) {
				CleanupTempFile(tempPath);
			}

			plugins[pluginName] = info;
			return false;
		}

		// Create plugin instance
		try {
			BasePlugin* pluginPtr = info.createFunc();
			if (!pluginPtr) {
				info.hasError = true;
				info.errorMessage = "Plugin creation function returned null";
				UNLOAD_LIBRARY(info.handle);
				info.handle = nullptr;

				if (!tempPath.empty()) {
					CleanupTempFile(tempPath);
				}

				plugins[pluginName] = info;
				return false;
			}

			info.instance = std::shared_ptr<BasePlugin>(pluginPtr, [this, destroyFunc = info.destroyFunc](BasePlugin* ptr) {
				if (ptr && destroyFunc) {
					destroyFunc(ptr);
				}
			});

			// Initialize the plugin
			info.instance->Initialize(entityManager, viewManager);
			info.isLoaded = true;
			info.hasError = false;

			plugins[pluginName] = info;

			std::cout << "Successfully loaded plugin: " << pluginName;
			if (info.getNameFunc) {
				std::cout << " (" << info.getNameFunc() << ")";
			}
			if (info.getVersionFunc) {
				std::cout << " v" << info.getVersionFunc();
			}
			std::cout << std::endl;

			// Notify callback
			if (loadCallback) {
				loadCallback(pluginName, true);
			}

			// Clean up temp file if used
			if (!tempPath.empty()) {
				CleanupTempFile(tempPath);
			}

			return true;
		}
		catch (const std::exception& e) {
			info.hasError = true;
			info.errorMessage = "Exception during plugin initialization: " + std::string(e.what());
			std::cerr << "Error initializing plugin " << pluginName << ": " << info.errorMessage << std::endl;

			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;

			if (!tempPath.empty()) {
				CleanupTempFile(tempPath);
			}

			plugins[pluginName] = info;

			if (loadCallback) {
				loadCallback(pluginName, false);
			}

			return false;
		}
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

		try {
			// Shutdown the plugin
			if (info.instance && info.isLoaded) {
				info.instance->Shutdown();
				info.instance.reset();
			}

			// Unload the library
			if (info.handle) {
				UNLOAD_LIBRARY(info.handle);
				info.handle = nullptr;
			}

			std::cout << "Successfully unloaded plugin: " << name << std::endl;

			// Notify callback
			if (unloadCallback) {
				unloadCallback(name);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error unloading plugin " << name << ": " << e.what() << std::endl;
		}

		plugins.erase(it);
	}

	bool PluginManager::ReloadPlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			std::cerr << "Cannot reload plugin " << pluginName << ": not found" << std::endl;
			return false;
		}

		std::string pluginPath = it->second.path;
		std::cout << "Reloading plugin: " << pluginName << std::endl;

		return LoadPluginInternal(pluginPath, true);
	}

	void PluginManager::UnloadAllPlugins() {
		std::lock_guard<std::mutex> lock(pluginMutex);

		std::cout << "Unloading all plugins..." << std::endl;

		for (auto&[name, info] : plugins) {
			try {
				if (info.instance && info.isLoaded) {
					info.instance->Shutdown();
					info.instance.reset();
				}

				if (info.handle) {
					UNLOAD_LIBRARY(info.handle);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error unloading plugin " << name << ": " << e.what() << std::endl;
			}
		}

		plugins.clear();
	}

	void PluginManager::StartHotReload(const std::string& watchDir, std::chrono::milliseconds interval) {
		if (hotReloadActive) {
			std::cout << "Hot reload already active" << std::endl;
			return;
		}

		watchDirectory = watchDir;
		checkInterval = interval;
		shouldStopWatching = false;
		hotReloadActive = true;

		// Create watch directory if it doesn't exist
		if (!std::filesystem::exists(watchDirectory)) {
			std::filesystem::create_directories(watchDirectory);
		}

		// Start watching thread
		watchThread = std::thread(&PluginManager::WatchForChanges, this);

		std::cout << "Started hot reload watching: " << watchDirectory
			<< " (interval: " << interval.count() << "ms)" << std::endl;
	}

	void PluginManager::StopHotReload() {
		if (!hotReloadActive) {
			return;
		}

		shouldStopWatching = true;
		hotReloadActive = false;

		if (watchThread.joinable()) {
			watchThread.join();
		}

		std::cout << "Stopped hot reload" << std::endl;
	}

	void PluginManager::Update(float deltaTime) {
		// Update all loaded plugins
		std::lock_guard<std::mutex> lock(pluginMutex);

		for (auto&[name, info] : plugins) {
			if (info.instance && info.isLoaded && !info.hasError) {
				try {
					info.instance->Update(deltaTime);
				}
				catch (const std::exception& e) {
					std::cerr << "Error updating plugin " << name << ": " << e.what() << std::endl;
					info.hasError = true;
					info.errorMessage = "Update error: " + std::string(e.what());
				}
			}
		}
	}

	void PluginManager::RefreshPluginDirectory() {
		if (!std::filesystem::exists(watchDirectory)) {
			return;
		}

		try {
			for (const auto& entry : std::filesystem::directory_iterator(watchDirectory)) {
				if (entry.is_regular_file() && IsPluginFile(entry.path().string())) {
					std::string pluginName = GetPluginNameFromPath(entry.path().string());

					std::lock_guard<std::mutex> lock(pluginMutex);
					auto it = plugins.find(pluginName);

					if (it == plugins.end()) {
						// New plugin found
						std::cout << "Found new plugin: " << entry.path().string() << std::endl;
						// Note: Auto-loading new plugins can be enabled here if desired
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error refreshing plugin directory: " << e.what() << std::endl;
		}
	}

	std::vector<std::string> PluginManager::GetLoadedPluginNames() const {
		std::lock_guard<std::mutex> lock(pluginMutex);

		std::vector<std::string> names;
		for (const auto&[name, info] : plugins) {
			if (info.isLoaded) {
				names.push_back(name);
			}
		}
		return names;
	}

	std::vector<PluginManager::PluginInfo> PluginManager::GetAllPluginInfo() const {
		std::lock_guard<std::mutex> lock(pluginMutex);

		std::vector<PluginInfo> infos;
		for (const auto&[name, info] : plugins) {
			infos.push_back(info);
		}
		return infos;
	}

	PluginManager::PluginInfo* PluginManager::GetPluginInfo(const std::string& name) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(name);
		return (it != plugins.end()) ? &it->second : nullptr;
	}

	bool PluginManager::IsPluginLoaded(const std::string& name) const {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(name);
		return (it != plugins.end()) && it->second.isLoaded;
	}

	PluginManager::Stats PluginManager::GetStats() const {
		std::lock_guard<std::mutex> lock(pluginMutex);

		Stats stats = {};
		stats.totalPlugins = plugins.size();

		for (const auto&[name, info] : plugins) {
			if (info.isLoaded) {
				stats.loadedPlugins++;
			}
			if (info.hasError) {
				stats.errorPlugins++;
			}
		}

		stats.lastCheckTime = checkInterval;
		return stats;
	}

	// Private methods

	bool PluginManager::ValidatePluginFunctions(PluginInfo& info) {
		return info.createFunc != nullptr && info.destroyFunc != nullptr;
	}

	void PluginManager::WatchForChanges() {
		while (!shouldStopWatching) {
			CheckForPluginChanges();
			std::this_thread::sleep_for(checkInterval);
		}
	}

	void PluginManager::CheckForPluginChanges() {
		try {
			if (!std::filesystem::exists(watchDirectory)) {
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(watchDirectory)) {
				if (!entry.is_regular_file() || !IsPluginFile(entry.path().string())) {
					continue;
				}

				std::string pluginPath = entry.path().string();
				std::string pluginName = GetPluginNameFromPath(pluginPath);
				auto currentWriteTime = GetFileWriteTime(pluginPath);

				std::lock_guard<std::mutex> lock(pluginMutex);
				auto it = plugins.find(pluginName);

				if (it != plugins.end()) {
					// Check if file has been modified
					if (currentWriteTime > it->second.lastWriteTime) {
						std::cout << "Plugin file changed, reloading: " << pluginName << std::endl;

						// Update the path in case it moved
						it->second.path = pluginPath;

						// Reload the plugin (unlock temporarily to avoid deadlock)
						lock.~lock_guard();
						ReloadPlugin(pluginName);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error checking for plugin changes: " << e.what() << std::endl;
		}
	}

	std::string PluginManager::GetPluginNameFromPath(const std::string& path) {
		std::filesystem::path p(path);
		std::string filename = p.stem().string();

		// Remove common plugin prefixes/suffixes - C++17 compatible
		const std::string libPrefix = "lib";
		if (filename.length() >= libPrefix.length() &&
			filename.compare(0, libPrefix.length(), libPrefix) == 0) {
			filename = filename.substr(libPrefix.length());
		}

		return filename;
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
		catch (const std::exception&) {
			return std::filesystem::file_time_type{};
		}
	}

	std::string PluginManager::GetLastSystemError() {
#ifdef _WIN32
		DWORD error = GetLastError();
		if (error == 0) return "No error";

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);
		LocalFree(messageBuffer);
		return message;
#else
		const char* error = dlerror();
		return error ? std::string(error) : "No error";
#endif
	}

	void PluginManager::CopyPluginForReload(const std::string& originalPath, std::string& tempPath) {
		std::filesystem::path original(originalPath);
		std::filesystem::path temp = original.parent_path() / ("temp_" + original.filename().string());

		try {
			std::filesystem::copy_file(original, temp, std::filesystem::copy_options::overwrite_existing);
			tempPath = temp.string();
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to create temp copy for reload: " << e.what() << std::endl;
			tempPath = originalPath; // Fallback to original
		}
	}

	void PluginManager::CleanupTempFile(const std::string& tempPath) {
		try {
			if (std::filesystem::exists(tempPath)) {
				std::filesystem::remove(tempPath);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to cleanup temp file " << tempPath << ": " << e.what() << std::endl;
		}
	}

} // namespace Plugin