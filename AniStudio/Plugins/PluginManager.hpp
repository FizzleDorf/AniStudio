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
 */

#pragma once

#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "ECS.h"
#include "GUI.h"
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE PluginHandle;
#define PLUGIN_EXTENSION ".dll"
#define LOAD_PLUGIN(path) LoadLibraryA(path)
#define GET_PLUGIN_FUNCTION(handle, name) GetProcAddress(handle, name)
#define UNLOAD_PLUGIN(handle) FreeLibrary(handle)
#define PLUGIN_ERROR() GetLastError()
#else
#include <dlfcn.h>
typedef void* PluginHandle;
#define PLUGIN_EXTENSION ".so"
#define LOAD_PLUGIN(path) dlopen(path, RTLD_LAZY)
#define GET_PLUGIN_FUNCTION(handle, name) dlsym(handle, name)
#define UNLOAD_PLUGIN(handle) dlclose(handle)
#define PLUGIN_ERROR() dlerror()
#endif

namespace Plugin {

	// Function pointer types for plugin interface
	typedef BasePlugin* (*CreatePluginFunc)();
	typedef void(*DestroyPluginFunc)(BasePlugin*);
	typedef const char* (*GetPluginNameFunc)();
	typedef const char* (*GetPluginVersionFunc)();
	typedef const char* (*GetPluginDescriptionFunc)();

	struct LoadedPlugin {
		PluginHandle handle = nullptr;
		std::unique_ptr<BasePlugin> plugin = nullptr;
		CreatePluginFunc createFunc = nullptr;
		DestroyPluginFunc destroyFunc = nullptr;
		GetPluginNameFunc getNameFunc = nullptr;
		GetPluginVersionFunc getVersionFunc = nullptr;
		GetPluginDescriptionFunc getDescriptionFunc = nullptr;
		std::string filePath;
		std::string name;
		std::string version;
		std::string description;
		bool initialized = false;
	};

	class PluginManager {
	public:
		PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
			: entityManager(entityMgr), viewManager(viewMgr) {

			// Initialize plugin registry
			PluginRegistry::Initialize(&entityManager, &viewManager);
		}

		~PluginManager() {
			UnloadAllPlugins();
		}

		void Init() {
			std::cout << "PluginManager: Initializing..." << std::endl;

			// Scan for plugins in the plugins directory
			ScanPluginDirectory(Utils::FilePaths::pluginPath);

			std::cout << "PluginManager: Initialization complete. Found "
				<< loadedPlugins.size() << " plugins." << std::endl;
		}

		void Update(float deltaTime) {
			// Update all loaded plugins
			for (auto&[name, pluginData] : loadedPlugins) {
				if (pluginData.initialized && pluginData.plugin) {
					try {
						pluginData.plugin->Update(deltaTime);
					}
					catch (const std::exception& e) {
						std::cerr << "Plugin " << name << " update error: " << e.what() << std::endl;
					}
				}
			}
		}

		bool LoadPlugin(const std::string& filePath) {
			std::cout << "PluginManager: Loading plugin from " << filePath << std::endl;

			// Check if file exists
			if (!std::filesystem::exists(filePath)) {
				std::cerr << "Plugin file not found: " << filePath << std::endl;
				return false;
			}

			// Load the library
			PluginHandle handle = LOAD_PLUGIN(filePath.c_str());
			if (!handle) {
#ifdef _WIN32
				std::cerr << "Failed to load plugin: " << filePath
					<< " Error: " << PLUGIN_ERROR() << std::endl;
#else
				std::cerr << "Failed to load plugin: " << filePath
					<< " Error: " << PLUGIN_ERROR() << std::endl;
#endif
				return false;
			}

			LoadedPlugin pluginData;
			pluginData.handle = handle;
			pluginData.filePath = filePath;

			// Get required function pointers
			pluginData.createFunc = (CreatePluginFunc)GET_PLUGIN_FUNCTION(handle, "CreatePlugin");
			pluginData.destroyFunc = (DestroyPluginFunc)GET_PLUGIN_FUNCTION(handle, "DestroyPlugin");
			pluginData.getNameFunc = (GetPluginNameFunc)GET_PLUGIN_FUNCTION(handle, "GetPluginName");
			pluginData.getVersionFunc = (GetPluginVersionFunc)GET_PLUGIN_FUNCTION(handle, "GetPluginVersion");
			pluginData.getDescriptionFunc = (GetPluginDescriptionFunc)GET_PLUGIN_FUNCTION(handle, "GetPluginDescription");

			if (!pluginData.createFunc || !pluginData.destroyFunc ||
				!pluginData.getNameFunc || !pluginData.getVersionFunc) {
				std::cerr << "Plugin missing required functions: " << filePath << std::endl;
				UNLOAD_PLUGIN(handle);
				return false;
			}

			// Get plugin metadata
			pluginData.name = pluginData.getNameFunc();
			pluginData.version = pluginData.getVersionFunc();
			pluginData.description = pluginData.getDescriptionFunc ?
				pluginData.getDescriptionFunc() : "No description";

			// Check if plugin is already loaded
			if (loadedPlugins.find(pluginData.name) != loadedPlugins.end()) {
				std::cerr << "Plugin already loaded: " << pluginData.name << std::endl;
				UNLOAD_PLUGIN(handle);
				return false;
			}

			// Create plugin instance
			try {
				pluginData.plugin.reset(pluginData.createFunc());
				if (!pluginData.plugin) {
					std::cerr << "Failed to create plugin instance: " << pluginData.name << std::endl;
					UNLOAD_PLUGIN(handle);
					return false;
				}

				// Initialize the plugin
				if (!pluginData.plugin->Initialize()) {
					std::cerr << "Failed to initialize plugin: " << pluginData.name << std::endl;
					pluginData.plugin.reset();
					UNLOAD_PLUGIN(handle);
					return false;
				}

				pluginData.initialized = true;
				loadedPlugins[pluginData.name] = std::move(pluginData);

				std::cout << "Successfully loaded plugin: " << pluginData.name
					<< " v" << pluginData.version << std::endl;

				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception loading plugin " << pluginData.name
					<< ": " << e.what() << std::endl;
				UNLOAD_PLUGIN(handle);
				return false;
			}
		}

		bool UnloadPlugin(const std::string& pluginName) {
			auto it = loadedPlugins.find(pluginName);
			if (it == loadedPlugins.end()) {
				std::cerr << "Plugin not found: " << pluginName << std::endl;
				return false;
			}

			std::cout << "Unloading plugin: " << pluginName << std::endl;

			LoadedPlugin& pluginData = it->second;

			try {
				// Shutdown the plugin
				if (pluginData.plugin && pluginData.initialized) {
					pluginData.plugin->Shutdown();
				}

				// Destroy plugin instance
				if (pluginData.plugin && pluginData.destroyFunc) {
					pluginData.destroyFunc(pluginData.plugin.release());
				}

				// Unload the library
				if (pluginData.handle) {
					UNLOAD_PLUGIN(pluginData.handle);
				}

				loadedPlugins.erase(it);
				std::cout << "Successfully unloaded plugin: " << pluginName << std::endl;
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception unloading plugin " << pluginName
					<< ": " << e.what() << std::endl;
				return false;
			}
		}

		void UnloadAllPlugins() {
			std::cout << "Unloading all plugins..." << std::endl;

			auto pluginNames = GetLoadedPluginNames();
			for (const auto& name : pluginNames) {
				UnloadPlugin(name);
			}

			loadedPlugins.clear();
		}

		void ScanPluginDirectory(const std::string& directory) {
			if (!std::filesystem::exists(directory)) {
				std::cout << "Plugin directory does not exist: " << directory << std::endl;
				return;
			}

			std::cout << "Scanning for plugins in: " << directory << std::endl;

			try {
				for (const auto& entry : std::filesystem::directory_iterator(directory)) {
					if (entry.is_regular_file()) {
						std::string filePath = entry.path().string();
						std::string extension = entry.path().extension().string();

						if (extension == PLUGIN_EXTENSION) {
							LoadPlugin(filePath);
						}
					}
				}
			}
			catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Error scanning plugin directory: " << e.what() << std::endl;
			}
		}

		void ReloadPlugin(const std::string& pluginName) {
			auto it = loadedPlugins.find(pluginName);
			if (it == loadedPlugins.end()) {
				std::cerr << "Plugin not found for reload: " << pluginName << std::endl;
				return;
			}

			std::string filePath = it->second.filePath;
			UnloadPlugin(pluginName);
			LoadPlugin(filePath);
		}

		void ReloadAllPlugins() {
			std::vector<std::string> pluginPaths;

			// Collect all file paths
			for (const auto&[name, pluginData] : loadedPlugins) {
				pluginPaths.push_back(pluginData.filePath);
			}

			// Unload all plugins
			UnloadAllPlugins();

			// Reload all plugins
			for (const auto& path : pluginPaths) {
				LoadPlugin(path);
			}
		}

		std::vector<std::string> GetLoadedPluginNames() const {
			std::vector<std::string> names;
			names.reserve(loadedPlugins.size());

			for (const auto&[name, pluginData] : loadedPlugins) {
				names.push_back(name);
			}

			return names;
		}

		const LoadedPlugin* GetPluginInfo(const std::string& pluginName) const {
			auto it = loadedPlugins.find(pluginName);
			return (it != loadedPlugins.end()) ? &it->second : nullptr;
		}

		size_t GetLoadedPluginCount() const {
			return loadedPlugins.size();
		}

		bool IsPluginLoaded(const std::string& pluginName) const {
			return loadedPlugins.find(pluginName) != loadedPlugins.end();
		}

		// Get plugin instance for direct access (use with caution)
		BasePlugin* GetPlugin(const std::string& pluginName) {
			auto it = loadedPlugins.find(pluginName);
			return (it != loadedPlugins.end() && it->second.initialized) ?
				it->second.plugin.get() : nullptr;
		}

	private:
		ECS::EntityManager& entityManager;
		GUI::ViewManager& viewManager;
		std::unordered_map<std::string, LoadedPlugin> loadedPlugins;
	};

} // namespace Plugin