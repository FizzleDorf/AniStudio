#pragma once

#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include "PluginAPI.hpp"
#include <string>
#include <iostream>

namespace ExamplePlugin {

	// Example component
	struct ExampleComponent : public ECS::BaseComponent {
		int counter = 0;
		float value = 0.0f;
		bool enabled = true;
		char text[256] = "Hello World";

		ExampleComponent() {
			compName = "ExampleComponent";
			compCategory = "Example";
		}

		virtual nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"counter", counter},
				{"value", value},
				{"enabled", enabled},
				{"text", std::string(text)}
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
			if (componentData.contains("value"))
				value = componentData["value"];
			if (componentData.contains("enabled"))
				enabled = componentData["enabled"];
			if (componentData.contains("text")) {
				std::string textStr = componentData["text"];
				strncpy_s(text, sizeof(text), textStr.c_str(), _TRUNCATE);
			}
		}
	};

	// Example view - SAFELY handles manager access
	class ExampleView : public GUI::BaseView {
	private:
		ECS::EntityID testEntity = 0;

	public:
		// BaseView constructor takes EntityManager reference
		ExampleView(ECS::EntityManager& entityMgr) : GUI::BaseView(entityMgr) {
			viewName = "Example View";
		}

		void Render() override {
			if (!ImGui::Begin("Example Plugin View")) {
				ImGui::End();
				return;
			}

			// SAFETY CHECK: Ensure mgr is valid before using
			try {
				// Use mgr from BaseView (protected member)
				if (mgr.HasComponent<ExampleComponent>(testEntity)) {
					auto& comp = mgr.GetComponent<ExampleComponent>(testEntity);

					// ImGui controls with proper parameters
					ImGui::InputInt("Counter", &comp.counter);
					ImGui::SliderFloat("Value", &comp.value, 0.0f, 100.0f);
					ImGui::Checkbox("Enabled", &comp.enabled);
					ImGui::InputText("Text", comp.text, sizeof(comp.text));

					if (ImGui::Button("Reset Values")) {
						comp.counter = 0;
						comp.value = 0.0f;
						comp.enabled = true;
						strcpy_s(comp.text, sizeof(comp.text), "Hello World");
					}
				}

				if (ImGui::Button("Create Test Entity")) {
					try {
						testEntity = mgr.AddNewEntity();
						auto& newComp = mgr.AddComponent<ExampleComponent>(testEntity);
						newComp.counter = 42;
					}
					catch (const std::exception& e) {
						ImGui::Text("Error creating entity: %s", e.what());
					}
				}

				if (ImGui::Button("List All Entities")) {
					ImGui::Text("All entities with ExampleComponent:");
					try {
						auto entities = mgr.GetAllEntities();
						for (auto entity : entities) {
							if (mgr.HasComponent<ExampleComponent>(entity)) {
								auto& c = mgr.GetComponent<ExampleComponent>(entity);
								ImGui::Text("Entity %u: counter=%d", entity, c.counter);
							}
						}
					}
					catch (const std::exception& e) {
						ImGui::Text("Error listing entities: %s", e.what());
					}
				}
			}
			catch (const std::exception& e) {
				ImGui::Text("ERROR: %s", e.what());
			}

			ImGui::End();
		}
	};

} // namespace ExamplePlugin

// Main plugin class - FIXED for proper initialization
class ExamplePluginImpl : public Plugin::BasePlugin {
private:
	std::string m_name = "ExamplePlugin";
	std::string m_version = "1.0.0";
	std::string m_description = "Example plugin demonstrating basic functionality";
	bool m_initialized = false;
	GUI::ViewListID m_viewID = 0;

public:
	bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) override {
		try {
			std::cout << "=== EXAMPLE PLUGIN INITIALIZE START ===" << std::endl;

			// SAFETY CHECK: Ensure managers are valid
			if (!&entityManager || !&viewManager) {
				std::cerr << "ExamplePlugin: Invalid managers passed to Initialize!" << std::endl;
				return false;
			}

			std::cout << "EntityManager: " << &entityManager << std::endl;
			std::cout << "ViewManager: " << &viewManager << std::endl;

			// Register our component type with the entity manager
			try {
				entityManager.RegisterComponentName<ExamplePlugin::ExampleComponent>("ExampleComponent");
				std::cout << "Component registered successfully" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Failed to register component: " << e.what() << std::endl;
				return false;
			}

			// Create view using direct ViewManager approach
			try {
				m_viewID = viewManager.CreateView();
				if (m_viewID == 0) {
					std::cerr << "Failed to create view!" << std::endl;
					return false;
				}

				// Create the view instance and add it to the manager
				ExamplePlugin::ExampleView exampleView(entityManager);
				viewManager.AddView<ExamplePlugin::ExampleView>(m_viewID, std::move(exampleView));

				// Initialize the view
				viewManager.GetView<ExamplePlugin::ExampleView>(m_viewID).Init();

				std::cout << "ExamplePlugin: Created view with ID: " << m_viewID << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "ExamplePlugin: Error creating view: " << e.what() << std::endl;
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

	void Shutdown() override {
		try {
			std::cout << "=== EXAMPLE PLUGIN SHUTDOWN START ===" << std::endl;

			if (m_initialized && m_viewID != 0) {
				// Get the view manager safely
				auto* viewMgr = GetViewManager();
				if (viewMgr) {
					try {
						// Remove the view if it exists
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

	void Update(float deltaTime) override {
		if (m_initialized) {
			// Update logic if needed
		}
	}

	const std::string& GetName() const override { return m_name; }
	const std::string& GetVersion() const override { return m_version; }
	const std::string& GetDescription() const override { return m_description; }
	bool HasSettings() const override { return false; }
	std::vector<std::string> GetDependencies() const override { return {}; }
};