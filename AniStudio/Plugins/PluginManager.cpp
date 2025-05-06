// PluginManager.cpp
#include "PluginManager.hpp"
#include "GUI.h"  // Update paths as needed
#include "ECS.h" // Update paths as needed
#include <iostream>
#include <algorithm>

namespace Plugin {

	PluginManager::PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
		: mgr(entityMgr), viewMgr(viewMgr), pluginsDirectory("../plugins") {
	}

	PluginManager::~PluginManager() {
		// Unload all plugins
		for (auto& pair : loadedPlugins) {
			UnloadPlugin(pair.first);
		}
	}

	void PluginManager::Init() {
		std::cout << "Initializing Plugin Manager" << std::endl;
		ScanForPlugins();
	}

	void PluginManager::ScanForPlugins(const std::string& pluginDir) {
		pluginsDirectory = pluginDir;

		// Create directory if it doesn't exist
		if (!std::filesystem::exists(pluginDir)) {
			std::filesystem::create_directories(pluginDir);
		}

		// Iterate through all files in the directory
		for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
			if (entry.is_regular_file() && entry.path().extension() == PLUGIN_EXTENSION) {
				std::string path = entry.path().string();

				// Skip if already loaded
				bool alreadyLoaded = false;
				for (const auto& plugin : loadedPlugins) {
					if (plugin.second.path == path) {
						alreadyLoaded = true;
						break;
					}
				}

				if (!alreadyLoaded) {
					LoadPlugin(path);
				}
			}
		}
	}

	bool PluginManager::LoadPlugin(const std::string& path) {
		std::cout << "Loading plugin: " << path << std::endl;

		PluginInfo info;
		info.path = path;
		info.lastModified = std::filesystem::last_write_time(path);

		// Open the library
		info.handle = OpenLibrary(path);
		if (!info.handle) {
			std::cerr << "Failed to load plugin: " << path << std::endl;
			return false;
		}

		// Get the create and destroy functions
		info.createFn = reinterpret_cast<CreatePluginFn>(GetSymbol(info.handle, "CreatePlugin"));
		info.destroyFn = reinterpret_cast<DestroyPluginFn>(GetSymbol(info.handle, "DestroyPlugin"));

		if (!info.createFn || !info.destroyFn) {
			std::cerr << "Plugin does not export required functions: " << path << std::endl;
			CloseLibrary(info.handle);
			return false;
		}

		// Create the plugin instance
		info.instance = info.createFn();
		if (!info.instance) {
			std::cerr << "Failed to create plugin instance: " << path << std::endl;
			CloseLibrary(info.handle);
			return false;
		}

		// Initialize the plugin
		try {
			info.instance->OnLoad(mgr, viewMgr);
			loadedPlugins[info.instance->GetName()] = info;
			std::cout << "Successfully loaded plugin: " << info.instance->GetName()
				<< " v" << info.instance->GetVersion() << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Error initializing plugin: " << e.what() << std::endl;
			info.destroyFn(info.instance);
			CloseLibrary(info.handle);
			return false;
		}
	}

	bool PluginManager::UnloadPlugin(const std::string& name) {
		auto it = loadedPlugins.find(name);
		if (it == loadedPlugins.end()) {
			std::cerr << "Plugin not found: " << name << std::endl;
			return false;
		}

		PluginInfo& info = it->second;

		// Clean up the plugin
		try {
			info.instance->OnUnload();
			info.destroyFn(info.instance);
			CloseLibrary(info.handle);

			loadedPlugins.erase(it);
			std::cout << "Successfully unloaded plugin: " << name << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Error unloading plugin: " << e.what() << std::endl;
			return false;
		}
	}

	void PluginManager::HotReloadPlugins() {
		if (CheckPluginUpdates()) {
			std::vector<std::string> toReload;

			// Collect plugins that need reload
			for (auto& pair : loadedPlugins) {
				if (pair.second.needsReload) {
					toReload.push_back(pair.first);
					pair.second.needsReload = false;
				}
			}

			// Reload each plugin
			for (const auto& name : toReload) {
				std::string path = loadedPlugins[name].path;
				std::cout << "Hot-reloading plugin: " << name << std::endl;

				UnloadPlugin(name);
				LoadPlugin(path);
			}
		}
	}

	bool PluginManager::CheckPluginUpdates() {
		bool updatesFound = false;

		for (auto& pair : loadedPlugins) {
			try {
				auto lastModified = std::filesystem::last_write_time(pair.second.path);

				if (lastModified > pair.second.lastModified) {
					pair.second.needsReload = true;
					pair.second.lastModified = lastModified;
					updatesFound = true;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error checking plugin update: " << e.what() << std::endl;
			}
		}

		// Also check for new plugins
		for (const auto& entry : std::filesystem::directory_iterator(pluginsDirectory)) {
			if (entry.is_regular_file() && entry.path().extension() == PLUGIN_EXTENSION) {
				std::string path = entry.path().string();

				// Check if this plugin is not loaded yet
				bool found = false;
				for (auto& pair : loadedPlugins) {
					if (pair.second.path == path) {
						found = true;
						break;
					}
				}

				if (!found) {
					LoadPlugin(path);
					updatesFound = true;
				}
			}
		}

		return updatesFound;
	}

	void PluginManager::Update(float deltaTime) {
		// Hot reload plugins if needed
		HotReloadPlugins();

		// Update all loaded plugins
		for (auto& pair : loadedPlugins) {
			try {
				pair.second.instance->OnUpdate(deltaTime);
			}
			catch (const std::exception& e) {
				std::cerr << "Error updating plugin " << pair.first << ": " << e.what() << std::endl;
			}
		}
	}

	std::vector<std::string> PluginManager::GetLoadedPlugins() const {
		std::vector<std::string> result;
		for (const auto& pair : loadedPlugins) {
			result.push_back(pair.first);
		}
		return result;
	}

	IPlugin* PluginManager::GetPlugin(const std::string& name) {
		auto it = loadedPlugins.find(name);
		if (it != loadedPlugins.end()) {
			return it->second.instance;
		}
		return nullptr;
	}

	std::string PluginManager::GetPluginPath(const std::string& name) const {
		auto it = loadedPlugins.find(name);
		if (it != loadedPlugins.end()) {
			return it->second.path;
		}
		return "";
	}

	// Platform-specific implementations
#ifdef _WIN32
	LibraryHandle PluginManager::OpenLibrary(const std::string& path) {
		return LoadLibraryA(path.c_str());
	}

	void PluginManager::CloseLibrary(LibraryHandle handle) {
		if (handle) {
			FreeLibrary(handle);
		}
	}

	void* PluginManager::GetSymbol(LibraryHandle handle, const std::string& name) {
		return reinterpret_cast<void*>(GetProcAddress(handle, name.c_str()));
	}
#else
	LibraryHandle PluginManager::OpenLibrary(const std::string& path) {
		return dlopen(path.c_str(), RTLD_NOW);
	}

	void PluginManager::CloseLibrary(LibraryHandle handle) {
		if (handle) {
			dlclose(handle);
		}
	}

	void* PluginManager::GetSymbol(LibraryHandle handle, const std::string& name) {
		return dlsym(handle, name.c_str());
	}
#endif

} // namespace Plugin