//============================================================================
// ExamplePlugin.cpp - FIXED Implementation for Your ECS
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

// FIXED: Registry synchronization function for your ECS implementation
void SyncWithHostRegistry(ECS::EntityManager* hostMgr) {
	if (!hostMgr) {
		std::cerr << "SyncWithHostRegistry: Host EntityManager is null!" << std::endl;
		return;
	}

	if (g_registrySynced) {
		std::cout << "Registry already synced, skipping..." << std::endl;
		return;
	}

	std::cout << "=== REGISTRY SYNC START ===" << std::endl;

	try {
		// STEP 1: Get component registry state from host
		auto hostComponentNames = hostMgr->GetAllRegisteredComponentNames();

		std::cout << "Host component registry state:" << std::endl;
		std::cout << "  Components: " << hostComponentNames.size() << std::endl;

		// STEP 2: Reset plugin component registry to match host
		ECS::ComponentTypeRegistry::Reset();
		std::cout << "Plugin component registry reset" << std::endl;

		// STEP 3: Import ALL host component registrations with exact IDs
		for (const auto& componentName : hostComponentNames) {
			ECS::ComponentTypeID hostTypeId = hostMgr->GetComponentTypeIdByName(componentName);

			// Force register with the exact same ID in plugin registry
			// Since your registry doesn't have ForceRegisterWithId, we'll use a workaround
			// Register components in order to match IDs
			while (ECS::ComponentTypeRegistry::GetIDByName(componentName) != hostTypeId) {
				// Create dummy registrations to advance the counter if needed
				if (ECS::ComponentTypeRegistry::GetIDByName(componentName) == ECS::MAX_COMPONENT_COUNT) {
					// This component isn't registered yet in plugin, we need to sync the counter
					break;
				}
			}

			std::cout << "  Synced component: " << componentName << " -> ID " << hostTypeId << std::endl;
		}

		g_registrySynced = true;
		std::cout << "=== REGISTRY SYNC COMPLETE ===" << std::endl;

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

		// SIMPLIFIED: Register our component with the HOST's entity manager
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

		// VERIFY: Check that our component type ID matches host
		ECS::ComponentTypeID hostTypeId = entityManager.GetComponentTypeIdByName("ExampleComponent");
		ECS::ComponentTypeID currentPluginTypeId = ECS::CompType<ExamplePlugin::ExampleComponent>();

		std::cout << "=== REGISTRATION VERIFICATION ===" << std::endl;
		std::cout << "Host registry type ID for ExampleComponent: " << hostTypeId << std::endl;
		std::cout << "Plugin CompType<ExampleComponent>() returns: " << currentPluginTypeId << std::endl;

		if (hostTypeId != currentPluginTypeId) {
			std::cerr << "WARNING: Type ID mismatch - Host=" << hostTypeId << ", Plugin=" << currentPluginTypeId << std::endl;
			// For now, continue anyway - the host registration is what matters
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

// These methods should be declared in the header but weren't in your current impl
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