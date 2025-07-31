/*
 * StudioPluginManager.cpp - Studio Plugin Manager Implementation
 */

#include "StudioPluginManager.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define access _access
#define mkdir _mkdir
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace Plugin {

	std::atomic<int> StudioPluginManager::tempFileCounter{ 0 };

	StudioPluginManager::StudioPluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
		: entityManager(&entityMgr), viewManager(&viewMgr) {

		// Set up studio context (both ECS and GUI)
		context.entityManager = &entityMgr;
		context.viewManager = &viewMgr;

		SetupTempDirectory();

		std::cout << "[StudioPluginManager] Initialized for Studio (ECS + GUI)" << std::endl;
	}

	StudioPluginManager::~StudioPluginManager() {
		StopHotReload();
		UnloadAllPlugins();
		CleanupTempDirectory();
	}

	void StudioPluginManager::SetupTempDirectory() {
#ifdef _WIN32
		char tempPath[MAX_PATH];
		GetTempPathA(MAX_PATH, tempPath);
		tempDirectory = std::string(tempPath) + "anistudio_plugins\\";
		_mkdir(tempDirectory.c_str());
#else
		tempDirectory = "/tmp/anistudio_plugins/";
		mkdir(tempDirectory.c_str(), 0755);
#endif
		std::cout << "[StudioPluginManager] Temp directory: " << tempDirectory << std::endl;
	}

	void StudioPluginManager::CleanupTempDirectory() {
		try {
			if (std::filesystem::exists(tempDirectory)) {
				std::filesystem::remove_all(tempDirectory);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioPluginManager] Failed to cleanup temp directory: " << e.what() << std::endl;
		}
	}

	bool StudioPluginManager::LoadPlugin(const std::string& pluginPath) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		return LoadPluginInternal(pluginPath, false);
	}

	bool StudioPluginManager::LoadPluginInternal(const std::string& pluginPath, bool isReload) {
		if (access(pluginPath.c_str(), 0) != 0) {
			std::cerr << "[StudioPluginManager] Plugin file not found: " << pluginPath << std::endl;
			return false;
		}

		std::string pluginName = GetPluginNameFromPath(pluginPath);

		if (!isReload && plugins.find(pluginName) != plugins.end()) {
			std::cout << "[StudioPluginManager] Plugin already loaded: " << pluginName << std::endl;
			return true;
		}

		if (isReload) {
			UnloadPluginInternal(pluginName);
		}

		PluginInfo info;
		info.name = pluginName;
		info.originalPath = pluginPath;
		info.lastWriteTime = GetFileWriteTime(pluginPath);

		std::cout << "[StudioPluginManager] " << (isReload ? "Reloading" : "Loading")
			<< " plugin: " << pluginName << std::endl;

		// Create temp copy for hot reload
		info.tempPath = CreateTempCopy(pluginPath);
		if (info.tempPath.empty()) {
			info.hasError = true;
			info.errorMessage = "Failed to create temp copy";
			std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
			return false;
		}

		// Load library
		info.handle = LOAD_LIBRARY(info.tempPath.c_str());
		if (!info.handle) {
			info.hasError = true;
			info.errorMessage = "Failed to load library: " + GetLastSystemError();
			std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
			return false;
		}

		if (!ValidatePluginFunctions(info)) {
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
			return false;
		}

		try {
			// Create plugin instance
			IPlugin* rawPlugin = info.createFunc();
			if (!rawPlugin) {
				info.hasError = true;
				info.errorMessage = "Failed to create plugin instance";
				std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
				UNLOAD_LIBRARY(info.handle);
				info.handle = nullptr;
				return false;
			}

			info.instance = std::unique_ptr<IPlugin>(rawPlugin);

			// Initialize plugin with full studio context (ECS + GUI)
			if (!info.instance->Initialize(&context)) {
				info.hasError = true;
				info.errorMessage = "Plugin initialization failed";
				std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
				info.instance.reset();
				UNLOAD_LIBRARY(info.handle);
				info.handle = nullptr;
				return false;
			}

			info.isLoaded = true;
			plugins[pluginName] = std::move(info);

			std::cout << "[StudioPluginManager] Plugin " << (isReload ? "reloaded" : "loaded")
				<< " successfully: " << pluginName << " (Studio mode - ECS + GUI available)" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			info.hasError = true;
			info.errorMessage = "Exception during plugin loading: " + std::string(e.what());
			std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
			return false;
		}
	}

	bool StudioPluginManager::UnloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		UnloadPluginInternal(pluginName);
		return true;
	}

	void StudioPluginManager::UnloadPluginInternal(const std::string& name) {
		auto it = plugins.find(name);
		if (it == plugins.end()) {
			return;
		}

		PluginInfo& info = it->second;
		std::cout << "[StudioPluginManager] Unloading plugin: " << name << std::endl;

		if (info.instance) {
			try {
				info.instance->Shutdown();
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioPluginManager] Exception during plugin shutdown: " << e.what() << std::endl;
			}
			info.instance.reset();
		}

		if (info.handle) {
			UNLOAD_LIBRARY(info.handle);
			info.handle = nullptr;
		}

		// Remove temp file
		if (!info.tempPath.empty() && std::filesystem::exists(info.tempPath)) {
			try {
				std::filesystem::remove(info.tempPath);
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioPluginManager] Failed to remove temp file: " << e.what() << std::endl;
			}
		}

		plugins.erase(it);
		std::cout << "[StudioPluginManager] Plugin unloaded: " << name << std::endl;
	}

	bool StudioPluginManager::ReloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			std::cerr << "[StudioPluginManager] Cannot reload - plugin not loaded: " << pluginName << std::endl;
			return false;
		}

		std::string originalPath = it->second.originalPath;
		return LoadPluginInternal(originalPath, true);
	}

	void StudioPluginManager::UnloadAllPlugins() {
		std::lock_guard<std::mutex> lock(pluginMutex);

		std::cout << "[StudioPluginManager] Unloading all plugins..." << std::endl;

		std::vector<std::string> pluginNames;
		for (const auto& pair : plugins) {
			pluginNames.push_back(pair.first);
		}

		for (const std::string& name : pluginNames) {
			UnloadPluginInternal(name);
		}

		plugins.clear();
		std::cout << "[StudioPluginManager] All plugins unloaded" << std::endl;
	}

	void StudioPluginManager::Update(float deltaTime) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		for (auto& pair : plugins) {
			PluginInfo& info = pair.second;
			if (info.isLoaded && info.instance) {
				try {
					info.instance->Update(deltaTime);
				}
				catch (const std::exception& e) {
					info.hasError = true;
					info.errorMessage = "Exception in plugin update: " + std::string(e.what());
					std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
				}
			}
		}
	}

	bool StudioPluginManager::IsPluginLoaded(const std::string& name) const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		auto it = plugins.find(name);
		return (it != plugins.end()) && it->second.isLoaded;
	}

	std::vector<std::string> StudioPluginManager::GetLoadedPluginNames() const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		std::vector<std::string> names;
		for (const auto& pair : plugins) {
			if (pair.second.isLoaded) {
				names.push_back(pair.first);
			}
		}
		return names;
	}

	StudioPluginManager::PluginInfo* StudioPluginManager::GetPluginInfo(const std::string& name) {
		std::lock_guard<std::mutex> lock(pluginMutex);
		auto it = plugins.find(name);
		return (it != plugins.end()) ? &it->second : nullptr;
	}

	void StudioPluginManager::StartHotReload(const std::string& watchDir) {
		if (hotReloadActive) {
			StopHotReload();
		}

		watchDirectory = watchDir;

		if (!std::filesystem::exists(watchDirectory)) {
			std::cerr << "[StudioPluginManager] Watch directory does not exist: " << watchDirectory << std::endl;
			return;
		}

		shouldStopWatching = false;
		hotReloadActive = true;

		watchThread = std::thread(&StudioPluginManager::WatchForChanges, this);

		std::cout << "[StudioPluginManager] Hot reload started for directory: " << watchDir << std::endl;
	}

	void StudioPluginManager::StopHotReload() {
		if (!hotReloadActive) return;

		shouldStopWatching = true;
		hotReloadActive = false;

		if (watchThread.joinable()) {
			watchThread.join();
		}

		std::cout << "[StudioPluginManager] Hot reload stopped" << std::endl;
	}

	bool StudioPluginManager::ValidatePluginFunctions(PluginInfo& info) {
		info.createFunc = (IPlugin*(*)())GET_PROC_ADDRESS(info.handle, "CreatePlugin");
		info.destroyFunc = (void(*)(IPlugin*))GET_PROC_ADDRESS(info.handle, "DestroyPlugin");
		info.getNameFunc = (const char*(*)())GET_PROC_ADDRESS(info.handle, "GetPluginName");
		info.getVersionFunc = (const char*(*)())GET_PROC_ADDRESS(info.handle, "GetPluginVersion");
		info.getDescFunc = (const char*(*)())GET_PROC_ADDRESS(info.handle, "GetPluginDescription");

		if (!info.createFunc || !info.destroyFunc || !info.getNameFunc || !info.getVersionFunc) {
			info.hasError = true;
			info.errorMessage = "Missing required plugin functions";
			std::cerr << "[StudioPluginManager] " << info.errorMessage << std::endl;
			return false;
		}

		return true;
	}

	std::string StudioPluginManager::CreateTempCopy(const std::string& originalPath) {
		try {
			std::filesystem::path originalFile(originalPath);
			std::string filename = originalFile.stem().string() + "_studio_" +
				std::to_string(tempFileCounter.fetch_add(1)) +
				originalFile.extension().string();

			std::filesystem::path tempPath = std::filesystem::path(tempDirectory) / filename;

			std::filesystem::copy_file(originalPath, tempPath,
				std::filesystem::copy_options::overwrite_existing);

			return tempPath.string();
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioPluginManager] Failed to create temp copy: " << e.what() << std::endl;
			return "";
		}
	}

	void StudioPluginManager::WatchForChanges() {
		while (!shouldStopWatching) {
			CheckForPluginChanges();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	void StudioPluginManager::CheckForPluginChanges() {
		if (!std::filesystem::exists(watchDirectory)) {
			return;
		}

		std::lock_guard<std::mutex> lock(pluginMutex);

		try {
			for (const auto& entry : std::filesystem::directory_iterator(watchDirectory)) {
				if (!entry.is_regular_file() || !IsPluginFile(entry.path().string())) {
					continue;
				}

				std::string pluginName = GetPluginNameFromPath(entry.path().string());
				auto currentWriteTime = GetFileWriteTime(entry.path().string());

				auto it = plugins.find(pluginName);

				// ONLY reload existing plugins
				if (it != plugins.end()) {
					if (currentWriteTime > it->second.lastWriteTime) {
						std::cout << "[StudioPluginManager] Plugin file changed, reloading: " << pluginName << std::endl;

						std::this_thread::sleep_for(std::chrono::milliseconds(100));

						std::string originalPath = it->second.originalPath;
						LoadPluginInternal(originalPath, true);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioPluginManager] Error checking for plugin changes: " << e.what() << std::endl;
		}
	}

	std::string StudioPluginManager::GetPluginNameFromPath(const std::string& path) const {
		std::filesystem::path p(path);
		return p.stem().string();
	}

	std::filesystem::file_time_type StudioPluginManager::GetFileWriteTime(const std::string& path) const {
		try {
			return std::filesystem::last_write_time(path);
		}
		catch (const std::exception&) {
			return std::filesystem::file_time_type{};
		}
	}

	bool StudioPluginManager::IsPluginFile(const std::string& path) const {
		std::filesystem::path p(path);
		std::string ext = p.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

#ifdef _WIN32
		return ext == ".dll";
#else
		return ext == ".so";
#endif
	}

	std::string StudioPluginManager::GetLastSystemError() const {
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

} // namespace Plugin