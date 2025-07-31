/*
 * ExamplePlugin.cpp - SIMPLE VERSION THAT ACTUALLY WORKS
 * No fancy architecture, just get the damn thing running
 */

#include "PluginAPI.hpp"
#include <iostream>
#include <string>

 // ECS includes - only what we ACTUALLY need
#include "BaseComponent.hpp"
#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "Types.hpp"
#include "nlohmann/json.hpp"

// ============================================================================
// SIMPLE COMPONENT
// ============================================================================

struct SimpleComponent : public ECS::BaseComponent {
	float value = 42.0f;
	std::string message = "Hello from plugin!";
	bool enabled = true;

	SimpleComponent() {
		compName = "SimpleComponent";
		compCategory = "Plugin";
	}

	nlohmann::json Serialize() const override {
		nlohmann::json json;
		json[compName]["value"] = value;
		json[compName]["message"] = message;
		json[compName]["enabled"] = enabled;
		return json;
	}

	void Deserialize(const nlohmann::json& json) override {
		if (json.contains(compName)) {
			const auto& comp = json[compName];
			if (comp.contains("value")) value = comp["value"];
			if (comp.contains("message")) message = comp["message"];
			if (comp.contains("enabled")) enabled = comp["enabled"];
		}
	}
};

// ============================================================================
// SIMPLE SYSTEM
// ============================================================================

class SimpleSystem : public ECS::BaseSystem {
private:
	float timer = 0.0f;

public:
	SimpleSystem(ECS::EntityManager& entityMgr) : BaseSystem(entityMgr) {
		sysName = "SimpleSystem";

		// Get component ID and add to signature
		ECS::ComponentTypeID compId = ECS::ComponentTypeRegistry::GetIDByName("SimpleComponent");
		if (compId != ECS::MAX_COMPONENT_COUNT) {
			signature.insert(compId);
			std::cout << "[SimpleSystem] Registered for SimpleComponent" << std::endl;
		}
	}

	void Start() override {
		std::cout << "[SimpleSystem] Started with " << entities.size() << " entities" << std::endl;
	}

	void Update(float deltaTime) override {
		timer += deltaTime;

		// Do something every 5 seconds
		if (timer >= 5.0f) {
			timer = 0.0f;

			std::cout << "[SimpleSystem] Processing " << entities.size() << " entities:" << std::endl;

			for (ECS::EntityID entity : entities) {
				SimpleComponent& comp = mgr.GetComponent<SimpleComponent>(entity);
				if (comp.enabled) {
					comp.value += 1.0f;
					std::cout << "  Entity " << entity << ": " << comp.message
						<< " (value: " << comp.value << ")" << std::endl;
				}
			}
		}
	}

	void Destroy() override {
		std::cout << "[SimpleSystem] Destroyed" << std::endl;
	}
};

// ============================================================================
// SIMPLE PLUGIN
// ============================================================================

class SimplePlugin : public Plugin::IPlugin {
private:
	Plugin::PluginContext* context = nullptr;
	bool initialized = false;

public:
	bool Initialize(Plugin::PluginContext* ctx) override {
		if (!ctx || !ctx->entityManager) {
			std::cerr << "[SimplePlugin] ERROR: No EntityManager!" << std::endl;
			return false;
		}

		context = ctx;
		std::cout << "[SimplePlugin] Initializing..." << std::endl;

		try {
			// Register component
			context->entityManager->RegisterComponentName<SimpleComponent>("SimpleComponent");
			std::cout << "[SimplePlugin] SimpleComponent registered" << std::endl;

			// Register system
			context->entityManager->RegisterSystem<SimpleSystem>();
			std::cout << "[SimplePlugin] SimpleSystem registered" << std::endl;

			// Create test entity
			ECS::EntityID entity = context->entityManager->AddNewEntity();
			SimpleComponent& comp = context->entityManager->AddComponent<SimpleComponent>(entity);
			comp.message = "Plugin working!";
			comp.value = 100.0f;

			std::cout << "[SimplePlugin] Created test entity " << entity << std::endl;

			initialized = true;
			std::cout << "[SimplePlugin] *** PLUGIN LOADED SUCCESSFULLY! ***" << std::endl;
			return true;

		}
		catch (const std::exception& e) {
			std::cerr << "[SimplePlugin] Exception: " << e.what() << std::endl;
			return false;
		}
	}

	void Shutdown() override {
		if (initialized) {
			std::cout << "[SimplePlugin] Shutting down..." << std::endl;
			initialized = false;
			context = nullptr;
		}
	}

	void Update(float deltaTime) override {
		// Optional: do plugin-level updates
	}

	const char* GetName() const override {
		return "Simple Plugin";
	}

	const char* GetVersion() const override {
		return "1.0.0";
	}

	const char* GetDescription() const override {
		return "Simple plugin that just fucking works";
	}
};

IMPLEMENT_PLUGIN(SimplePlugin, "Simple Plugin", "1.0.0", "Simple plugin that just fucking works")