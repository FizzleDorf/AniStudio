// ExamplePlugin.cpp - Simple plugin that works with YOUR existing PluginAPI
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations to match your PluginAPI.hpp
namespace ECS {
	using EntityID = size_t;
	class EntityManager;
}

namespace GUI {
	using ViewListID = size_t;
	class ViewManager;
}

// Platform-specific plugin export macros
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

// Include ImGui for rendering
#include <imgui.h>

// ============================================================================
// SIMPLE PLUGIN COMPONENTS - Stored internally in plugin
// ============================================================================

struct Transform2D {
	float x = 0.0f;
	float y = 0.0f;
	float rotation = 0.0f;
	float scaleX = 1.0f;
	float scaleY = 1.0f;
};

struct Velocity2D {
	float vx = 0.0f;
	float vy = 0.0f;
	float maxSpeed = 300.0f;
};

struct PlayerController {
	float moveSpeed = 200.0f;
	float jumpForce = 400.0f;
	bool isGrounded = false;
	bool canDoubleJump = true;
	bool hasDoubleJumped = false;
};

// Component storage - simple maps stored in plugin
static std::unordered_map<ECS::EntityID, Transform2D> g_transforms;
static std::unordered_map<ECS::EntityID, Velocity2D> g_velocities;
static std::unordered_map<ECS::EntityID, PlayerController> g_controllers;

// ============================================================================
// SIMPLE PLUGIN CLASS - Matches YOUR BasePlugin interface
// ============================================================================

class SimplePlatformerPlugin {
private:
	ECS::EntityManager* entityManager = nullptr;
	GUI::ViewManager* viewManager = nullptr;

	// Game state
	ECS::EntityID playerEntity = 0;
	std::vector<ECS::EntityID> platformEntities;
	float gameTime = 0.0f;
	bool gameRunning = true;
	bool initialized = false;

public:
	// Plugin lifecycle - matches YOUR BasePlugin interface
	bool Initialize(ECS::EntityManager& entityMgr, GUI::ViewManager* viewMgr = nullptr) {
		std::cout << "[SimplePlatformerPlugin] Initializing..." << std::endl;

		entityManager = &entityMgr;
		viewManager = viewMgr;

		CreateGameEntities();

		initialized = true;
		std::cout << "[SimplePlatformerPlugin] Initialized successfully!" << std::endl;
		return true;
	}

	void Shutdown() {
		std::cout << "[SimplePlatformerPlugin] Shutting down..." << std::endl;

		// Clear component storage
		g_transforms.clear();
		g_velocities.clear();
		g_controllers.clear();

		initialized = false;
	}

	void Update(float deltaTime) {
		if (!initialized || !gameRunning) return;

		gameTime += deltaTime;
		UpdateInput(deltaTime);
		UpdatePhysics(deltaTime);
		RenderGame();
	}

	// Plugin information
	const std::string& GetName() const {
		static std::string name = "Simple 2D Platformer";
		return name;
	}

	const std::string& GetVersion() const {
		static std::string version = "1.0.0";
		return version;
	}

	const std::string& GetDescription() const {
		static std::string desc = "A simple 2D platformer game plugin";
		return desc;
	}

	// Static versions for C interface
	static const char* StaticGetName() { return "Simple 2D Platformer"; }
	static const char* StaticGetVersion() { return "1.0.0"; }
	static const char* StaticGetDescription() { return "A simple 2D platformer game plugin"; }

	bool HasSettings() const { return false; }
	void ShowSettings() {}
	bool CanReload() const { return true; }

private:
	void CreateGameEntities() {
		if (!entityManager) return;

		// Create player entity
		playerEntity = entityManager->AddNewEntity();
		std::cout << "[SimplePlatformerPlugin] Created player entity: " << playerEntity << std::endl;

		// Add components to player (stored in plugin)
		g_transforms[playerEntity] = Transform2D{ 100.0f, 300.0f, 0.0f, 1.0f, 1.0f };
		g_velocities[playerEntity] = Velocity2D{ 0.0f, 0.0f, 300.0f };
		g_controllers[playerEntity] = PlayerController{ 200.0f, 400.0f, false, true, false };

		// Create platform entities
		platformEntities.clear();
		for (int i = 0; i < 3; i++) {
			ECS::EntityID platform = entityManager->AddNewEntity();
			g_transforms[platform] = Transform2D{ 200.0f + i * 150.0f, 450.0f - i * 50.0f, 0.0f, 1.0f, 1.0f };
			platformEntities.push_back(platform);
		}

		std::cout << "[SimplePlatformerPlugin] Created " << (platformEntities.size() + 1) << " entities" << std::endl;
	}

	void UpdateInput(float deltaTime) {
		auto transformIt = g_transforms.find(playerEntity);
		auto velocityIt = g_velocities.find(playerEntity);
		auto controllerIt = g_controllers.find(playerEntity);

		if (transformIt == g_transforms.end() || velocityIt == g_velocities.end() || controllerIt == g_controllers.end()) {
			return;
		}

		Velocity2D& velocity = velocityIt->second;
		PlayerController& controller = controllerIt->second;

		// Movement
		float moveInput = 0.0f;
		if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) moveInput -= 1.0f;
		if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) moveInput += 1.0f;

		velocity.vx = moveInput * controller.moveSpeed;

		// Jumping
		if ((ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsKeyPressed(ImGuiKey_W)) && controller.isGrounded) {
			velocity.vy = -controller.jumpForce;
			controller.isGrounded = false;
		}
	}

	void UpdatePhysics(float deltaTime) {
		auto transformIt = g_transforms.find(playerEntity);
		auto velocityIt = g_velocities.find(playerEntity);
		auto controllerIt = g_controllers.find(playerEntity);

		if (transformIt == g_transforms.end() || velocityIt == g_velocities.end() || controllerIt == g_controllers.end()) {
			return;
		}

		Transform2D& transform = transformIt->second;
		Velocity2D& velocity = velocityIt->second;
		PlayerController& controller = controllerIt->second;

		const float GRAVITY = 980.0f;
		const float GROUND_Y = 400.0f;

		// Apply gravity
		velocity.vy += GRAVITY * deltaTime;

		// Update position
		transform.x += velocity.vx * deltaTime;
		transform.y += velocity.vy * deltaTime;

		// Ground collision
		if (transform.y > GROUND_Y) {
			transform.y = GROUND_Y;
			velocity.vy = 0.0f;
			controller.isGrounded = true;
			controller.hasDoubleJumped = false;
		}
		else {
			controller.isGrounded = false;
		}

		// Screen bounds
		if (transform.x < 0) { transform.x = 0; velocity.vx = 0; }
		if (transform.x > 800) { transform.x = 800; velocity.vx = 0; }
	}

	void RenderGame() {
		if (ImGui::Begin("Simple 2D Platformer", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Simple Plugin - No Complex Registration Needed!");
			ImGui::Separator();

			ImGui::Text("Game Time: %.1fs", gameTime);
			ImGui::Text("Player Entity: %zu", playerEntity);
			ImGui::Text("Platform Entities: %zu", platformEntities.size());

			if (ImGui::Button(gameRunning ? "Pause" : "Play")) {
				gameRunning = !gameRunning;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset")) {
				CreateGameEntities();
				gameTime = 0.0f;
			}

			ImGui::Text("Use WASD or Arrow Keys to move, Space to jump");

			// Game viewport
			if (ImGui::BeginChild("GameViewport", ImVec2(500, 350), true)) {
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				ImVec2 canvasPos = ImGui::GetCursorScreenPos();
				ImVec2 canvasSize = ImGui::GetContentRegionAvail();

				// Background
				drawList->AddRectFilled(canvasPos,
					ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
					IM_COL32(135, 206, 235, 255));

				// Ground
				float groundY = canvasPos.y + canvasSize.y * 0.8f;
				drawList->AddRectFilled(
					ImVec2(canvasPos.x, groundY),
					ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
					IM_COL32(34, 139, 34, 255));

				// Render player
				auto playerTransform = g_transforms.find(playerEntity);
				if (playerTransform != g_transforms.end()) {
					const Transform2D& transform = playerTransform->second;
					float screenX = canvasPos.x + (transform.x / 800.0f) * canvasSize.x;
					float screenY = canvasPos.y + (transform.y / 600.0f) * canvasSize.y;

					drawList->AddRectFilled(
						ImVec2(screenX, screenY),
						ImVec2(screenX + 32, screenY + 32),
						IM_COL32(255, 100, 100, 255));

					// Show entity ID
					char idText[32];
					snprintf(idText, sizeof(idText), "P:%zu", playerEntity);
					drawList->AddText(ImVec2(screenX, screenY - 15),
						IM_COL32(255, 255, 255, 255), idText);
				}

				// Render platforms
				for (auto platformID : platformEntities) {
					auto platformTransform = g_transforms.find(platformID);
					if (platformTransform != g_transforms.end()) {
						const Transform2D& transform = platformTransform->second;
						float screenX = canvasPos.x + (transform.x / 800.0f) * canvasSize.x;
						float screenY = canvasPos.y + (transform.y / 600.0f) * canvasSize.y;

						drawList->AddRectFilled(
							ImVec2(screenX, screenY),
							ImVec2(screenX + 150, screenY + 20),
							IM_COL32(139, 69, 19, 255));

						// Show entity ID
						char idText[32];
						snprintf(idText, sizeof(idText), "PL:%zu", platformID);
						drawList->AddText(ImVec2(screenX, screenY - 15),
							IM_COL32(255, 255, 255, 255), idText);
					}
				}

				drawList->AddText(ImVec2(canvasPos.x + 5, canvasPos.y + 5),
					IM_COL32(255, 255, 255, 255), "WASD: Move | Space: Jump");
			}
			ImGui::EndChild();

			// Component debug info
			if (ImGui::CollapsingHeader("Component Debug")) {
				ImGui::Text("Transform2D Components: %zu", g_transforms.size());
				ImGui::Text("Velocity2D Components: %zu", g_velocities.size());
				ImGui::Text("PlayerController Components: %zu", g_controllers.size());
			}
		}
		ImGui::End();
	}
};

// ============================================================================
// PLUGIN INTERFACE IMPLEMENTATION - YOUR required C interface
// ============================================================================

namespace Plugin {
	class BasePlugin {
	public:
		virtual ~BasePlugin() = default;
		virtual bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager* viewManager = nullptr) = 0;
		virtual void Shutdown() = 0;
		virtual void Update(float deltaTime) = 0;
		virtual const std::string& GetName() const = 0;
		virtual const std::string& GetVersion() const = 0;
		virtual const std::string& GetDescription() const = 0;
		virtual bool HasSettings() const = 0;
		virtual void ShowSettings() = 0;
		virtual bool CanReload() const = 0;
	};
}

// Wrapper class that implements YOUR BasePlugin interface
class PluginWrapper : public Plugin::BasePlugin {
private:
	std::unique_ptr<SimplePlatformerPlugin> plugin;

public:
	PluginWrapper() : plugin(std::make_unique<SimplePlatformerPlugin>()) {}

	bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager* viewManager = nullptr) override {
		return plugin->Initialize(entityManager, viewManager);
	}

	void Shutdown() override {
		plugin->Shutdown();
	}

	void Update(float deltaTime) override {
		plugin->Update(deltaTime);
	}

	const std::string& GetName() const override {
		return plugin->GetName();
	}

	const std::string& GetVersion() const override {
		return plugin->GetVersion();
	}

	const std::string& GetDescription() const override {
		return plugin->GetDescription();
	}

	bool HasSettings() const override {
		return plugin->HasSettings();
	}

	void ShowSettings() override {
		plugin->ShowSettings();
	}

	bool CanReload() const override {
		return plugin->CanReload();
	}
};

// ============================================================================
// C INTERFACE - Required by YOUR PluginManager
// ============================================================================

static std::unique_ptr<PluginWrapper> g_pluginInstance;

extern "C" {
	PLUGIN_API Plugin::BasePlugin* CreatePlugin() {
		g_pluginInstance = std::make_unique<PluginWrapper>();
		return g_pluginInstance.get();
	}

	PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin) {
		g_pluginInstance.reset();
	}

	PLUGIN_API const char* GetPluginName() {
		return SimplePlatformerPlugin::StaticGetName();
	}

	PLUGIN_API const char* GetPluginVersion() {
		return SimplePlatformerPlugin::StaticGetVersion();
	}

	PLUGIN_API const char* GetPluginDescription() {
		return SimplePlatformerPlugin::StaticGetDescription();
	}

	PLUGIN_API bool HasPluginSettings() {
		return g_pluginInstance ? g_pluginInstance->HasSettings() : false;
	}

	PLUGIN_API bool CanPluginReload() {
		return g_pluginInstance ? g_pluginInstance->CanReload() : true;
	}
}