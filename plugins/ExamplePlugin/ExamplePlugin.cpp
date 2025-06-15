/*
	FIXED ExamplePlugin.cpp with Proper ImGui Cleanup using ImGuiContextManager
	- Uses existing ImGuiContextManager infrastructure
	- Adds proper ImGui window cleanup on plugin shutdown
	- Prevents duplicate rendering after reload
	- Fixes ImGui state persistence issues
*/

#include "BasePlugin.hpp"
#include "BaseComponent.hpp"
#include "BaseView.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "PluginRegistry.hpp"
#include "PluginInterface.hpp"
#include "PluginImGuiHelper.hpp"
#include <imgui.h>
#include <iostream>
#include <nlohmann/json.hpp>

// Global function pointers to host managers and context
static ECS::EntityManager* (*g_getHostEntityManager)() = nullptr;
static GUI::ViewManager* (*g_getHostViewManager)() = nullptr;
static ImGuiContext* (*g_getHostImGuiContext)() = nullptr;
static ImGuiMemAllocFunc(*g_getHostAllocFunc)() = nullptr;
static ImGuiMemFreeFunc(*g_getHostFreeFunc)() = nullptr;
static void* (*g_getHostUserData)() = nullptr;

static std::atomic<bool> g_managersValid{ false };
static std::atomic<bool> g_contextValid{ false };
static std::atomic<bool> g_shutdownInProgress{ false };

// CRITICAL: Track our plugin state for cleanup
static std::atomic<bool> g_pluginInitialized{ false };

// Helper functions
ECS::EntityManager* GetHostEntityManagerSafe() {
	if (g_shutdownInProgress.load() || !g_managersValid.load()) {
		return nullptr;
	}
	if (g_getHostEntityManager) {
		return g_getHostEntityManager();
	}
	return nullptr;
}

GUI::ViewManager* GetHostViewManagerSafe() {
	if (g_shutdownInProgress.load() || !g_managersValid.load()) {
		return nullptr;
	}
	if (g_getHostViewManager) {
		return g_getHostViewManager();
	}
	return nullptr;
}

bool EnsureImGuiContext() {
	if (g_shutdownInProgress.load() || !g_contextValid.load()) {
		return false;
	}

	// Use the ImGuiContextManager for proper context handling
	return Plugin::ImGuiContextManager::EnsureContext();
}

// CRITICAL: ImGui cleanup function using the proper context manager
void CleanupPluginImGuiState() {
	if (!Plugin::ImGuiContextManager::IsContextValid()) {
		std::cout << "No ImGui context available for cleanup" << std::endl;
		return;
	}

	std::cout << "Cleaning up plugin ImGui state..." << std::endl;

	// Use the context manager's safe wrapper for cleanup operations
	Plugin::ImGuiContextManager::SafeImGuiCall([]() {
		// Only use the most basic public API functions
		// Clear keyboard focus (this is the safest and most effective)
		ImGui::SetKeyboardFocusHere(-1);

		// Clear focus multiple times to ensure it sticks
		for (int i = 0; i < 5; i++) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		return true; // Return something for the template
	});

	std::cout << "Plugin ImGui state cleanup complete" << std::endl;
}

// UNIFIED plugin entry point with all parameters
extern "C" PLUGIN_API void SetManagerGetters(
	ECS::EntityManager* (*entityGetter)(),
	GUI::ViewManager* (*viewGetter)(),
	ImGuiContext* (*contextGetter)(),
	ImGuiMemAllocFunc(*allocGetter)(),
	ImGuiMemFreeFunc(*freeGetter)(),
	void* (*userDataGetter)()) {

	std::cout << "SetManagerGetters called in plugin with unified interface" << std::endl;

	// Set our custom globals
	g_getHostEntityManager = entityGetter;
	g_getHostViewManager = viewGetter;
	g_getHostImGuiContext = contextGetter;
	g_getHostAllocFunc = allocGetter;
	g_getHostFreeFunc = freeGetter;
	g_getHostUserData = userDataGetter;

	g_managersValid.store(true);
	g_contextValid.store(contextGetter != nullptr);
	g_shutdownInProgress.store(false);

	// Call the Plugin namespace function to set up PluginRegistry
	Plugin::SetManagerGetters(entityGetter, viewGetter, contextGetter, allocGetter, freeGetter, userDataGetter);

	// CRITICAL: Set up the ImGuiContextManager with the host context
	if (contextGetter) {
		ImGuiContext* context = contextGetter();
		ImGuiMemAllocFunc allocFunc = allocGetter ? allocGetter() : nullptr;
		ImGuiMemFreeFunc freeFunc = freeGetter ? freeGetter() : nullptr;
		void* userData = userDataGetter ? userDataGetter() : nullptr;

		Plugin::ImGuiContextManager::SetSharedContext(context, allocFunc, freeFunc, userData);
		std::cout << "Plugin ImGui context manager initialized successfully" << std::endl;
	}
	else {
		std::cout << "Warning: No ImGui context getter provided" << std::endl;
	}

	std::cout << "SetManagerGetters called in plugin with unified interface - COMPLETE" << std::endl;
}

namespace ExamplePlugin {

	struct ExampleComponent : public ECS::BaseComponent {
		int counter = 0;

		ExampleComponent() {
			compName = "ExampleComponent";
			compCategory = "Example";
		}

		virtual nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"counter", counter}
			};
			return j;
		}

		virtual void Deserialize(const nlohmann::json& j) override {
			ECS::BaseComponent::Deserialize(j);
			nlohmann::json componentData;
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				componentData = j;
			}
			if (componentData.contains("counter"))
				counter = componentData["counter"];
		}
	};

	class ExampleView : public GUI::BaseView {
	private:
		ECS::EntityID testEntity = 0;
		bool isInitialized = false;
		bool cleanedUp = false;
		int renderCount = 0;
		std::string windowName = "Example Plugin View###ExamplePluginWindow";

	public:
		ExampleView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
			viewName = "ExampleView";
			std::cout << "ExampleView constructor called" << std::endl;
		}

		~ExampleView() {
			std::cout << "ExampleView destructor called" << std::endl;
			SafeCleanup();
		}

		void SafeCleanup() {
			if (cleanedUp) return;
			cleanedUp = true;

			std::cout << "ExampleView::SafeCleanup() called" << std::endl;

			// CRITICAL: Clean up ImGui state FIRST using the context manager
			CleanupPluginImGuiState();

			if (!g_shutdownInProgress.load() && g_managersValid.load()) {
				try {
					auto* hostMgr = GetHostEntityManagerSafe();
					if (hostMgr && testEntity != 0 && hostMgr->GetEntitiesSignatures().count(testEntity)) {
						hostMgr->DestroyEntity(testEntity);
						std::cout << "ExampleView: Cleaned up test entity" << std::endl;
					}
				}
				catch (...) {
					// Ignore cleanup errors
				}
			}
			testEntity = 0;
		}

		void Init() override {
			std::cout << "ExampleView::Init() called" << std::endl;

			try {
				auto* hostMgr = GetHostEntityManagerSafe();
				if (!hostMgr) {
					std::cerr << "ExampleView::Init - no host EntityManager" << std::endl;
					return;
				}

				testEntity = hostMgr->AddNewEntity();
				hostMgr->AddComponent<ExampleComponent>(testEntity);

				auto& comp = hostMgr->GetComponent<ExampleComponent>(testEntity);
				comp.counter = 42;

				isInitialized = true;
				std::cout << "ExampleView initialized with entity " << testEntity << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Error in ExampleView::Init: " << e.what() << std::endl;
				isInitialized = false;
			}
		}

		void Render() override {
			renderCount++;

			// CRITICAL: Use the ImGuiContextManager for safe rendering
			if (!Plugin::ImGuiContextManager::IsContextValid()) {
				if (renderCount % 60 == 0) {
					std::cout << "ExampleView::Render - No ImGui context available (render #" << renderCount << ")" << std::endl;
				}
				return;
			}

			if (g_shutdownInProgress.load() || !g_managersValid.load()) {
				return;
			}

			// Use the safe wrapper for all ImGui operations
			Plugin::ImGuiContextManager::SafeImGuiCall([this]() {
				bool windowOpen = true;
				if (ImGui::Begin(windowName.c_str(), &windowOpen)) {
					ImGui::Text("=== EXAMPLE PLUGIN VIEW ===");
					ImGui::Text("ImGui Context: %p", ImGui::GetCurrentContext());
					ImGui::Text("Render Count: %d", renderCount);

					if (!isInitialized) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Plugin not properly initialized!");
						ImGui::End();
						return true;
					}

					auto* hostMgr = GetHostEntityManagerSafe();
					if (!hostMgr) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Host EntityManager not available!");
						ImGui::End();
						return true;
					}

					ImGui::Text("Host EntityManager: %p", hostMgr);
					ImGui::Text("Plugin EntityManager: %p", &mgr);
					ImGui::Separator();

					ImGui::Text("Test Entity ID: %zu", testEntity);

					if (testEntity != 0 && hostMgr->GetEntitiesSignatures().count(testEntity)) {
						if (hostMgr->HasComponent<ExampleComponent>(testEntity)) {
							auto& comp = hostMgr->GetComponent<ExampleComponent>(testEntity);
							ImGui::Text("Counter value: %d", comp.counter);

							if (ImGui::Button("Increment Counter")) {
								comp.counter++;
								std::cout << "Incremented counter to: " << comp.counter << std::endl;
							}

							ImGui::SameLine();
							if (ImGui::Button("Reset Counter")) {
								comp.counter = 0;
							}

							ImGui::SameLine();
							if (ImGui::Button("Add 10")) {
								comp.counter += 10;
							}
						}
						else {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Test entity missing ExampleComponent!");
						}
					}
					else {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Test entity not found!");
					}

					ImGui::Separator();

					ImGui::Spacing();
					if (ImGui::Button("Create New Entity with ExampleComponent")) {
						try {
							ECS::EntityID newEntity = hostMgr->AddNewEntity();
							hostMgr->AddComponent<ExampleComponent>(newEntity);
							auto& comp = hostMgr->GetComponent<ExampleComponent>(newEntity);
							comp.counter = 100;
							std::cout << "Created new entity " << newEntity << " with ExampleComponent" << std::endl;
						}
						catch (const std::exception& e) {
							std::cerr << "Error creating entity: " << e.what() << std::endl;
						}
					}

					// Show all entities with ExampleComponent
					ImGui::Separator();
					ImGui::Text("All entities with ExampleComponent:");

					auto entities = hostMgr->GetAllEntities();
					int exampleCompCount = 0;
					for (auto entity : entities) {
						if (hostMgr->GetEntitiesSignatures().count(entity) == 0) {
							continue;
						}

						if (hostMgr->HasComponent<ExampleComponent>(entity)) {
							exampleCompCount++;
							auto& exampleComp = hostMgr->GetComponent<ExampleComponent>(entity);

							ImGui::Text("Entity ID: %zu, Counter: %d", entity, exampleComp.counter);

							ImGui::PushID(static_cast<int>(entity));
							if (ImGui::Button("Reset")) {
								exampleComp.counter = 0;
							}
							ImGui::SameLine();
							if (ImGui::Button("+1")) {
								exampleComp.counter++;
							}
							ImGui::SameLine();
							if (ImGui::Button("-1")) {
								exampleComp.counter--;
							}
							ImGui::PopID();
						}
					}

					ImGui::Text("Total entities with ExampleComponent: %d", exampleCompCount);
				}
				ImGui::End();
				return true;
			});
		}

		void Update(const float deltaT) override {
			// Update logic here if needed
		}
	};

} // namespace ExamplePlugin

class ExamplePluginImpl : public Plugin::BasePlugin {
private:
	std::string m_name = "ExamplePlugin";
	std::string m_version = "1.0.0";
	std::string m_description = "A fixed example plugin with proper ImGui context sharing and cleanup";
	GUI::ViewListID m_viewID = 0;
	bool m_initialized = false;

public:
	ExamplePluginImpl() {
		std::cout << "ExamplePluginImpl constructor" << std::endl;
	}

	~ExamplePluginImpl() override {
		std::cout << "ExamplePluginImpl destructor" << std::endl;
		if (m_initialized) {
			Shutdown();
		}
	}

	bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) override {
		try {
			std::cout << "=== INITIALIZING EXAMPLE PLUGIN ===" << std::endl;

			// Get the host managers directly via our function pointers
			auto* hostEM = GetHostEntityManagerSafe();
			auto* hostVM = GetHostViewManagerSafe();

			if (!hostEM || !hostVM) {
				std::cerr << "CRITICAL: Failed to get host managers!" << std::endl;
				std::cerr << "hostEM: " << hostEM << ", hostVM: " << hostVM << std::endl;
				return false;
			}

			std::cout << "Got host managers - EM: " << hostEM << ", VM: " << hostVM << std::endl;

			// Verify ImGui context is available
			if (!Plugin::ImGuiContextManager::IsContextValid()) {
				std::cerr << "WARNING: ImGui context not available during plugin initialization" << std::endl;
			}

			std::cout << "Registering component with host EntityManager..." << std::endl;
			// Register component with HOST EntityManager directly
			hostEM->RegisterComponentName<ExamplePlugin::ExampleComponent>("Plugin_ExampleComponent");
			std::cout << "ExampleComponent registered successfully" << std::endl;

			std::cout << "Registering view with host ViewManager..." << std::endl;
			// Register view with HOST ViewManager directly
			hostVM->RegisterViewType<ExamplePlugin::ExampleView>("Plugin_ExampleView");
			std::cout << "ExampleView registered successfully" << std::endl;

			std::cout << "Creating view with host ViewManager..." << std::endl;
			// Create view using HOST ViewManager and EntityManager
			m_viewID = hostVM->CreateView();
			hostVM->AddView<ExamplePlugin::ExampleView>(m_viewID, ExamplePlugin::ExampleView(*hostEM));
			hostVM->GetView<ExamplePlugin::ExampleView>(m_viewID).Init();

			if (m_viewID == 0) {
				std::cerr << "Failed to create ExampleView!" << std::endl;
				return false;
			}

			std::cout << "View created with ID: " << m_viewID << std::endl;

			SetInitialized(true);
			m_initialized = true;
			g_pluginInitialized.store(true);

			std::cout << "=== EXAMPLE PLUGIN INITIALIZED SUCCESSFULLY ===" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "EXCEPTION in ExamplePlugin::Initialize: " << e.what() << std::endl;
			return false;
		}
	}

	void Shutdown() override {
		if (!m_initialized) return;

		try {
			std::cout << "=== SHUTTING DOWN EXAMPLE PLUGIN ===" << std::endl;
			g_shutdownInProgress.store(true);
			g_pluginInitialized.store(false);

			// CRITICAL: Clean up ImGui state BEFORE destroying views
			CleanupPluginImGuiState();

			if (m_viewID != 0 && g_managersValid.load()) {
				auto* hostVM = GetHostViewManagerSafe();
				if (hostVM) {
					try {
						auto& view = hostVM->GetView<ExamplePlugin::ExampleView>(m_viewID);
						view.SafeCleanup();
					}
					catch (...) {
						// View might not exist
					}

					hostVM->DestroyView(m_viewID);
					m_viewID = 0;
					std::cout << "ExampleView destroyed" << std::endl;
				}
			}

			SetInitialized(false);
			m_initialized = false;
			std::cout << "=== EXAMPLE PLUGIN SHUTDOWN COMPLETE ===" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during shutdown: " << e.what() << std::endl;
		}
	}

	void Update(float deltaTime) override {
		if (!g_shutdownInProgress.load() && m_initialized) {
			// Update logic if needed
		}
	}

	const std::string& GetName() const override { return m_name; }
	const std::string& GetVersion() const override { return m_version; }
	const std::string& GetDescription() const override { return m_description; }
	bool HasSettings() const override { return false; }
	std::vector<std::string> GetDependencies() const override { return {}; }
};

// Plugin entry points
extern "C" PLUGIN_API Plugin::BasePlugin* CreatePlugin() {
	try {
		std::cout << "=== CREATE PLUGIN CALLED ===" << std::endl;
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
		std::cout << "=== DESTROY PLUGIN CALLED ===" << std::endl;
		if (plugin) {
			g_managersValid.store(false);
			g_shutdownInProgress.store(true);

			// CRITICAL: Final ImGui cleanup before plugin destruction
			CleanupPluginImGuiState();

			// CRITICAL: Clean up the ImGuiContextManager
			Plugin::ImGuiContextManager::Cleanup();

			delete plugin;
			std::cout << "Plugin deleted successfully" << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in DestroyPlugin: " << e.what() << std::endl;
	}
}

extern "C" PLUGIN_API const char* GetPluginName() {
	return "ExamplePlugin";
}

extern "C" PLUGIN_API const char* GetPluginVersion() {
	return "1.0.0";
}