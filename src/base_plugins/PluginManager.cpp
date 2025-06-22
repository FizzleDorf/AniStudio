//============================================================================
// PluginManager.cpp - CRASH FIXED Implementation
//============================================================================

#include "PluginManager.hpp"
#include "PluginRegistry.hpp"
#include "FilePaths.hpp"
#include <iostream>

namespace Plugin {

	PluginManager::PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
		: entityManager(entityMgr), viewManager(viewMgr)
	{
		pluginsDirectory = Utils::FilePaths::pluginPath;
		std::filesystem::create_directories(pluginsDirectory);
		std::cout << "Using plugins directory: " << pluginsDirectory << std::endl;

		// Initialize function pointers to null - will be set by engine
		hostGetEntityManager = nullptr;
		hostGetViewManager = nullptr;
		hostGetImGuiContext = nullptr;
		hostGetImGuiAlloc = nullptr;
		hostGetImGuiFree = nullptr;
		hostGetImGuiUserData = nullptr;
	}

	PluginManager::~PluginManager() {
		Shutdown();
	}

	void PluginManager::Initialize() {
		std::cout << "Initializing PluginManager..." << std::endl;

		// CRITICAL: Initialize PluginRegistry with managers FIRST
		PluginRegistry::Initialize(&entityManager, &viewManager);

		// Scan for plugin DLLs/SOs
		ScanForPlugins();

		std::cout << "PluginManager initialized" << std::endl;
	}

	void PluginManager::SetHostFunctions(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter) {

		std::cout << "PluginManager: Setting host function pointers..." << std::endl;

		hostGetEntityManager = entityGetter;
		hostGetViewManager = viewGetter;
		hostGetImGuiContext = contextGetter;
		hostGetImGuiAlloc = allocGetter;
		hostGetImGuiFree = freeGetter;
		hostGetImGuiUserData = userDataGetter;

		std::cout << "Host function pointers set successfully" << std::endl;
	}

	void PluginManager::Shutdown() {
		std::cout << "Shutting down PluginManager..." << std::endl;

		// Unload all plugins
		auto loadedPlugins = GetLoadedPlugins();
		for (const auto& pluginName : loadedPlugins) {
			UnloadPlugin(pluginName);
		}

		plugins.clear();
		PluginRegistry::Shutdown();
	}

	void PluginManager::Update(float deltaTime) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		for (auto&[name, plugin] : plugins) {
			if (plugin.isLoaded && plugin.instance) {
				try {
					plugin.instance->Update(deltaTime);
				}
				catch (const std::exception& e) {
					std::cerr << "Error updating plugin " << name << ": " << e.what() << std::endl;
				}
			}
		}
	}

	bool PluginManager::ScanForPlugins() {
		std::lock_guard<std::mutex> lock(pluginMutex);

		if (!std::filesystem::exists(pluginsDirectory)) {
			std::cout << "Plugins directory doesn't exist: " << pluginsDirectory << std::endl;
			return false;
		}

		bool foundAny = false;

		// Scan for plugin directories (e.g., build/plugins/ExamplePlugin/)
		for (const auto& entry : std::filesystem::directory_iterator(pluginsDirectory)) {
			if (entry.is_directory()) {
				std::string pluginName = entry.path().filename().string();
				std::string pluginDir = entry.path().string();

				// Check if already exists
				if (plugins.find(pluginName) == plugins.end()) {
					PluginInfo pluginInfo;
					pluginInfo.name = pluginName;
					pluginInfo.pluginDir = pluginDir;

					// Set up paths
#ifdef _WIN32
					pluginInfo.stagingPath = pluginDir + "/staging/" + pluginName + ".dll";
					pluginInfo.activePath = pluginDir + "/" + pluginName + ".dll";
#else
					pluginInfo.stagingPath = pluginDir + "/staging/lib" + pluginName + ".so";
					pluginInfo.activePath = pluginDir + "/lib" + pluginName + ".so";
#endif

					// Create staging directory if it doesn't exist
					std::filesystem::create_directories(pluginDir + "/staging");

					plugins[pluginName] = std::move(pluginInfo);
					foundAny = true;
					std::cout << "Found plugin directory: " << pluginName << " at " << pluginDir << std::endl;
				}
			}
		}

		return foundAny;
	}

	bool PluginManager::LoadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			SetError("Plugin not found: " + pluginName);
			return false;
		}

		auto& plugin = it->second;

		if (plugin.isLoaded) {
			std::cout << "Plugin already loaded: " << pluginName << std::endl;
			return true;
		}

		// Check for new version in staging first, then fall back to active
		std::string libraryToLoad;

		if (std::filesystem::exists(plugin.stagingPath)) {
			// Hot reload: copy from staging to active
			std::cout << "Hot reloading " << pluginName << " from staging..." << std::endl;

			try {
				std::filesystem::copy_file(plugin.stagingPath, plugin.activePath,
					std::filesystem::copy_options::overwrite_existing);
				libraryToLoad = plugin.activePath;

				// Remove from staging after successful copy
				std::filesystem::remove(plugin.stagingPath);
				std::cout << "Copied from staging to active: " << plugin.activePath << std::endl;
			}
			catch (const std::exception& e) {
				SetError("Failed to copy from staging: " + std::string(e.what()));
				return false;
			}
		}
		else if (std::filesystem::exists(plugin.activePath)) {
			// Normal load from active directory
			libraryToLoad = plugin.activePath;
			std::cout << "Loading " << pluginName << " from active directory..." << std::endl;
		}
		else {
			SetError("Plugin library not found: " + plugin.activePath);
			return false;
		}

		// Load the library
		if (!LoadPluginLibrary(plugin, libraryToLoad)) {
			return false;
		}

		plugin.isLoaded = true;

		std::cout << "Successfully loaded plugin: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::UnloadPlugin(const std::string& pluginName) {
		std::lock_guard<std::mutex> lock(pluginMutex);

		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			return true; // Already unloaded
		}

		auto& plugin = it->second;

		if (!plugin.isLoaded) {
			return true; // Already unloaded
		}

		std::cout << "Unloading plugin: " << pluginName << std::endl;

		UnloadPluginLibrary(plugin);
		plugin.isLoaded = false;

		std::cout << "Successfully unloaded plugin: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::ReloadPlugin(const std::string& pluginName) {
		std::cout << "Reloading plugin: " << pluginName << std::endl;

		auto it = plugins.find(pluginName);
		if (it == plugins.end()) {
			return false;
		}

		// Save state if supported
		if (it->second.isLoaded && it->second.instance) {
			it->second.instance->OnPreReload();
		}

		// Unload and reload
		bool wasLoaded = it->second.isLoaded;
		if (wasLoaded) {
			UnloadPlugin(pluginName);
		}

		if (wasLoaded) {
			if (!LoadPlugin(pluginName)) {
				return false;
			}

			// Restore state if supported
			if (it->second.instance) {
				it->second.instance->OnPostReload();
			}
		}

		return true;
	}

	bool PluginManager::IsPluginLoaded(const std::string& pluginName) const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		auto it = plugins.find(pluginName);
		return it != plugins.end() && it->second.isLoaded;
	}

	std::vector<std::string> PluginManager::GetLoadedPlugins() const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		std::vector<std::string> loaded;
		for (const auto&[name, plugin] : plugins) {
			if (plugin.isLoaded) {
				loaded.push_back(name);
			}
		}
		return loaded;
	}

	std::vector<std::string> PluginManager::GetAvailablePlugins() const {
		std::lock_guard<std::mutex> lock(pluginMutex);
		std::vector<std::string> available;
		for (const auto&[name, plugin] : plugins) {
			available.push_back(name);
		}
		return available;
	}

	bool PluginManager::LoadPluginLibrary(PluginInfo& plugin, const std::string& libraryPath) {
		std::cout << "=== LOADING PLUGIN LIBRARY ===" << std::endl;
		std::cout << "Library path: " << libraryPath << std::endl;

		// Load the library
		plugin.handle = LOAD_LIBRARY(libraryPath.c_str());
		if (!plugin.handle) {
#ifdef _WIN32
			DWORD error = ::GetLastError();
			SetError("Failed to load library: " + std::to_string(error));
#else
			SetError("Failed to load library: " + std::string(dlerror()));
#endif
			return false;
		}

		std::cout << "Library loaded successfully" << std::endl;

		// Get function pointers
		plugin.createFunc = (CreatePluginFunc)GET_PROC_ADDRESS(plugin.handle, "CreatePlugin");
		plugin.destroyFunc = (DestroyPluginFunc)GET_PROC_ADDRESS(plugin.handle, "DestroyPlugin");
		plugin.getNameFunc = (GetPluginNameFunc)GET_PROC_ADDRESS(plugin.handle, "GetPluginName");
		plugin.getVersionFunc = (GetPluginVersionFunc)GET_PROC_ADDRESS(plugin.handle, "GetPluginVersion");

		if (!plugin.createFunc || !plugin.destroyFunc || !plugin.getNameFunc) {
			SetError("Missing required plugin functions");
			UNLOAD_LIBRARY(plugin.handle);
			plugin.handle = nullptr;
			return false;
		}

		std::cout << "Function pointers retrieved successfully" << std::endl;

		// Get the SetManagerGetters function from the plugin (OPTIONAL)
		typedef void(*SetManagerGettersFunc)(
			GetEntityManagerFunc,
			GetViewManagerFunc,
			GetImGuiContextFunc,
			GetImGuiAllocFunc,
			GetImGuiFreeFunc,
			GetImGuiUserDataFunc
			);
		SetManagerGettersFunc setManagerGetters =
			(SetManagerGettersFunc)GET_PROC_ADDRESS(plugin.handle, "SetManagerGetters");

		// Create plugin instance
		std::cout << "Creating plugin instance..." << std::endl;
		BasePlugin* rawPlugin = plugin.createFunc();
		if (!rawPlugin) {
			SetError("CreatePlugin returned nullptr");
			UNLOAD_LIBRARY(plugin.handle);
			plugin.handle = nullptr;
			return false;
		}

		std::cout << "Plugin instance created successfully" << std::endl;

		// CRITICAL: Set up manager access for the plugin BEFORE initialization
		std::cout << "Setting up manager access..." << std::endl;
		rawPlugin->SetManagers(&entityManager, &viewManager);

		// Set up function getters if the plugin supports it
		if (setManagerGetters) {
			std::cout << "Setting up manager getters..." << std::endl;

			// Only set if we have the host function pointers
			if (hostGetEntityManager && hostGetViewManager && hostGetImGuiContext) {
				setManagerGetters(
					hostGetEntityManager,
					hostGetViewManager,
					hostGetImGuiContext,
					hostGetImGuiAlloc,
					hostGetImGuiFree,
					hostGetImGuiUserData
				);
				std::cout << "Manager getters set successfully" << std::endl;
			}
			else {
				std::cout << "Warning: Host function pointers not set, skipping SetManagerGetters" << std::endl;
			}
		}

		// Wrap in shared_ptr with custom deleter
		plugin.instance = std::shared_ptr<BasePlugin>(rawPlugin, [&plugin](BasePlugin* p) {
			if (plugin.destroyFunc) {
				plugin.destroyFunc(p);
			}
		});

		std::cout << "Initializing plugin..." << std::endl;

		// Initialize the plugin
		if (!plugin.instance->Initialize(entityManager, viewManager)) {
			SetError("Plugin initialization failed");
			plugin.instance.reset();
			UNLOAD_LIBRARY(plugin.handle);
			plugin.handle = nullptr;
			return false;
		}

		std::cout << "Plugin initialized successfully" << std::endl;
		return true;
	}

	void PluginManager::UnloadPluginLibrary(PluginInfo& plugin) {
		if (plugin.instance) {
			try {
				plugin.instance->Shutdown();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during plugin shutdown: " << e.what() << std::endl;
			}
			plugin.instance.reset();
		}

		if (plugin.handle) {
			UNLOAD_LIBRARY(plugin.handle);
			plugin.handle = nullptr;
		}

		// Clear function pointers
		plugin.createFunc = nullptr;
		plugin.destroyFunc = nullptr;
		plugin.getNameFunc = nullptr;
		plugin.getVersionFunc = nullptr;
	}

	void PluginManager::SetError(const std::string& error) {
		lastError = error;
		std::cerr << "PluginManager Error: " << error << std::endl;
	}

} // namespace Plugin