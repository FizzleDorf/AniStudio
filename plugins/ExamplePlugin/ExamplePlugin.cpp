#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include <iostream>
#include <string>

// Custom Component
struct ExampleComponent {
	ECS::EntityID entityID;
	std::string message;
	float value;

	ExampleComponent() : message("Hello from plugin!"), value(0.0f) {}
};

// Custom System
class ExampleSystem {
public:
	ExampleSystem(ECS::EntityManager& entityMgr) : m_entityManager(entityMgr) {
		std::cout << "[ExampleSystem] Created" << std::endl;
	}

	void Update(float deltaTime) {
		// System update logic
		static float timer = 0.0f;
		timer += deltaTime;

		if (timer > 1.0f) {
			std::cout << "[ExampleSystem] Update tick" << std::endl;
			timer = 0.0f;
		}
	}

	void Destroy() {
		std::cout << "[ExampleSystem] Destroyed" << std::endl;
	}

private:
	ECS::EntityManager& m_entityManager;
};

// Custom View
class ExampleView : public GUI::BaseView {
public:
	static constexpr const char* GetMetadataJSON() {
		return R"({
            "displayName": "Example Plugin View",
            "category": "Tools",
            "description": "Example view from the plugin system"
        })";
	}

	ExampleView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "ExamplePluginView";
		windowOpen = true;
	}

	void Init() override {
		std::cout << "[ExampleView] Initialized" << std::endl;
	}

	void Update(float deltaT) override {
		// View update logic
	}

	void Render() override {
		if (!windowOpen) return;

		if (ImGui::Begin("Example Plugin View", &windowOpen)) {
			ImGui::Text("This is a custom view from the ExamplePlugin!");
			ImGui::Text("Total entities: %zu", mgr.GetEntityCount());

			if (ImGui::Button("Create Example Entity")) {
				ECS::EntityID entity = mgr.AddNewEntity();
				ImGui::Text("Created entity: %zu", entity);
			}

			// Show plugin info
			ImGui::Separator();
			ImGui::Text("Plugin: ExamplePlugin v1.0.0");
			ImGui::Text("Registration: Direct Manager Pass-Through");
		}
		ImGui::End();
	}
};

// Plugin implementation
class ExamplePlugin : public Plugins::BasePlugin {
public:
	ExamplePlugin() : BasePlugin("ExamplePlugin", "1.0.0") {}

	bool OnEngineInit(ECS::EntityManager& entityMgr, Plugins::IPluginRegistry& registry) override {
		LogInfo("Initializing in engine with direct registry access...");

		// NO MORE STATIC VARIABLES! Registry is passed directly!
		std::cout << "[ExamplePlugin] Registry available: " << &registry << std::endl;
		std::cout << "[ExamplePlugin] Plugin name from registry: " << registry.GetCurrentPluginName() << std::endl;

		// Register custom component using the registry
		m_componentId = registry.RegisterComponent({
			"ExampleComponent",
			sizeof(ExampleComponent),
			[](void* memory, ECS::EntityID entity) {
				new(memory) ExampleComponent();
				static_cast<ExampleComponent*>(memory)->entityID = entity;
				std::cout << "[ExamplePlugin] Constructed ExampleComponent for entity: " << entity << std::endl;
			},
			[](void* memory) {
				std::cout << "[ExamplePlugin] Destroying ExampleComponent" << std::endl;
				static_cast<ExampleComponent*>(memory)->~ExampleComponent();
			}
			});

		if (m_componentId == 0) {
			LogError("Failed to register ExampleComponent");
			return false;
		}

		LogInfo("ExampleComponent registered with ID: " + std::to_string(m_componentId));

		// Register custom system using the registry
		m_systemId = registry.RegisterSystem({
			"ExampleSystem",
			[](ECS::EntityManager* mgr) -> void* {
				return new ExampleSystem(*mgr);
			},
			[](void* system) {
				delete static_cast<ExampleSystem*>(system);
			},
			[](void* system, float deltaTime) {
				static_cast<ExampleSystem*>(system)->Update(deltaTime);
			},
			{m_componentId}
			});

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

		// Register custom view using the registry
		m_viewId = registry.RegisterView({
			"ExampleView",
			"Tools",
			[](ECS::EntityManager* mgr) -> void* {
				return new ExampleView(*mgr);
			},
			[](void* view) {
				delete static_cast<ExampleView*>(view);
			}
			});

		if (m_viewId == 0) {
			LogError("Failed to register ExampleView");
			return false;
		}

		LogInfo("ExampleView registered with ID: " + std::to_string(m_viewId));
		LogInfo("Studio initialization successful!");
		return true;
	}

	void OnUpdate(float deltaTime) override {
		static float timer = 0.0f;
		timer += deltaTime;

		if (timer > 5.0f) { // Less frequent logging
			LogInfo("Plugin is running smoothly");
			timer = 0.0f;
		}
	}

	void OnShutdown() override {
		LogInfo("Shutting down plugin...");
		LogInfo("Components, systems, and views will be automatically cleaned up");
	}

private:
	ECS::ComponentTypeID m_componentId = 0;
	ECS::SystemTypeID m_systemId = 0;
	GUI::ViewTypeID m_viewId = 0;
};

// Plugin exports
extern "C" __declspec(dllexport) Plugins::BasePlugin* CreatePlugin() {
	return new ExamplePlugin();
}

extern "C" __declspec(dllexport) void DestroyPlugin(Plugins::BasePlugin* plugin) {
	delete plugin;
}