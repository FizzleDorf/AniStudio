#include "PluginManager.hpp"
#include "EntityManager.hpp"
#include <iostream>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Plugins {

	// PluginRegistry implementation - NOW WITH IMGUI CONTEXT
	PluginRegistry::PluginRegistry(const std::string& pluginName, PluginManager* manager, ImGuiContext* imguiContext)
		: pluginName(pluginName), manager(manager), imguiContext(imguiContext) {
		std::cout << "[PluginRegistry] Constructor - plugin: " << pluginName
			<< ", ImGui context: " << imguiContext << std::endl;
	}

	ECS::ComponentTypeID PluginRegistry::RegisterComponent(const ComponentDescriptor& desc) {
		std::cout << "[PluginRegistry] Registering component: " << desc.name << " for plugin: " << pluginName << std::endl;
		return manager->registerComponent(pluginName, desc);
	}

	ECS::SystemTypeID PluginRegistry::RegisterSystem(const SystemDescriptor& desc) {
		std::cout << "[PluginRegistry] Registering system: " << desc.name << " for plugin: " << pluginName << std::endl;
		return manager->registerSystem(pluginName, desc);
	}

	GUI::ViewTypeID PluginRegistry::RegisterView(const ViewDescriptor& desc) {
		std::cout << "[PluginRegistry] Registering view: " << desc.name << " for plugin: " << pluginName
			<< " with context: " << imguiContext << std::endl;
		return manager->registerView(pluginName, desc);
	}

	// PluginManager implementation - NOW WITH IMGUI CONTEXT STORAGE
	PluginManager::PluginManager(ECS::EntityManager& entityMgr, ImGuiContext* imguiContext)
		: entityManager(entityMgr), imguiContext(imguiContext) {
		std::cout << "[PluginManager] Constructor - manager created with ImGui context: " << imguiContext << std::endl;
	}

	PluginManager::~PluginManager() {
		std::cout << "[PluginManager] Destructor - cleaning up plugins..." << std::endl;

		// Unload all plugins in order
		std::vector<std::string> pluginNames;
		for (const auto& pair : plugins) {
			pluginNames.push_back(pair.first);
		}

		for (const auto& name : pluginNames) {
			if (plugins[name].loaded) {
				if (plugins[name].enabled) disablePlugin(name);
				unloadPlugin(name);
			}
		}
	}

	bool PluginManager::loadPlugin(const std::string& dllPath) {
		std::filesystem::path path(dllPath);
		if (!std::filesystem::exists(path)) {
			std::cerr << "[PluginManager] Plugin file not found: " << dllPath << std::endl;
			return false;
		}

		std::string pluginName = path.stem().string();

		if (plugins.find(pluginName) != plugins.end()) {
			std::cout << "[PluginManager] Plugin already loaded: " << pluginName << std::endl;
			return true;
		}

		void* handle = loadDynamicLibrary(dllPath);
		if (!handle) {
			std::cerr << "[PluginManager] Failed to load library: " << dllPath << std::endl;
			return false;
		}

		auto createFunc = reinterpret_cast<BasePlugin*(*)()>(getFunction(handle, "CreatePlugin"));
		auto destroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(handle, "DestroyPlugin"));

		if (!createFunc || !destroyFunc) {
			std::cerr << "[PluginManager] Plugin missing required functions: " << dllPath << std::endl;
			unloadLibrary(handle);
			return false;
		}

		PluginInfo info;
		info.name = pluginName;
		info.path = dllPath;
		info.handle = handle;
		info.loaded = true;
		info.enabled = false;
		info.createFunc = createFunc;
		info.destroyFunc = destroyFunc;
		info.lastWriteTime = std::filesystem::last_write_time(dllPath);

		plugins[pluginName] = info;
		std::cout << "[PluginManager] Plugin loaded: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::enablePlugin(const std::string& pluginName) {
		std::cout << "[PluginManager] === ENABLING PLUGIN: " << pluginName << " ===" << std::endl;

		auto it = plugins.find(pluginName);
		if (it == plugins.end() || !it->second.loaded) {
			std::cerr << "[PluginManager] Plugin not found or not loaded: " << pluginName << std::endl;
			return false;
		}

		PluginInfo& plugin = it->second;
		if (plugin.enabled) {
			std::cout << "[PluginManager] Plugin already enabled: " << pluginName << std::endl;
			return true;
		}

		try {
			// Create the plugin instance
			std::cout << "[PluginManager] Creating plugin instance..." << std::endl;
			plugin.instance = plugin.createFunc();

			if (!plugin.instance) {
				std::cerr << "[PluginManager] Failed to create plugin instance: " << pluginName << std::endl;
				return false;
			}

			// Get version from plugin instance
			plugin.version = plugin.instance->GetVersion();

			// Create a registry for this plugin - NOW WITH IMGUI CONTEXT
			PluginRegistry registry(pluginName, this, imguiContext);

			// Call OnEngineInit with direct registry access
			std::cout << "[PluginManager] Calling OnEngineInit with registry that has ImGui context: "
				<< imguiContext << std::endl;

			if (!plugin.instance->OnEngineInit(entityManager, registry)) {
				std::cerr << "[PluginManager] Plugin initialization failed: " << pluginName << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
			}

			plugin.instance->SetInitialized(true);
			plugin.enabled = true;

			std::cout << "[PluginManager] Plugin enabled successfully: " << pluginName << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Exception during plugin enable: " << e.what() << std::endl;
			if (plugin.instance) {
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
			}
			return false;
		}

		return true;
	}

	bool PluginManager::disablePlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end() || !it->second.enabled) {
			return false;
		}

		PluginInfo& plugin = it->second;

		std::cout << "[PluginManager] Disabling plugin: " << pluginName << std::endl;

		// Call plugin shutdown
		plugin.instance->OnShutdown();
		plugin.instance->SetInitialized(false);

		// IMPORTANT: Cleanup all plugin registrations BEFORE destroying the instance
		cleanupPluginComponents(pluginName);
		cleanupPluginSystems(pluginName);
		cleanupPluginViews(pluginName);

		plugin.destroyFunc(plugin.instance);
		plugin.instance = nullptr;
		plugin.enabled = false;

		std::cout << "[PluginManager] Plugin disabled and cleaned up: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::unloadPlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		PluginInfo& plugin = it->second;
		if (plugin.enabled) {
			disablePlugin(pluginName);
		}

		if (plugin.handle) {
			unloadLibrary(plugin.handle);
		}

		plugins.erase(it);
		std::cout << "[PluginManager] Plugin unloaded: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::reloadPlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		bool wasEnabled = it->second.enabled;
		std::string path = it->second.path;

		if (!unloadPlugin(pluginName)) return false;
		if (!loadPlugin(path)) return false;
		if (wasEnabled && !enablePlugin(pluginName)) return false;

		std::cout << "[PluginManager] Plugin reloaded: " << pluginName << std::endl;
		return true;
	}

	void PluginManager::updatePlugins(float deltaTime) {
		for (auto& pair : plugins) {
			auto& plugin = pair.second;
			if (plugin.enabled && plugin.instance) {
				plugin.instance->OnUpdate(deltaTime);
			}
		}
	}

	void PluginManager::checkForChanges() {
		if (!hotReloadEnabled) return;

		for (auto& pair : plugins) {
			auto& plugin = pair.second;
			if (plugin.loaded && std::filesystem::exists(plugin.path)) {
				auto newWriteTime = std::filesystem::last_write_time(plugin.path);
				if (newWriteTime > plugin.lastWriteTime) {
					std::cout << "[PluginManager] Change detected, reloading: " << pair.first << std::endl;
					reloadPlugin(pair.first);
					plugin.lastWriteTime = newWriteTime;
				}
			}
		}
	}

	void PluginManager::scanPluginDirectory(const std::string& directory) {
		if (!std::filesystem::exists(directory)) {
			std::cout << "[PluginManager] Plugin directory doesn't exist: " << directory << std::endl;
			return;
		}

		std::cout << "[PluginManager] Scanning for plugins in: " << directory << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.is_regular_file()) {
				std::string path = entry.path().string();
				std::string extension = entry.path().extension().string();

#ifdef _WIN32
				if (extension == ".dll") {
#else
				if (extension == ".so") {
#endif
					loadPlugin(path);
				}
				}
			}
		}

	std::vector<PluginInfo> PluginManager::getLoadedPlugins() const {
		std::vector<PluginInfo> loadedPlugins;
		for (const auto& pair : plugins) {
			loadedPlugins.push_back(pair.second);
		}
		return loadedPlugins;
	}

	BasePlugin* PluginManager::getPlugin(const std::string& name) const {
		auto it = plugins.find(name);
		return (it != plugins.end()) ? it->second.instance : nullptr;
	}

	ECS::ComponentTypeID PluginManager::registerComponent(const std::string& pluginName, const ComponentDescriptor& desc) {
		std::cout << "[PluginManager] registerComponent called for plugin: " << pluginName << ", component: " << desc.name << std::endl;

		// Use proper component type registration with next sequential ID
		ECS::ComponentTypeID id = ECS::ComponentTypeRegistry::RegisterType<void>(desc.name);
		if (id == ECS::MAX_COMPONENT_COUNT) {
			std::cerr << "[PluginManager] Failed to register component: " << desc.name << std::endl;
			return ECS::MAX_COMPONENT_COUNT;
		}

		// Register with EntityManager for plugin component support
		entityManager.RegisterPluginComponent(
			id,
			desc.size,
			desc.constructor,
			desc.destructor
		);

		pluginComponents[pluginName].push_back(id);

		std::cout << "[PluginManager] Registered component " << desc.name << " with ID " << id << " for plugin " << pluginName << std::endl;
		return id;
	}

	ECS::SystemTypeID PluginManager::registerSystem(const std::string& pluginName, const SystemDescriptor& desc) {
		std::cout << "[PluginManager] registerSystem called for plugin: " << pluginName << ", system: " << desc.name << std::endl;

		// Use the main SystemTypeRegistry to get proper sequential IDs
		ECS::SystemTypeID id = ECS::SystemTypeRegistry::RegisterType<void>();

		// Register with EntityManager for plugin system support
		entityManager.RegisterPluginSystem(
			id,
			desc.creator,
			desc.destructor,
			desc.updater,
			[](void*) {}, // Default empty starter
			desc.requiredComponents
		);

		pluginSystems[pluginName].push_back(id);

		std::cout << "[PluginManager] Registered system " << desc.name << " with ID " << id << " for plugin " << pluginName << std::endl;
		return id;
	}

	void PluginManager::cleanupPluginComponents(const std::string& pluginName) {
		auto it = pluginComponents.find(pluginName);
		if (it == pluginComponents.end()) return;

		std::cout << "[PluginManager] Cleaning up components for plugin: " << pluginName << std::endl;

		for (ECS::ComponentTypeID id : it->second) {
			// Remove any entities that have this component
			auto entities = entityManager.GetAllEntities();
			for (ECS::EntityID entity : entities) {
				if (entityManager.HasPluginComponent(entity, id)) {
					std::cout << "[PluginManager] Removing component " << id << " from entity " << entity << std::endl;
					entityManager.RemovePluginComponent(entity, id);
				}
			}

			// Unregister the component type
			entityManager.UnregisterPluginComponent(id);
			std::cout << "[PluginManager] Unregistered component ID: " << id << std::endl;
		}

		pluginComponents.erase(it);
	}

	void PluginManager::cleanupPluginSystems(const std::string& pluginName) {
		auto it = pluginSystems.find(pluginName);
		if (it == pluginSystems.end()) return;

		std::cout << "[PluginManager] Cleaning up systems for plugin: " << pluginName << std::endl;

		for (ECS::SystemTypeID id : it->second) {
			// Stop and destroy the system
			void* systemInstance = entityManager.GetPluginSystem(id);
			if (systemInstance) {
				std::cout << "[PluginManager] Destroying system instance for ID: " << id << std::endl;
			}

			// Unregister the system
			entityManager.UnregisterPluginSystem(id);
			std::cout << "[PluginManager] Unregistered system ID: " << id << std::endl;
		}

		pluginSystems.erase(it);
	}

	void* PluginManager::loadDynamicLibrary(const std::string& path) {
#ifdef _WIN32
		return ::LoadLibraryA(path.c_str());
#else
		return dlopen(path.c_str(), RTLD_LAZY);
#endif
	}

	void PluginManager::unloadLibrary(void* handle) {
		if (!handle) return;
#ifdef _WIN32
		::FreeLibrary(static_cast<HMODULE>(handle));
#else
		dlclose(handle);
#endif
	}

	void* PluginManager::getFunction(void* handle, const std::string& name) {
		if (!handle) return nullptr;
#ifdef _WIN32
		return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), name.c_str()));
#else
		return dlsym(handle, name.c_str());
#endif
	}

	} // namespace Plugins