#include "BasePlugin.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "BaseView.hpp"
#include "BaseComponent.hpp"
#include "BaseSystem.hpp"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <random>

// ============================================
// Custom Component - derives from BaseComponent
// ============================================
struct ExampleComponent : public ECS::BaseComponent {
	std::string message;
	float value;
	int counter;

	ExampleComponent() : BaseComponent(),
		message("Hello from plugin!"), value(0.0f), counter(0) {
		compName = "ExampleComponent";
	}

	nlohmann::json Serialize() const override {
		nlohmann::json j = BaseComponent::Serialize();
		j[compName]["message"] = message;
		j[compName]["value"] = value;
		j[compName]["counter"] = counter;
		return j;
	}

	void Deserialize(const nlohmann::json& j) override {
		BaseComponent::Deserialize(j);
		if (j.contains(compName)) {
			auto data = j.at(compName);
			if (data.contains("message")) message = data["message"];
			if (data.contains("value")) value = data["value"];
			if (data.contains("counter")) counter = data["counter"];
		}
	}
};

// ============================================
// Custom System - uses template-based ECS
// ============================================
class ExampleSystem : public ECS::BaseSystem {
public:
	ExampleSystem(ECS::EntityManager& entityMgr) : BaseSystem(entityMgr) {
		sysName = "ExampleSystem";
		AddComponentSignature<ExampleComponent>();
	}

	void Start() override {
		std::cout << "[ExampleSystem] System started" << std::endl;
	}

	void Update(float deltaTime) override {
		static float timer = 0.0f;
		timer += deltaTime;

		if (timer > 2.5f) {
			int count = 0;
			float totalValue = 0.0f;

			for (auto entity : entities) {
				if (mgr.HasComponent<ExampleComponent>(entity)) {
					auto& comp = mgr.GetComponent<ExampleComponent>(entity);
					totalValue += comp.value;
					count++;
				}
			}

			if (count > 0) {
				std::cout << "[ExampleSystem] Tracking " << count
					<< " entities, average value: " << (totalValue / count) << std::endl;
			}
			timer = 0.0f;
		}
	}

	void Destroy() override {
		std::cout << "[ExampleSystem] System destroyed" << std::endl;
	}
};

// ============================================
// Plugin View
// ============================================
class ExamplePluginView : public GUI::BaseView {
public:
	ExamplePluginView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "ExamplePluginView";
		windowOpen = true;
	}

	void Init() override {
		std::cout << "[ExamplePluginView] View initialized" << std::endl;
	}

	void Update(float deltaT) override {
		// Optional: Add per-frame logic here
	}

	void Render() override {
		if (!windowOpen) return;

		if (ImGui::Begin("Example Plugin View", &windowOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("HOT RELOAD WORKING! Direct manager access!");
			ImGui::Text("Build Time: %s %s", __DATE__, __TIME__);
			ImGui::Text("Entities: %zu", mgr.GetEntityCount());

			ImGui::Separator();

			if (ImGui::Button("Create Entity")) {
				CreateExampleEntity();
			}

			ImGui::SameLine();

			if (ImGui::Button("Remove All")) {
				RemoveAllExampleEntities();
			}

			ImGui::SameLine();

			if (ImGui::Button("Randomize Values")) {
				RandomizeAllValues();
			}

			ImGui::Separator();
			RenderEntities();
		}
		ImGui::End();
	}

	nlohmann::json Serialize() const override {
		auto json = BaseView::Serialize();
		json["plugin_data"] = "example";
		json["entity_count"] = m_entityCounter;
		return json;
	}

	void Deserialize(const nlohmann::json& json) override {
		BaseView::Deserialize(json);
		if (json.contains("entity_count")) {
			m_entityCounter = json["entity_count"];
		}
	}

	static constexpr const char* GetMetadataJSON() {
		return R"({
			"displayName": "Example Plugin",
			"category": "Tools",
			"description": "Simplified plugin with direct manager access"
		})";
	}

private:
	int m_entityCounter = 0;
	std::random_device m_rd;
	std::mt19937 m_gen{ m_rd() };
	std::uniform_real_distribution<float> m_dist{ 0.0f, 100.0f };

	void CreateExampleEntity() {
		auto entity = mgr.AddNewEntity();
		auto& comp = mgr.AddComponent<ExampleComponent>(entity);

		comp.message = "Direct entity #" + std::to_string(++m_entityCounter);
		comp.value = m_dist(m_gen);
		comp.counter = m_entityCounter;

		std::cout << "[ExamplePlugin] Created entity " << entity << std::endl;
	}

	void RemoveAllExampleEntities() {
		auto entities = mgr.GetAllEntities();
		std::vector<ECS::EntityID> toRemove;

		for (auto entity : entities) {
			if (mgr.HasComponent<ExampleComponent>(entity)) {
				toRemove.push_back(entity);
			}
		}

		for (auto entity : toRemove) {
			mgr.DestroyEntity(entity);
		}

		m_entityCounter = 0;
		std::cout << "[ExamplePlugin] Removed " << toRemove.size() << " entities" << std::endl;
	}

	void RandomizeAllValues() {
		auto entities = mgr.GetAllEntities();
		int count = 0;

		for (auto entity : entities) {
			if (mgr.HasComponent<ExampleComponent>(entity)) {
				auto& comp = mgr.GetComponent<ExampleComponent>(entity);
				comp.value = m_dist(m_gen);
				count++;
			}
		}

		std::cout << "[ExamplePlugin] Randomized " << count << " values" << std::endl;
	}

	void RenderEntities() {
		auto entities = mgr.GetAllEntities();
		int exampleCount = 0;

		ImGui::Text("Example Entities:");
		ImGui::Indent();

		for (auto entity : entities) {
			if (mgr.HasComponent<ExampleComponent>(entity)) {
				auto& comp = mgr.GetComponent<ExampleComponent>(entity);
				ImGui::PushID(static_cast<int>(entity));

				ImGui::Text("Entity %zu:", entity);
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", comp.message.c_str());

				ImGui::Indent();
				ImGui::Text("Value: %.1f", comp.value);
				ImGui::Text("Counter: %d", comp.counter);
				ImGui::Unindent();

				if (ImGui::Button("Delete")) {
					mgr.DestroyEntity(entity);
				}

				ImGui::PopID();
				ImGui::Separator();
				exampleCount++;
			}
		}

		if (exampleCount == 0) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No example entities found.");
			ImGui::Text("Click 'Create Entity' to add one.");
		}

		ImGui::Unindent();
	}
};

// ============================================
// MAIN PLUGIN CLASS - SIMPLIFIED
// ============================================
class ExamplePlugin : public Plugins::BasePlugin {
public:
	ExamplePlugin() : BasePlugin("ExamplePlugin", "2.1.0") {
		LogInfo("Plugin constructed");
	}

	~ExamplePlugin() {
		LogInfo("Plugin destructed");
	}

	// ============================================
	// DIRECT MANAGER ACCESS - NO REGISTRY
	// ============================================
	bool OnEngineInit(ECS::EntityManager& entityMgr) override {
		LogInfo("ENGINE INIT - Direct access v2.1.0");

		m_entityMgr = &entityMgr;

		// REGISTER DIRECTLY WITH ENTITY MANAGER
		LogInfo("Registering component...");
		entityMgr.RegisterComponent<ExampleComponent>("ExampleComponent");

		LogInfo("Registering system...");
		entityMgr.RegisterSystem<ExampleSystem>();

		LogInfo("Engine init complete!");
		return true;
	}

	bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
		LogInfo("STUDIO INIT - Direct access v2.1.0");

		// First do engine init
		if (!OnEngineInit(entityMgr)) {
			LogError("Failed to initialize engine components");
			return false;
		}

		m_viewMgr = &viewMgr;

		// REGISTER DIRECTLY WITH VIEW MANAGER
		LogInfo("Registering view...");
		viewMgr.RegisterView<ExamplePluginView>("ExamplePluginView", "ExamplePlugin");

		// Create view instance
		LogInfo("Creating view instance...");
		m_viewId = viewMgr.CreateViewByName("ExamplePluginView", entityMgr);

		if (m_viewId == GUI::MAX_VIEW_COUNT) {
			LogError("Failed to create view instance");
			return false;
		}

		LogInfo("View created with ID: " + std::to_string(m_viewId));

		return true;
	}

	void OnUpdate(float deltaTime) override {
		static float timer = 0.0f;
		timer += deltaTime;

		if (timer > 5.0f) {
			if (m_entityMgr) {
				auto entities = m_entityMgr->GetAllEntities();
				int exampleCount = 0;

				for (auto entity : entities) {
					if (m_entityMgr->HasComponent<ExampleComponent>(entity)) {
						exampleCount++;
					}
				}

				LogInfo("Status - Example entities: " + std::to_string(exampleCount) +
					" (Total: " + std::to_string(entities.size()) + ")");
			}
			timer = 0.0f;
		}
	}

	void OnShutdown() override {
		LogInfo("Shutdown complete");
		m_entityMgr = nullptr;
		m_viewMgr = nullptr;
		m_viewId = GUI::MAX_VIEW_COUNT;
	}

private:
	ECS::EntityManager* m_entityMgr = nullptr;
	GUI::ViewManager* m_viewMgr = nullptr;
	GUI::ViewTypeID m_viewId = GUI::MAX_VIEW_COUNT;
};

// ============================================
// EXPORTS
// ============================================
extern "C" {
	__declspec(dllexport) Plugins::BasePlugin* CreatePlugin() {
		std::cout << "[ExamplePlugin] CREATE PLUGIN v2.1.0" << std::endl;
		return new ExamplePlugin();
	}

	__declspec(dllexport) void DestroyPlugin(Plugins::BasePlugin* plugin) {
		std::cout << "[ExamplePlugin] DESTROY PLUGIN v2.1.0" << std::endl;
		delete plugin;
	}
}