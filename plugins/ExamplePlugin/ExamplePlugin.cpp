#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "BaseView.hpp"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

// Custom Component - inherits from BaseComponent properly
struct ExampleComponent : public ECS::BaseComponent {
	std::string message;
	float value;

	ExampleComponent() : BaseComponent(), message("Hello from plugin!"), value(0.0f) {
		compName = "ExampleComponent";
	}

	nlohmann::json Serialize() const override {
		nlohmann::json j = BaseComponent::Serialize();
		j[compName]["message"] = message;
		j[compName]["value"] = value;
		return j;
	}

	void Deserialize(const nlohmann::json& j) override {
		BaseComponent::Deserialize(j);

		nlohmann::json componentData;
		if (j.contains(compName)) {
			componentData = j.at(compName);
		}

		if (componentData.contains("message")) message = componentData["message"];
		if (componentData.contains("value")) value = componentData["value"];
	}
};

// Custom System - inherits from BaseSystem properly
class ExampleSystem : public ECS::BaseSystem {
public:
	ExampleSystem(ECS::EntityManager& entityMgr) : BaseSystem(entityMgr) {
		sysName = "ExampleSystem";
		std::cout << "[ExampleSystem] Created" << std::endl;
	}

	~ExampleSystem() {
		std::cout << "[ExampleSystem] Destroyed" << std::endl;
	}

	void Update(float deltaTime) override {
		static float timer = 0.0f;
		timer += deltaTime;

		if (timer > 5.0f) {
			std::cout << "[ExampleSystem] Update tick - processing entities" << std::endl;

			// Actually process entities that have our component
			auto allEntities = mgr.GetAllEntities();
			int processedCount = 0;

			for (auto entity : allEntities) {
				if (mgr.HasPluginComponent(entity, m_componentId)) {
					void* component = mgr.GetPluginComponent(entity, m_componentId);
					if (component) {
						ExampleComponent* exampleComp = static_cast<ExampleComponent*>(component);
						exampleComp->value += deltaTime;
						processedCount++;
					}
				}
			}

			if (processedCount > 0) {
				std::cout << "[ExampleSystem] Processed " << processedCount << " entities with ExampleComponent" << std::endl;
			}

			timer = 0.0f;
		}
	}

	void SetComponentId(ECS::ComponentTypeID id) {
		m_componentId = id;
	}

private:
	ECS::ComponentTypeID m_componentId = ECS::MAX_COMPONENT_COUNT;
};

// SELF-CONTAINED PLUGIN VIEW - Manages its own ImGui context like Blender/Godot plugins
class ExamplePluginView : public GUI::BaseView {
public:
	static constexpr const char* GetMetadataJSON() {
		return R"({
            "displayName": "Example Plugin View",
            "category": "Tools", 
            "description": "A demonstration view from the ExamplePlugin"
        })";
	}

	// CRITICAL FIX: Plugin view is self-contained and context-aware
	ExamplePluginView(ECS::EntityManager& entityMgr, ImGuiContext* mainContext = nullptr)
		: BaseView(entityMgr), m_mainImGuiContext(mainContext), m_contextValid(false) {
		viewName = "ExamplePluginView";
		windowOpen = true;

		std::cout << "[ExamplePluginView] Created with main context: " << mainContext << std::endl;

		// VALIDATE the context immediately
		ValidateContext();
	}

	virtual ~ExamplePluginView() {
		std::cout << "[ExamplePluginView] Destroyed" << std::endl;
	}

	void Init() override {
		std::cout << "[ExamplePluginView] Initialized" << std::endl;
		BaseView::Init();

		// Re-validate context during initialization
		ValidateContext();

		std::cout << "[ExamplePluginView] Context validation result: " << (m_contextValid ? "VALID" : "INVALID") << std::endl;
	}

	void Update(float deltaT) override {
		// Minimal update - just ensure context is still valid
		if (!m_contextValid) {
			ValidateContext();
		}
	}

	void Render() override {
		if (!windowOpen) return;

		// CRITICAL: Ensure we have a valid ImGui context before ANY ImGui calls
		if (!EnsureValidContext()) {
			std::cerr << "[ExamplePluginView] Cannot render - no valid ImGui context" << std::endl;
			return;
		}

		// NOW we can safely make ImGui calls
		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			ImGui::Text("ExamplePlugin View - SELF-CONTAINED SUCCESS!");
			ImGui::Text("Plugin context management: WORKING");

			ImGui::Separator();
			ImGui::Text("Context Info:");
			ImGui::Text("Main Context: %p", m_mainImGuiContext);
			ImGui::Text("Current Context: %p", ImGui::GetCurrentContext());
			ImGui::Text("Context Valid: %s", m_contextValid ? "YES" : "NO");

			// Safe entity operations
			try {
				size_t entityCount = mgr.GetEntityCount();
				ImGui::Text("Total entities: %zu", entityCount);

				if (ImGui::Button("Create Example Entity")) {
					CreateExampleEntity();
				}

				ImGui::Separator();
				RenderEntityList();

			}
			catch (const std::exception& e) {
				ImGui::Text("Error: %s", e.what());
			}

			ImGui::Separator();
			ImGui::Text("Plugin: ExamplePlugin v1.0.0");
			ImGui::Text("Status: SELF-CONTAINED SUCCESS");
			ImGui::Text("Architecture: Like Blender/Godot plugins");
		}
		ImGui::End();
	}

	nlohmann::json Serialize() const override {
		try {
			nlohmann::json json = BaseView::Serialize();
			json["plugin_type"] = "ExamplePlugin";
			json["custom_data"] = "example_data";
			return json;
		}
		catch (...) {
			return nlohmann::json::object();
		}
	}

	void Deserialize(const nlohmann::json& json) override {
		try {
			BaseView::Deserialize(json);
			std::cout << "[ExamplePluginView] Deserialized successfully" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[ExamplePluginView] Deserialize error: " << e.what() << std::endl;
		}
	}

	static ECS::ComponentTypeID s_componentId;

protected:
	std::string GetWindowTitle() const override {
		try {
			return viewName + "##" + std::to_string(GetID());
		}
		catch (...) {
			return "ExamplePluginView##SAFE";
		}
	}

private:
	ImGuiContext* m_mainImGuiContext;
	bool m_contextValid;

	// CRITICAL: Self-contained context validation and management
	void ValidateContext() {
		m_contextValid = false;

		// Check if we have a main context
		if (!m_mainImGuiContext) {
			std::cout << "[ExamplePluginView] No main context provided" << std::endl;
			return;
		}

		// Check if the main context is still valid by trying to use it
		ImGuiContext* currentContext = ImGui::GetCurrentContext();

		if (currentContext == m_mainImGuiContext) {
			// Already using the right context
			m_contextValid = true;
			std::cout << "[ExamplePluginView] Already using correct context" << std::endl;
			return;
		}

		// Try to switch to the main context
		try {
			ImGui::SetCurrentContext(m_mainImGuiContext);
			ImGuiContext* newCurrent = ImGui::GetCurrentContext();

			if (newCurrent == m_mainImGuiContext) {
				m_contextValid = true;
				std::cout << "[ExamplePluginView] Successfully switched to main context" << std::endl;
			}
			else {
				std::cout << "[ExamplePluginView] Context switch failed" << std::endl;
			}
		}
		catch (...) {
			std::cout << "[ExamplePluginView] Exception during context switch" << std::endl;
		}
	}

	// CRITICAL: Ensure valid context before any ImGui operations
	bool EnsureValidContext() {
		if (m_contextValid) {
			return true;
		}

		// Try to validate/fix the context
		ValidateContext();
		return m_contextValid;
	}

	void CreateExampleEntity() {
		try {
			ECS::EntityID entity = mgr.AddNewEntity();

			if (s_componentId != ECS::MAX_COMPONENT_COUNT) {
				void* component = mgr.AddPluginComponent(entity, s_componentId);
				if (component) {
					ExampleComponent* exampleComp = static_cast<ExampleComponent*>(component);
					exampleComp->message = "Created from self-contained plugin view!";
					exampleComp->value = 100.0f;
					std::cout << "[ExamplePluginView] Created entity " << entity << " with component" << std::endl;
				}
			}
		}
		catch (const std::exception& e) {
			std::cout << "[ExamplePluginView] Error creating entity: " << e.what() << std::endl;
		}
	}

	void RenderEntityList() {
		ImGui::Text("Entities with ExampleComponent:");

		try {
			auto allEntities = mgr.GetAllEntities();
			int entityCount = 0;

			for (auto entity : allEntities) {
				if (mgr.HasPluginComponent(entity, s_componentId)) {
					void* component = mgr.GetPluginComponent(entity, s_componentId);
					if (component) {
						ExampleComponent* exampleComp = static_cast<ExampleComponent*>(component);
						ImGui::Text("Entity %zu: %s (value: %.2f)",
							entity, exampleComp->message.c_str(), exampleComp->value);
						entityCount++;
					}
				}
			}

			if (entityCount == 0) {
				ImGui::Text("No entities found");
			}
		}
		catch (const std::exception& e) {
			ImGui::Text("Error listing entities: %s", e.what());
		}
	}
};

// Initialize static member
ECS::ComponentTypeID ExamplePluginView::s_componentId = ECS::MAX_COMPONENT_COUNT;

// Static variable to share component ID with system creator
static ECS::ComponentTypeID g_exampleComponentId = ECS::MAX_COMPONENT_COUNT;

// Plugin implementation
class ExamplePlugin : public Plugins::BasePlugin {
public:
	ExamplePlugin() : BasePlugin("ExamplePlugin", "1.0.0") {}

	bool OnEngineInit(ECS::EntityManager& entityMgr, Plugins::IPluginRegistry& registry) override {
		LogInfo("Initializing in engine with direct registry access...");

		std::cout << "[ExamplePlugin] Registry available: " << &registry << std::endl;
		std::cout << "[ExamplePlugin] Plugin name from registry: " << registry.GetCurrentPluginName() << std::endl;

		// Register custom component using the registry
		Plugins::ComponentDescriptor compDesc;
		compDesc.name = "ExampleComponent";
		compDesc.size = sizeof(ExampleComponent);
		compDesc.constructor = [](void* memory, ECS::EntityID entity) {
			new(memory) ExampleComponent();
			std::cout << "[ExamplePlugin] Constructed ExampleComponent for entity: " << entity << std::endl;
		};
		compDesc.destructor = [](void* memory) {
			std::cout << "[ExamplePlugin] Destroying ExampleComponent" << std::endl;
			static_cast<ExampleComponent*>(memory)->~ExampleComponent();
		};

		m_componentId = registry.RegisterComponent(compDesc);

		if (m_componentId == ECS::MAX_COMPONENT_COUNT) {
			LogError("Failed to register ExampleComponent");
			return false;
		}

		// Store component ID for both the view and system to use
		ExamplePluginView::s_componentId = m_componentId;
		g_exampleComponentId = m_componentId;

		LogInfo("ExampleComponent registered with ID: " + std::to_string(m_componentId));

		// Register custom system using the registry
		Plugins::SystemDescriptor systemDesc;
		systemDesc.name = "ExampleSystem";
		systemDesc.creator = [](ECS::EntityManager* mgr) -> void* {
			auto system = new ExampleSystem(*mgr);
			system->SetComponentId(g_exampleComponentId);
			return system;
		};
		systemDesc.destructor = [](void* system) {
			delete static_cast<ExampleSystem*>(system);
		};
		systemDesc.updater = [](void* system, float deltaTime) {
			static_cast<ExampleSystem*>(system)->Update(deltaTime);
		};
		systemDesc.requiredComponents = { m_componentId };

		m_systemId = registry.RegisterSystem(systemDesc);

		if (m_systemId == 0) {
			LogError("Failed to register ExampleSystem");
			return false;
		}

		LogInfo("ExampleSystem registered with ID: " + std::to_string(m_systemId));
		LogInfo("Engine initialization successful!");
		return true;
	}

	bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr, Plugins::IPluginRegistry& registry) override {
		LogInfo("Initializing in studio with direct registry access...");

		std::cout << "[ExamplePlugin] StudioInit - Registry available: " << &registry << std::endl;
		std::cout << "[ExamplePlugin] StudioInit - ViewManager available: " << &viewMgr << std::endl;

		// Get the ImGui context from the registry
		ImGuiContext* mainContext = registry.GetImGuiContext();
		std::cout << "[ExamplePlugin] Got ImGui context from registry: " << mainContext << std::endl;

		// Register custom view - SELF-CONTAINED ARCHITECTURE
		Plugins::ViewDescriptor viewDesc;
		viewDesc.name = "ExamplePluginView";
		viewDesc.category = "Tools";

		// SELF-CONTAINED FACTORY: Creates a view that manages its own context
		viewDesc.factory = [mainContext](ECS::EntityManager& mgr, ImGuiContext* factoryContext) -> std::unique_ptr<GUI::BaseView> {
			try {
				std::cout << "[ExamplePlugin] Creating SELF-CONTAINED view" << std::endl;
				std::cout << "[ExamplePlugin] Factory context: " << factoryContext << std::endl;
				std::cout << "[ExamplePlugin] Main context: " << mainContext << std::endl;

				// Create view with the main context - it will manage itself from here
				auto view = std::make_unique<ExamplePluginView>(mgr, mainContext);
				std::cout << "[ExamplePlugin] Self-contained view created successfully" << std::endl;
				return view;
			}
			catch (const std::exception& e) {
				std::cerr << "[ExamplePlugin] Error creating self-contained view: " << e.what() << std::endl;
				return nullptr;
			}
		};

		m_viewId = registry.RegisterView(viewDesc);

		if (m_viewId == GUI::MAX_VIEW_COUNT) {
			LogError("Failed to register ExamplePluginView");
			return false;
		}

		LogInfo("ExamplePluginView registered with ID: " + std::to_string(m_viewId));
		LogInfo("Studio initialization successful!");
		return true;
	}

	void OnUpdate(float deltaTime) override {
		static float timer = 0.0f;
		timer += deltaTime;

		if (timer > 10.0f) {
			LogInfo("Plugin is running smoothly");
			timer = 0.0f;
		}
	}

	void OnShutdown() override {
		LogInfo("Shutting down plugin...");
		LogInfo("Components, systems, and views will be automatically cleaned up");
	}

private:
	ECS::ComponentTypeID m_componentId = ECS::MAX_COMPONENT_COUNT;
	ECS::SystemTypeID m_systemId = 0;
	GUI::ViewTypeID m_viewId = GUI::MAX_VIEW_COUNT;
};

// Plugin exports
extern "C" __declspec(dllexport) Plugins::BasePlugin* CreatePlugin() {
	try {
		return new ExamplePlugin();
	}
	catch (const std::exception& e) {
		std::cerr << "[ExamplePlugin] Exception creating plugin: " << e.what() << std::endl;
		return nullptr;
	}
}

extern "C" __declspec(dllexport) void DestroyPlugin(Plugins::BasePlugin* plugin) {
	try {
		if (plugin) {
			delete plugin;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[ExamplePlugin] Exception destroying plugin: " << e.what() << std::endl;
	}
}