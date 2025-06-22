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

// Main plugin class - FIXED with all required method declarations
class ExamplePluginImpl : public Plugin::BasePlugin {
private:
	std::string m_name = "ExamplePlugin";
	std::string m_version = "2.0.0";
	std::string m_description = "Example plugin demonstrating basic functionality";
	bool m_initialized = false;
	GUI::ViewListID m_viewID = 0;

public:
	// Default constructor
	ExamplePluginImpl();

	// Core plugin lifecycle - these are REQUIRED by BasePlugin
	bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) override;
	void Shutdown() override;
	void Update(float deltaTime) override;

	// Plugin information - these are REQUIRED by BasePlugin
	const std::string& GetName() const override;
	const std::string& GetVersion() const override;
	const std::string& GetDescription() const override;
	bool HasSettings() const override;
	std::vector<std::string> GetDependencies() const override;

	// Hot reload support methods - ADD THESE DECLARATIONS
	bool CanReload() const;
	void SaveState();
	void LoadState();
	void OnPreReload();
	void OnPostReload();
};