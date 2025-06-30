// PluginInterface.cpp - Implementation of plugin interface functions
// This gets compiled with the plugin and provides bridge to host PluginRegistry

#include "PluginInterface.hpp"

// ============================================================================
// EXTERNAL FUNCTIONS - These will be provided by the host at runtime
// ============================================================================

// Function pointers that will be set by the host when the plugin loads
namespace Plugin {

	// Global function pointers to host's PluginRegistry functions
	static bool(*g_RegisterComponent)(const std::string&, ComponentCreateFunc, ComponentGetFunc, ComponentHasFunc, ComponentRemoveFunc) = nullptr;
	static bool(*g_RegisterSystem)(const std::string&, SystemCreateFunc) = nullptr;
	static bool(*g_RegisterView)(const std::string&, ViewCreateFunc) = nullptr;
	static ECS::EntityID(*g_CreateEntity)() = nullptr;
	static void(*g_DestroyEntity)(ECS::EntityID) = nullptr;
	static bool(*g_IsEntityValid)(ECS::EntityID) = nullptr;
	static bool(*g_AddComponentByName)(ECS::EntityID, const std::string&) = nullptr;
	static void* (*g_GetComponentByName)(ECS::EntityID, const std::string&) = nullptr;
	static bool(*g_HasComponentByName)(ECS::EntityID, const std::string&) = nullptr;
	static void(*g_RemoveComponentByName)(ECS::EntityID, const std::string&) = nullptr;
	static bool(*g_CreateSystemByName)(const std::string&) = nullptr;
	static GUI::ViewListID(*g_CreateViewByName)(const std::string&) = nullptr;
	static bool(*g_HasGUISupport)() = nullptr;

	// ============================================================================
	// PLUGIN INTERFACE FUNCTION - Called by host to set up function pointers
	// ============================================================================

	extern "C" PLUGIN_API void SetPluginRegistryFunctions(
		bool(*registerComponent)(const std::string&, ComponentCreateFunc, ComponentGetFunc, ComponentHasFunc, ComponentRemoveFunc),
		bool(*registerSystem)(const std::string&, SystemCreateFunc),
		bool(*registerView)(const std::string&, ViewCreateFunc),
		ECS::EntityID(*createEntity)(),
		void(*destroyEntity)(ECS::EntityID),
		bool(*isEntityValid)(ECS::EntityID),
		bool(*addComponentByName)(ECS::EntityID, const std::string&),
		void* (*getComponentByName)(ECS::EntityID, const std::string&),
		bool(*hasComponentByName)(ECS::EntityID, const std::string&),
		void(*removeComponentByName)(ECS::EntityID, const std::string&),
		bool(*createSystemByName)(const std::string&),
		GUI::ViewListID(*createViewByName)(const std::string&),
		bool(*hasGUISupport)()
	) {
		g_RegisterComponent = registerComponent;
		g_RegisterSystem = registerSystem;
		g_RegisterView = registerView;
		g_CreateEntity = createEntity;
		g_DestroyEntity = destroyEntity;
		g_IsEntityValid = isEntityValid;
		g_AddComponentByName = addComponentByName;
		g_GetComponentByName = getComponentByName;
		g_HasComponentByName = hasComponentByName;
		g_RemoveComponentByName = removeComponentByName;
		g_CreateSystemByName = createSystemByName;
		g_CreateViewByName = createViewByName;
		g_HasGUISupport = hasGUISupport;

		std::cout << "[Plugin] Registry functions set up successfully" << std::endl;
	}

	// ============================================================================
	// BASE PLUGIN IMPLEMENTATION - Uses function pointers to call host
	// ============================================================================

	bool BasePlugin::RegisterComponent(const std::string& name,
		ComponentCreateFunc createFunc,
		ComponentGetFunc getFunc,
		ComponentHasFunc hasFunc,
		ComponentRemoveFunc removeFunc) {
		if (g_RegisterComponent) {
			return g_RegisterComponent(name, createFunc, getFunc, hasFunc, removeFunc);
		}
		std::cerr << "[Plugin] RegisterComponent function not available!" << std::endl;
		return false;
	}

	bool BasePlugin::RegisterSystem(const std::string& name, SystemCreateFunc createFunc) {
		if (g_RegisterSystem) {
			return g_RegisterSystem(name, createFunc);
		}
		std::cerr << "[Plugin] RegisterSystem function not available!" << std::endl;
		return false;
	}

	bool BasePlugin::RegisterView(const std::string& name, ViewCreateFunc createFunc) {
		if (g_RegisterView) {
			return g_RegisterView(name, createFunc);
		}
		std::cerr << "[Plugin] RegisterView function not available!" << std::endl;
		return false;
	}

	ECS::EntityID BasePlugin::CreateEntity() {
		if (g_CreateEntity) {
			return g_CreateEntity();
		}
		std::cerr << "[Plugin] CreateEntity function not available!" << std::endl;
		return 0;
	}

	void BasePlugin::DestroyEntity(ECS::EntityID entity) {
		if (g_DestroyEntity) {
			g_DestroyEntity(entity);
		}
		else {
			std::cerr << "[Plugin] DestroyEntity function not available!" << std::endl;
		}
	}

	bool BasePlugin::IsEntityValid(ECS::EntityID entity) {
		if (g_IsEntityValid) {
			return g_IsEntityValid(entity);
		}
		std::cerr << "[Plugin] IsEntityValid function not available!" << std::endl;
		return false;
	}

	bool BasePlugin::AddComponent(ECS::EntityID entity, const std::string& componentName) {
		if (g_AddComponentByName) {
			return g_AddComponentByName(entity, componentName);
		}
		std::cerr << "[Plugin] AddComponentByName function not available!" << std::endl;
		return false;
	}

	void* BasePlugin::GetComponent(ECS::EntityID entity, const std::string& componentName) {
		if (g_GetComponentByName) {
			return g_GetComponentByName(entity, componentName);
		}
		std::cerr << "[Plugin] GetComponentByName function not available!" << std::endl;
		return nullptr;
	}

	bool BasePlugin::HasComponent(ECS::EntityID entity, const std::string& componentName) {
		if (g_HasComponentByName) {
			return g_HasComponentByName(entity, componentName);
		}
		std::cerr << "[Plugin] HasComponentByName function not available!" << std::endl;
		return false;
	}

	void BasePlugin::RemoveComponent(ECS::EntityID entity, const std::string& componentName) {
		if (g_RemoveComponentByName) {
			g_RemoveComponentByName(entity, componentName);
		}
		else {
			std::cerr << "[Plugin] RemoveComponentByName function not available!" << std::endl;
		}
	}

	bool BasePlugin::CreateSystem(const std::string& systemName) {
		if (g_CreateSystemByName) {
			return g_CreateSystemByName(systemName);
		}
		std::cerr << "[Plugin] CreateSystemByName function not available!" << std::endl;
		return false;
	}

	GUI::ViewListID BasePlugin::CreateView(const std::string& viewName) {
		if (g_CreateViewByName) {
			return g_CreateViewByName(viewName);
		}
		std::cerr << "[Plugin] CreateViewByName function not available!" << std::endl;
		return 0;
	}

	bool BasePlugin::HasGUISupport() {
		if (g_HasGUISupport) {
			return g_HasGUISupport();
		}
		return false;
	}

} // namespace Plugin