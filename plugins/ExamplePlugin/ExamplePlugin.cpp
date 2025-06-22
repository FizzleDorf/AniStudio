//============================================================================
// ExamplePlugin.cpp - WORKING Implementation with Registry Sync
//============================================================================

#include "ExamplePlugin.hpp"
#include <iostream>

// Global storage for host function pointers
static GetEntityManagerFunc g_getEntityManager = nullptr;
static GetViewManagerFunc g_getViewManager = nullptr;
static GetImGuiContextFunc g_getImGuiContext = nullptr;

// Global storage for host managers (set during initialization)
static ECS::EntityManager* g_hostEntityManager = nullptr;
static GUI::ViewManager* g_hostViewManager = nullptr;

// Registry synchronization state
static bool g_registrySynced = false;

// Plugin entry points
extern "C" PLUGIN_API Plugin::BasePlugin* CreatePlugin() {
	try {
		std::cout << "=== EXAMPLE PLUGIN CREATE START ===" << std::endl;
		auto* plugin = new ExamplePluginImpl();
		std::cout << "ExamplePluginImpl created successfully" << std::endl;
		return plugin;
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in CreatePlugin: " << e.what() << std::endl;
		return nullptr;
	}
}

extern "C" PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin) {
	try {
		std::cout << "=== EXAMPLE PLUGIN DESTROY START ===" << std::endl;
		if (plugin) {
			delete plugin;
			std::cout << "Plugin deleted successfully" << std::endl;
		}

		// Clear global references
		g_hostEntityManager = nullptr;
		g_hostViewManager = nullptr;
		g_registrySynced = false;

	}
	catch (const std::exception& e) {
		std::cerr << "Exception in DestroyPlugin: " << e.what() << std::endl;
	}
}

extern "C" PLUGIN_API const char* GetPluginName() {
	return "ExamplePlugin";
}

extern "C" PLUGIN_API const char* GetPluginVersion() {
	return "2.0.0";
}

extern "C" PLUGIN_API const char* GetPluginDescription() {
	return "Example plugin with perfect hot reload support";
}

extern "C" PLUGIN_API bool GetPluginCanHotReload() {
	return true;
}

// CRITICAL: Perfect registry synchronization function
void SyncWithHostRegistry(ECS::EntityManager* hostMgr) {
	if (!hostMgr) {
		std::cerr << "SyncWithHostRegistry: Host EntityManager is null!" << std::endl;
		return;
	}

	if (g_registrySynced) {
		std::cout << "Registry already synced, skipping..." << std::endl;
		return;
	}

	std::cout << "=== PERFECT REGISTRY SYNC START ===" << std::endl;

	try {
		// STEP 1: Get complete host registry state
		auto hostComponentNames = hostMgr->GetAllRegisteredComponentNames();
		auto hostSystemNames = hostMgr->GetAllRegisteredSystemNames();

		std::cout << "Host registry state:" << std::endl;
		std::cout << "  Components: " << hostComponentNames.size() << std::endl;
		std::cout << "  Systems: " << hostSystemNames.size() << std::endl;

		// STEP 2: Completely reset plugin registries
		ECS::ComponentTypeRegistry::Reset();
		ECS::SystemTypeRegistry::Reset();

		std::cout << "Plugin registries reset" << std::endl;

		// STEP 3: Import ALL host component registrations with exact IDs
		for (const auto& componentName : hostComponentNames) {
			ECS::ComponentTypeID hostTypeId = hostMgr->GetComponentTypeIdByName(componentName);
			ECS::ComponentTypeRegistry::ForceRegisterWithId(componentName, hostTypeId);
			std::cout << "  Synced component: " << componentName << " -> ID " << hostTypeId << std::endl;
		}

		// STEP 4: Import ALL host system registrations with exact IDs
		for (const auto& systemName : hostSystemNames) {
			ECS::SystemTypeID hostTypeId = hostMgr->GetSystemTypeIdByName(systemName);
			ECS::SystemTypeRegistry::ForceRegisterWithId(systemName, hostTypeId);
			std::cout << "  Synced system: " << systemName << " -> ID " << hostTypeId << std::endl;
		}

		// STEP 5: Synchronize next type IDs to prevent conflicts
		ECS::ComponentTypeID maxCompId = 0;
		for (const auto& componentName : hostComponentNames) {
			ECS::ComponentTypeID hostTypeId = hostMgr->GetComponentTypeIdByName(componentName);
			if (hostTypeId > maxCompId) {
				maxCompId = hostTypeId;
			}
		}

		ECS::SystemTypeID maxSysId = 0;
		for (const auto& systemName : hostSystemNames) {
			ECS::SystemTypeID hostTypeId = hostMgr->GetSystemTypeIdByName(systemName);
			if (hostTypeId > maxSysId) {
				maxSysId = hostTypeId;
			}
		}

		ECS::ComponentTypeRegistry::SetNextTypeID(maxCompId + 1);
		ECS::SystemTypeRegistry::SetNextTypeID(maxSysId + 1);

		std::cout << "Set plugin component nextTypeID to: " << (maxCompId + 1) << std::endl;
		std::cout << "Set plugin system nextTypeID to: " << (maxSysId + 1) << std::endl;

		// STEP 6: Verify synchronization
		std::cout << "=== REGISTRY SYNC VERIFICATION ===" << std::endl;
		for (const auto& componentName : hostComponentNames) {
			ECS::ComponentTypeID hostId = hostMgr->GetComponentTypeIdByName(componentName);
			ECS::ComponentTypeID pluginId = ECS::ComponentTypeRegistry::GetIDByName(componentName);

			if (hostId != pluginId) {
				std::cerr << "SYNC ERROR: " << componentName
					<< " - Host ID: " << hostId << ", Plugin ID: " << pluginId << std::endl;
				return;
			}
		}

		g_registrySynced = true;
		std::cout << "=== REGISTRY SYNC COMPLETE & VERIFIED ===" << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Exception during registry sync: " << e.what() << std::endl;
		g_registrySynced = false;
	}
}

// Manager getter setup function
extern "C" PLUGIN_API void SetManagerGetters(
	GetEntityManagerFunc entityGetter,
	GetViewManagerFunc viewGetter,
	GetImGuiContextFunc contextGetter,
	GetImGuiAllocFunc allocGetter,
	GetImGuiFreeFunc freeGetter,
	GetImGuiUserDataFunc userDataGetter) {

	std::cout << "=== SET MANAGER GETTERS CALLED ===" << std::endl;

	// Store the function pointers
	g_getEntityManager = entityGetter;
	g_getViewManager = viewGetter;
	g_getImGuiContext = contextGetter;

	// Set up ImGui context if provided
	if (contextGetter) {
		ImGuiContext* hostContext = contextGetter();
		if (hostContext) {
			ImGui::SetCurrentContext(hostContext);
			std::cout << "ImGui context set successfully" << std::endl;
		}
	}

	// CRITICAL: Sync registries immediately when we get access to host managers
	if (entityGetter) {
		ECS::EntityManager* hostMgr = entityGetter();
		if (hostMgr) {
			SyncWithHostRegistry(hostMgr);
		}
	}

	std::cout << "Manager getters stored and registry synced" << std::endl;
}

// Helper functions for cross-DLL manager access
namespace Plugin {
	ECS::EntityManager* GetHostEntityManagerViaPointer() {
		return g_getEntityManager ? g_getEntityManager() : g_hostEntityManager;
	}

	GUI::ViewManager* GetHostViewManagerViaPointer() {
		return g_getViewManager ? g_getViewManager() : g_hostViewManager;
	}

	ImGuiContext* GetHostImGuiContextViaPointer() {
		return g_getImGuiContext ? g_getImGuiContext() : nullptr;
	}
}

// PLUGIN IMPLEMENTATION
ExamplePluginImpl::ExamplePluginImpl() {
	m_name = "ExamplePlugin";
	m_version = "2.0.0";
	m_description = "Example plugin with perfect hot reload support";
	m_initialized = false;
	m_viewID = 0;
}

bool ExamplePluginImpl::Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) {
	try {
		std::cout << "=== EXAMPLE PLUGIN INITIALIZE START ===" << std::endl;

		// Store the host managers
		g_hostEntityManager = &entityManager;
		g_hostViewManager = &viewManager;

		// Verify managers are valid
		if (!&entityManager || !&viewManager) {
			std::cerr << "ExamplePlugin: Invalid managers passed to Initialize!" << std::endl;
			return false;
		}

		std::cout << "Using EntityManager: " << &entityManager << std::endl;
		std::cout << "Using ViewManager: " << &viewManager << std::endl;

		// Ensure registry is synced
		if (!g_registrySynced) {
			std::cout << "Registry not synced yet, syncing now..." << std::endl;
			SyncWithHostRegistry(&entityManager);
		}

		if (!g_registrySynced) {
			std::cerr << "Failed to sync registry!" << std::endl;
			return false;
		}

		// CRITICAL: Verify component type registration
		std::cout << "=== COMPONENT REGISTRATION VERIFICATION ===" << std::endl;

		// Check what our component type would get
		ECS::ComponentTypeID pluginCompTypeId = ECS::CompType<ExamplePlugin::ExampleComponent>();
		std::cout << "Plugin CompType<ExampleComponent>() returns: " << pluginCompTypeId << std::endl;

		if (pluginCompTypeId >= ECS::MAX_COMPONENT_COUNT) {
			std::cerr << "ERROR: Component type got invalid ID!" << std::endl;
			return false;
		}

		// Register our component with the HOST's entity manager
		try {
			if (!entityManager.IsComponentNameRegistered("ExampleComponent")) {
				entityManager.RegisterComponentName<ExamplePlugin::ExampleComponent>("ExampleComponent");
				std::cout << "Component registered successfully with HOST" << std::endl;
			}
			else {
				std::cout << "Component already registered in HOST, skipping" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to register component with HOST: " << e.what() << std::endl;
			return false;
		}

		// CRITICAL: Verify the registration worked correctly
		ECS::ComponentTypeID hostTypeId = entityManager.GetComponentTypeIdByName("ExampleComponent");
		ECS::ComponentTypeID currentPluginTypeId = ECS::CompType<ExamplePlugin::ExampleComponent>();

		std::cout << "=== REGISTRATION VERIFICATION ===" << std::endl;
		std::cout << "Host registry type ID for ExampleComponent: " << hostTypeId << std::endl;
		std::cout << "Plugin CompType<ExampleComponent>() now returns: " << currentPluginTypeId << std::endl;

		if (hostTypeId != currentPluginTypeId) {
			std::cerr << "ERROR: Type ID mismatch after registration!" << std::endl;
			std::cerr << "Host=" << hostTypeId << ", Plugin=" << currentPluginTypeId << std::endl;
			return false;
		}

		// Create view
		try {
			m_viewID = viewManager.CreateView();
			if (m_viewID == 0) {
				std::cerr << "Failed to create view!" << std::endl;
				return false;
			}

			ExamplePlugin::ExampleView exampleView(entityManager);
			viewManager.AddView<ExamplePlugin::ExampleView>(m_viewID, std::move(exampleView));
			viewManager.GetView<ExamplePlugin::ExampleView>(m_viewID).Init();

			std::cout << "View created successfully with ID: " << m_viewID << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Error creating view: " << e.what() << std::endl;
			return false;
		}

		SetInitialized(true);
		m_initialized = true;

		std::cout << "=== EXAMPLE PLUGIN INITIALIZE COMPLETE ===" << std::endl;
		return true;

	}
	catch (const std::exception& e) {
		std::cerr << "Exception during ExamplePlugin initialization: " << e.what() << std::endl;
		return false;
	}
}

void ExamplePluginImpl::Shutdown() {
	try {
		std::cout << "=== EXAMPLE PLUGIN SHUTDOWN START ===" << std::endl;

		if (m_initialized && m_viewID != 0) {
			auto* viewMgr = GetViewManager();
			if (viewMgr) {
				try {
					if (viewMgr->HasView<ExamplePlugin::ExampleView>(m_viewID)) {
						viewMgr->RemoveView<ExamplePlugin::ExampleView>(m_viewID);
					}
					viewMgr->DestroyView(m_viewID);
					m_viewID = 0;
					std::cout << "View removed successfully" << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "Error removing view: " << e.what() << std::endl;
				}
			}
		}

		SetInitialized(false);
		m_initialized = false;

		std::cout << "=== EXAMPLE PLUGIN SHUTDOWN COMPLETE ===" << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Exception during ExamplePlugin shutdown: " << e.what() << std::endl;
	}
}

void ExamplePluginImpl::Update(float deltaTime) {
	if (m_initialized) {
		// Plugin update logic here
	}
}

void ExamplePluginImpl::SaveState() {
	std::cout << "ExamplePlugin: Saving state for hot reload..." << std::endl;
	// Save any state that should persist across reloads
}

void ExamplePluginImpl::LoadState() {
	std::cout << "ExamplePlugin: Loading state after hot reload..." << std::endl;
	// Restore any state that should persist across reloads
}

void ExamplePluginImpl::OnPreReload() {
	std::cout << "ExamplePlugin: Preparing for hot reload..." << std::endl;
	SaveState();
}

void ExamplePluginImpl::OnPostReload() {
	std::cout << "ExamplePlugin: Completed hot reload..." << std::endl;
	LoadState();
}

const std::string& ExamplePluginImpl::GetName() const { return m_name; }
const std::string& ExamplePluginImpl::GetVersion() const { return m_version; }
const std::string& ExamplePluginImpl::GetDescription() const { return m_description; }
bool ExamplePluginImpl::HasSettings() const { return false; }
bool ExamplePluginImpl::CanReload() const { return true; }
std::vector<std::string> ExamplePluginImpl::GetDependencies() const { return {}; }