/*
	ExamplePlugin.cpp - Example plugin for AniStudio

	This demonstrates how to create a plugin that extends AniStudio's functionality
	by adding new components, systems, and views.
*/

#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "BaseComponent.hpp"
#include "BaseSystem.hpp"
#include "BaseView.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include <imgui.h>
#include <iostream>

// Example custom component
namespace ExamplePlugin {

	struct CustomComponent : public ECS::BaseComponent {
		float value = 1.0f;
		std::string message = "Hello from plugin!";
		bool enabled = true;

		CustomComponent() {
			compName = "CustomComponent";
			compCategory = "Example";
		}

		virtual nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"value", value},
				{"message", message},
				{"enabled", enabled}
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

			if (componentData.contains("value"))
				value = componentData["value"];
			if (componentData.contains("message"))
				message = componentData["message"];
			if (componentData.contains("enabled"))
				enabled = componentData["enabled"];
		}
	};

	// Example custom system
	class CustomSystem : public ECS::BaseSystem {
	public:
		CustomSystem(ECS::EntityManager& entityMgr) : BaseSystem(entityMgr) {
			sysName = "CustomSystem";
			AddComponentSignature<CustomComponent>();
		}

		void Start() override {
			std::cout << "CustomSystem started with " << entities.size() << " entities" << std::endl;
		}

		void Update(const float deltaT) override {
			for (auto entity : entities) {
				if (mgr.HasComponent<CustomComponent>(entity)) {
					auto& customComp = mgr.GetComponent<CustomComponent>(entity);
					if (customComp.enabled) {
						// Update the component value over time
						customComp.value += deltaT * 0.5f;
						if (customComp.value > 10.0f) {
							customComp.value = 1.0f;
						}
					}
				}
			}
		}

		void Destroy() override {
			std::cout << "CustomSystem destroyed" << std::endl;
		}
	};

	// Example custom view
	class CustomView : public GUI::BaseView {
	public:
		CustomView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
			viewName = "CustomView";
		}

		void Init() override {
			// Create an entity with our custom component for demonstration
			ECS::EntityID entity = mgr.AddNewEntity();
			mgr.AddComponent<CustomComponent>(entity);

			std::cout << "CustomView initialized with entity " << entity << std::endl;
		}

		void Render() override {
			if (ImGui::Begin("Custom Plugin View")) {
				ImGui::Text("This is a view from the example plugin!");

				// Show all entities with CustomComponent
				auto entities = mgr.GetAllEntities();
				for (auto entity : entities) {
					if (mgr.HasComponent<CustomComponent>(entity)) {
						auto& customComp = mgr.GetComponent<CustomComponent>(entity);

						ImGui::Separator();
						ImGui::Text("Entity ID: %zu", entity);

						ImGui::SliderFloat("Value", &customComp.value, 0.0f, 10.0f);
						ImGui::Checkbox("Enabled", &customComp.enabled);

						// Text input for message
						char buffer[256];
						strcpy(buffer, customComp.message.c_str());
						if (ImGui::InputText("Message", buffer, sizeof(buffer))) {
							customComp.message = buffer;
						}

						ImGui::Text("Message: %s", customComp.message.c_str());
					}
				}

				if (ImGui::Button("Create New Entity")) {
					ECS::EntityID newEntity = mgr.AddNewEntity();
					mgr.AddComponent<CustomComponent>(newEntity);
					std::cout << "Created new entity " << newEntity << " with CustomComponent" << std::endl;
				}
			}
			ImGui::End();
		}
	};

} // namespace ExamplePlugin

// Plugin implementation
class ExamplePluginImpl : public Plugin::BasePlugin {
private:
	// Store plugin information as member variables
	std::string m_name = "ExamplePlugin";
	std::string m_version = "1.0.0";
	std::string m_description = "An example plugin demonstrating AniStudio plugin capabilities";
	std::string m_author = "AniStudio Team";

public:
	ExamplePluginImpl() = default;
	~ExamplePluginImpl() override = default;

	bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) override {
		std::cout << "Initializing ExamplePlugin v" << m_version << std::endl;

		try {
			// Register our custom component using the PluginRegistry
			Plugin::PluginRegistry::RegisterComponent<ExamplePlugin::CustomComponent>("CustomComponent");

			// Register our custom system using the PluginRegistry
			Plugin::PluginRegistry::RegisterSystem<ExamplePlugin::CustomSystem>();

			// Register our custom view type using the PluginRegistry
			Plugin::PluginRegistry::RegisterView<ExamplePlugin::CustomView>("CustomView");

			// Create and add an instance of our custom view using the PluginRegistry
			auto viewID = Plugin::PluginRegistry::CreateView<ExamplePlugin::CustomView>("CustomView");

			if (viewID == 0) {
				std::cerr << "Failed to create CustomView instance" << std::endl;
				return false;
			}

			std::cout << "ExamplePlugin initialized successfully!" << std::endl;
			SetInitialized(true);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to initialize ExamplePlugin: " << e.what() << std::endl;
			return false;
		}
	}

	void Shutdown() override {
		std::cout << "Shutting down ExamplePlugin" << std::endl;
		SetInitialized(false);
		// Cleanup code here if needed
	}

	void Update(float deltaTime) override {
		// Plugin-specific update logic if needed
		// Most logic should be in systems, but global plugin state can be updated here
	}

	// Implement pure virtual functions from BasePlugin
	const std::string& GetName() const override {
		return m_name;
	}

	const std::string& GetVersion() const override {
		return m_version;
	}

	const std::string& GetDescription() const override {
		return m_description;
	}

	// Optional plugin capabilities
	bool HasSettings() const override {
		return false; // No settings UI for this example
	}

	std::vector<std::string> GetDependencies() const override {
		return {}; // No dependencies for this example
	}
};

// Plugin entry point - this is what AniStudio will call to get the plugin instance
extern "C" PLUGIN_API Plugin::BasePlugin* CreatePlugin() {
	return new ExamplePluginImpl();
}

extern "C" PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin) {
	delete plugin;
}

extern "C" PLUGIN_API const char* GetPluginName() {
	return "ExamplePlugin";
}

extern "C" PLUGIN_API const char* GetPluginVersion() {
	return "1.0.0";
}

extern "C" PLUGIN_API const char* GetPluginAPIVersion() {
	return "1.0.0";
}