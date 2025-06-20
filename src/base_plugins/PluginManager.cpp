/*
	FIXED PluginManager.cpp - DEADLOCK RESOLUTION
	- Removes double mutex locking that was causing resource deadlock
	- Simplifies the locking strategy
	- Fixes mutex contention issues
*/

#include "PluginManager.hpp"
#include "PluginRegistry.hpp"
#include "PluginInterface.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "FilePaths.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <typeindex>
#include <random>

namespace Plugin {

	// Static storage for current manager instances during plugin loading
	static ECS::EntityManager* s_currentEntityManager = nullptr;
	static GUI::ViewManager* s_currentViewManager = nullptr;
	static std::atomic<bool> s_managersValid = true;

	// Static C-style functions that plugins can call
	extern "C" {
		static ECS::EntityManager* GetCurrentEntityManager() {
			if (!s_managersValid.load()) {
				std::cout << "Plugin requesting EntityManager during shutdown - returning nullptr" << std::endl;
				return nullptr;
			}
			std::cout << "Plugin requesting EntityManager: " << s_currentEntityManager << std::endl;
			return s_currentEntityManager;
		}

		static GUI::ViewManager* GetCurrentViewManager() {
			if (!s_managersValid.load()) {
				std::cout << "Plugin requesting ViewManager during shutdown - returning nullptr" << std::endl;
				return nullptr;
			}
			std::cout << "Plugin requesting ViewManager: " << s_currentViewManager << std::endl;
			return s_currentViewManager;
		}
	}

	// C-style function types for plugin interface
	typedef Plugin::BasePlugin* (*CreatePluginFunc)();
	typedef void(*DestroyPluginFunc)(Plugin::BasePlugin*);
	typedef const char* (*GetPluginNameFunc)();
	typedef const char* (*GetPluginVersionFunc)();
	typedef void(*SetManagerGettersFunc)(ECS::EntityManager* (*)(), GUI::ViewManager* (*)());

	PluginManager::PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
		: entityManager(entityMgr)
		, viewManager(viewMgr)
		, hotReloadActive(false)
		, shouldStopWatching(false)
		, watchDirectory(Utils::FilePaths::pluginPath)
		, checkInterval(std::chrono::milliseconds(500)) {

		std::cout << "PluginManager constructor - EntityManager: " << &entityManager
			<< ", ViewManager: " << &viewManager << std::endl;

		// Initialize the PluginRegistry with our managers
		PluginRegistry::Initialize(&entityManager, &viewManager);

		// Set static managers as valid initially
		s_managersValid.store(true);
	}

	PluginManager::~PluginManager() {
		std::cout << "PluginManager destructor called" << std::endl;

		// CRITICAL: Mark managers as invalid BEFORE starting shutdown
		s_managersValid.store(false);
		s_currentEntityManager = nullptr;
		s_currentViewManager = nullptr;

		// Stop hot reload first
		StopHotReload();

		// Unload all plugins
		UnloadAllPlugins();
	}

	void PluginManager::Init() {
		std::cout << "PluginManager::Init() called" << std::endl;

		// Set the watch directory to the plugins path
		SetWatchDirectory(Utils::FilePaths::pluginPath);

		// Scan for existing plugins in the directory
		RefreshPluginDirectory();

		std::cout << "PluginManager initialized with watch directory: " << watchDirectory << std::endl;
	}

	bool PluginManager::LoadPlugin(const std::string& pluginPath) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		// FIXED: Call the internal method WITHOUT taking another lock
		return LoadPluginInternal(pluginPath, false);
	}

	bool PluginManager::LoadPluginInternal(const std::string& pluginPath, bool isReload) {

		try {
			std::cout << "=== LOADING PLUGIN START ===" << std::endl;

			PluginInfo info;
			info.path = pluginPath;
			info.name = GetPluginNameFromPath(pluginPath);

			std::cout << "Plugin name: " << info.name << std::endl;

			// Load the library
			std::cout << "Loading library..." << std::endl;
			info.handle = LOAD_LIBRARY(pluginPath.c_str());
			if (!info.handle) {
				info.hasError = true;
				info.errorMessage = "Failed to load library: " + GetLastSystemError();
				std::cerr << info.errorMessage << std::endl;
				return false;
			}

			// Get function pointers
			std::cout << "Getting function pointers..." << std::endl;
			if (!ValidatePluginFunctions(info)) {
				UNLOAD_LIBRARY(info.handle);
				info.handle = nullptr;
				return false;
			}

			// Create plugin instance
			std::cout << "Creating plugin instance..." << std::endl;
			BasePlugin* rawPlugin = info.createFunc();
			if (!rawPlugin) {
				info.hasError = true;
				info.errorMessage = "CreatePlugin returned nullptr";
				std::cerr << info.errorMessage << std::endl;
				UNLOAD_LIBRARY(info.handle);
				return false;
			}

			info.instance = std::shared_ptr<BasePlugin>(rawPlugin);

			// Set up manager getters for plugin
			std::cout << "Setting up manager getters for plugin..." << std::endl;

			// Get the SetManagerGetters function from the plugin
			typedef void(*SetManagerGettersFunc)(
				GetEntityManagerFunc,
				GetViewManagerFunc,
				GetImGuiContextFunc,
				GetImGuiAllocFunc,
				GetImGuiFreeFunc,
				GetImGuiUserDataFunc
				);
			SetManagerGettersFunc setManagerGetters =
				(SetManagerGettersFunc)GET_PROC_ADDRESS(info.handle, "SetManagerGetters");

			if (setManagerGetters) {
				std::cout << "Found SetManagerGetters function in plugin, calling it..." << std::endl;

				setManagerGetters(
					GetHostEntityManager,
					GetHostViewManager,
					GetHostImGuiContext,
					GetHostImGuiAllocFunc,
					GetHostImGuiFreeFunc,
					GetHostImGuiUserData
				);

				std::cout << "Manager getters and ImGui context set successfully" << std::endl;
			}
			else {
				std::cout << "SetManagerGetters function not found in plugin" << std::endl;
			}

			// Initialize the plugin
			std::cout << "Initializing plugin..." << std::endl;
			if (!info.instance->Initialize(entityManager, viewManager)) {
				info.hasError = true;
				info.errorMessage = "Plugin initialization failed";
				std::cerr << info.errorMessage << std::endl;

				// Clean up
				info.instance.reset();
				UNLOAD_LIBRARY(info.handle);
				return false;
			}

			info.isLoaded = true;
			info.hasError = false;
			info.lastWriteTime = GetFileWriteTime(pluginPath);

			std::string pluginName = info.name;
			std::string pluginVersion = info.instance->GetVersion();
			bool isLoaded = info.isLoaded;
			bool hasError = info.hasError;

			// Store the plugin info
			plugins[info.name] = std::move(info);

			std::cout << "=== PLUGIN LOADED SUCCESSFULLY ===" << std::endl;
			std::cout << "Plugin: " << pluginName << std::endl;
			std::cout << "Version: " << pluginVersion << std::endl;
			std::cout << "IsLoaded: " << isLoaded << std::endl;
			std::cout << "HasError: " << hasError << std::endl;

			// Call load callback
			if (loadCallback) {
				std::cout << "Calling load callback..." << std::endl;
				loadCallback(pluginName, true);
				std::cout << "Load callback completed successfully" << std::endl;
			}

			std::cout << "=== PLUGIN LOAD COMPLETE ===" << std::endl;
			return true;

		}
		catch (const std::exception& e) {
			std::cerr << "Exception during plugin loading: " << e.what() << std::endl;
			return false;
		}
	}

	bool PluginManager::UnloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		UnloadPluginInternal(pluginName);
		return true;
	}

	void PluginManager::UnloadPluginInternal(const std::string& pluginName) {
		// FIXED: No mutex lock needed here - caller already has the lock
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			std::cout << "Plugin " << pluginName << " not found for unloading" << std::endl;
			return;
		}

		PluginInfo& info = it->second;

		std::cout << "Unloading plugin: " << pluginName << std::endl;

		// Shutdown the plugin instance
		if (info.instance) {
			try {
				info.instance->Shutdown();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during plugin shutdown: " << e.what() << std::endl;
			}
			info.instance.reset();
		}

		// Cleanup plugin registry entries
		PluginRegistry::CleanupPlugin(pluginName);

		// Unload the library
		if (info.handle) {
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
		}

		// Clean up temp file if exists
		if (!info.tempPath.empty()) {
			CleanupTempFile(info.tempPath);
			info.tempPath.clear();
		}

		info.isLoaded = false;
		info.hasError = false;
		info.errorMessage.clear();

		// Notify callback
		if (unloadCallback) {
			unloadCallback(pluginName);
		}

		std::cout << "Plugin " << pluginName << " unloaded successfully" << std::endl;
	}

	bool PluginManager::ReloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			std::cerr << "Plugin " << pluginName << " not found for reloading" << std::endl;
			return false;
		}

		std::string pluginPath = it->second.path;

		// Unload first (no mutex needed, we already have it)
		UnloadPluginInternal(pluginName);

		// Small delay to ensure file handles are released
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Load again (no mutex needed, we already have it)
		return LoadPluginInternal(pluginPath, true);
	}

	void PluginManager::UnloadAllPlugins() {
		std::lock_guard<std::mutex> lock(pluginMutex);

		std::cout << "Unloading all plugins..." << std::endl;

		// Mark managers as invalid before unloading any plugins
		s_managersValid.store(false);

		for (auto&[name, info] : plugins) {
			if (info.isLoaded) {
				UnloadPluginInternal(name);
			}
		}

		plugins.clear();
		std::cout << "All plugins unloaded" << std::endl;
	}

	void PluginManager::StartHotReload(const std::string& watchDir, std::chrono::milliseconds interval) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		if (hotReloadActive) {
			std::cout << "Hot reload already active" << std::endl;
			return;
		}

		watchDirectory = watchDir;
		checkInterval = interval;
		shouldStopWatching = false;
		hotReloadActive = true;

		watchThread = std::thread(&PluginManager::WatchForChanges, this);

		std::cout << "Started hot reload for directory: " << watchDirectory
			<< " with interval: " << interval.count() << "ms" << std::endl;
	}

	void PluginManager::StopHotReload() {
		if (!hotReloadActive) {
			return;
		}

		std::cout << "Stopping hot reload..." << std::endl;

		shouldStopWatching = true;
		hotReloadActive = false;

		if (watchThread.joinable()) {
			watchThread.join();
		}

		std::cout << "Hot reload stopped" << std::endl;
	}

	void PluginManager::Update(float deltaTime) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		// Only update if managers are valid
		if (!s_managersValid.load()) {
			return;
		}

		// Update all loaded plugins
		for (auto&[name, info] : plugins) {
			if (info.isLoaded && info.instance) {
				try {
					info.instance->Update(deltaTime);
				}
				catch (const std::exception& e) {
					std::cerr << "Exception in plugin " << name << " update: " << e.what() << std::endl;
				}
			}
		}
	}

	void PluginManager::RefreshPluginDirectory() {
		if (!std::filesystem::exists(watchDirectory)) {
			std::filesystem::create_directories(watchDirectory);
			std::cout << "Created plugin directory: " << watchDirectory << std::endl;
			return;
		}

		std::cout << "Refreshing plugin directory: " << watchDirectory << std::endl;

		try {
			// FIXED: Don't take a lock here if called from other methods that already have it
			std::lock_guard<std::mutex> lock(pluginMutex);

			for (const auto& entry : std::filesystem::directory_iterator(watchDirectory)) {
				if (entry.is_regular_file() && IsPluginFile(entry.path().string())) {
					std::string pluginName = GetPluginNameFromPath(entry.path().string());

					// Add to plugins map if not already present
					if (plugins.find(pluginName) == plugins.end()) {
						PluginInfo info;
						info.name = pluginName;
						info.path = entry.path().string();
						info.lastWriteTime = GetFileWriteTime(info.path);
						info.isLoaded = false;

						plugins[pluginName] = info;
						std::cout << "Found plugin file: " << info.path << std::endl;
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

		std::vector<PluginInfo> result;
		for (const auto&[name, info] : plugins) {
			result.push_back(info);
		}
		return result;
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

		Stats stats;
		stats.totalPlugins = plugins.size();

		for (const auto&[name, info] : plugins) {
			if (info.isLoaded) {
				stats.loadedPlugins++;
			}
			if (info.hasError) {
				stats.errorPlugins++;
			}
		}

		stats.lastCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch());

		return stats;
	}

	bool PluginManager::ValidatePluginFunctions(PluginInfo& info) {
		// Get function pointers
		info.createFunc = (PluginInfo::CreatePluginFunc)GET_PROC_ADDRESS(info.handle, "CreatePlugin");
		info.destroyFunc = (PluginInfo::DestroyPluginFunc)GET_PROC_ADDRESS(info.handle, "DestroyPlugin");
		info.getNameFunc = (PluginInfo::GetPluginNameFunc)GET_PROC_ADDRESS(info.handle, "GetPluginName");
		info.getVersionFunc = (PluginInfo::GetPluginVersionFunc)GET_PROC_ADDRESS(info.handle, "GetPluginVersion");

		if (!info.createFunc) {
			std::cerr << "Plugin missing CreatePlugin function" << std::endl;
			info.hasError = true;
			info.errorMessage = "Missing CreatePlugin function";
			return false;
		}

		if (!info.destroyFunc) {
			std::cerr << "Plugin missing DestroyPlugin function" << std::endl;
			info.hasError = true;
			info.errorMessage = "Missing DestroyPlugin function";
			return false;
		}

		if (!info.getNameFunc) {
			std::cerr << "Plugin missing GetPluginName function" << std::endl;
			info.hasError = true;
			info.errorMessage = "Missing GetPluginName function";
			return false;
		}

		return true;
	}

	void PluginManager::WatchForChanges() {
		std::cout << "Hot reload watcher thread started" << std::endl;

		while (!shouldStopWatching) {
			try {
				CheckForPluginChanges();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in hot reload watcher: " << e.what() << std::endl;
			}

			std::this_thread::sleep_for(checkInterval);
		}

		std::cout << "Hot reload watcher thread stopped" << std::endl;
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
						std::cout << "Detected change in plugin: " << pluginName << std::endl;

						// Update the write time first
						it->second.lastWriteTime = currentWriteTime;

						// Reload if currently loaded - NO RECURSIVE LOCKING
						if (it->second.isLoaded) {
							std::cout << "Hot reloading plugin: " << pluginName << std::endl;

							std::string pluginPath = it->second.path;

							// Unload first
							UnloadPluginInternal(pluginName);

							// Small delay to ensure file handles are released
							std::this_thread::sleep_for(std::chrono::milliseconds(100));

							// Load again
							LoadPluginInternal(pluginPath, true);
							return;
						}
					}
				}
			}
		}
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
		catch (const std::exception&) {
			return std::filesystem::file_time_type{};
		}
	}

	std::string PluginManager::GetLastSystemError() {
#ifdef _WIN32
		DWORD error = GetLastError();
		LPSTR buffer = nullptr;

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&buffer, 0, nullptr);

		std::string result = buffer ? buffer : "Unknown error";
		if (buffer) {
			LocalFree(buffer);
		}
		return result;
#else
		return dlerror() ? dlerror() : "Unknown error";
#endif
	}

	bool PluginManager::IsFileInUse(const std::string& filePath) {
#ifdef _WIN32
		HANDLE file = CreateFileA(
			filePath.c_str(),
			GENERIC_READ,
			0, // No sharing - if file is in use, this will fail
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);

		if (file == INVALID_HANDLE_VALUE) {
			DWORD error = GetLastError();
			CloseHandle(file);
			return (error == ERROR_SHARING_VIOLATION);
		}

		CloseHandle(file);
		return false;
#else
		return (rename(filePath.c_str(), filePath.c_str()) != 0);
#endif
	}

	std::string PluginManager::CreateTempCopy(const std::string& originalPath) {
		try {
			std::filesystem::path original(originalPath);

			// Generate a unique temporary filename
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(1000, 9999);

			std::string tempName = original.stem().string() + "_temp_" + std::to_string(dis(gen)) + original.extension().string();
			std::filesystem::path tempPath = original.parent_path() / tempName;

			// Copy the file
			std::filesystem::copy_file(original, tempPath, std::filesystem::copy_options::overwrite_existing);

			std::cout << "Created temporary copy: " << tempPath.string() << std::endl;
			return tempPath.string();
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to create temporary copy: " << e.what() << std::endl;
			return "";
		}
	}

	void PluginManager::CleanupTempFile(const std::string& tempPath) {
		if (tempPath.empty()) {
			return;
		}

		try {
			if (std::filesystem::exists(tempPath)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				std::filesystem::remove(tempPath);
				std::cout << "Cleaned up temporary file: " << tempPath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to cleanup temporary file " << tempPath << ": " << e.what() << std::endl;
		}
	}

} // namespace Plugin