// SamplePlugin.cpp
#include "PluginInterface.hpp"
#include "BaseComponent.hpp"
#include "BaseSystem.hpp"
#include "BaseView.hpp"
#include <iostream>

// Define a custom component
namespace ECS {
	struct CustomColorComponent : public BaseComponent {
		CustomColorComponent() { compName = "CustomColor"; }
		float r = 1.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 1.0f;

		nlohmann::json Serialize() const override {
			nlohmann::json j = BaseComponent::Serialize();
			j[compName]["r"] = r;
			j[compName]["g"] = g;
			j[compName]["b"] = b;
			j[compName]["a"] = a;
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);
			if (j.contains(compName)) {
				auto& data = j[compName];
				if (data.contains("r")) r = data["r"];
				if (data.contains("g")) g = data["g"];
				if (data.contains("b")) b = data["b"];
				if (data.contains("a")) a = data["a"];
			}
		}
	};
}

// Define a custom system
class CustomColorSystem : public ECS::BaseSystem {
public:
	CustomColorSystem(ECS::EntityManager& entityMgr) : BaseSystem(entityMgr) {
		sysName = "CustomColorSystem";
		AddComponentSignature<ECS::CustomColorComponent>();
	}

	void Update(const float deltaT) override {
		for (auto entity : entities) {
			auto& colorComp = mgr.GetComponent<ECS::CustomColorComponent>(entity);
			// Animation effect: cycle through colors
			colorComp.r = (sin(glfwGetTime() * 1.0f) + 1.0f) * 0.5f;
			colorComp.g = (sin(glfwGetTime() * 0.5f) + 1.0f) * 0.5f;
			colorComp.b = (sin(glfwGetTime() * 0.3f) + 1.0f) * 0.5f;
		}
	}
};

// Define a custom view
class CustomView : public GUI::BaseView {
public:
	CustomView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "Custom Color View";
	}

	void Render() override {
		ImGui::Begin(viewName.c_str());

		ImGui::Text("Sample Plugin View");
		ImGui::Separator();

		if (ImGui::Button("Create Custom Entity")) {
			ECS::EntityID newEntity = mgr.AddNewEntity();
			auto& colorComp = mgr.AddComponent<ECS::CustomColorComponent>(newEntity);
		}

		ImGui::Text("Entities with CustomColorComponent:");

		// Get all entities with CustomColorComponent
		for (auto entity : mgr.GetAllEntities()) {
			if (mgr.HasComponent<ECS::CustomColorComponent>(entity)) {
				auto& colorComp = mgr.GetComponent<ECS::CustomColorComponent>(entity);

				ImGui::PushID(static_cast<int>(entity));

				ImGui::Text("Entity %zu:", entity);
				ImGui::SameLine();

				ImGui::ColorEdit4("Color", &colorComp.r);

				ImGui::PopID();
			}
		}

		ImGui::End();
	}
};

// Define the plugin class
class SamplePlugin : public Plugin::IPlugin {
public:
	SamplePlugin() : systemRegistered(false), viewID(0) {}

	~SamplePlugin() override {
		OnUnload();
	}

	void OnLoad(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
		std::cout << "SamplePlugin::OnLoad()" << std::endl;

		// Store references
		this->entityMgr = &entityMgr;
		this->viewMgr = &viewMgr;

		// Register custom component type
		entityMgr.RegisterComponentName<ECS::CustomColorComponent>("CustomColor");

		// Register custom system
		entityMgr.RegisterSystem<CustomColorSystem>();
		systemRegistered = true;

		// Create custom view
		viewID = viewMgr.CreateView();
		viewMgr.AddView<CustomView>(viewID, CustomView(entityMgr));
	}

	void OnUnload() override {
		std::cout << "SamplePlugin::OnUnload()" << std::endl;

		if (viewMgr && viewID != 0) {
			viewMgr->DestroyView(viewID);
			viewID = 0;
		}

		if (entityMgr && systemRegistered) {
			// Unregister system (your EntityManager needs to support this)
			// entityMgr->UnregisterSystem<CustomColorSystem>();
			systemRegistered = false;
		}
	}

	void OnUpdate(float deltaTime) override {
		// Any per-frame updates for the plugin
	}

	std::string GetName() const override {
		return "SamplePlugin";
	}

	std::string GetVersion() const override {
		return "1.0.0";
	}

	std::string GetDescription() const override {
		return "A sample plugin that adds custom color components and a view to manage them";
	}

private:
	ECS::EntityManager* entityMgr = nullptr;
	GUI::ViewManager* viewMgr = nullptr;
	bool systemRegistered = false;
	GUI::ViewListID viewID = 0;
};

// Export plugin creation/destruction functions
extern "C" {
	Plugin::IPlugin* CreatePlugin() {
		return new SamplePlugin();
	}

	void DestroyPlugin(Plugin::IPlugin* plugin) {
		delete plugin;
	}
}