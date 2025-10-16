#pragma once
#include <string>
#include <iostream>

// Forward declarations only
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

// Forward declaration for ImGui
struct ImGuiContext;

namespace Plugins {

	class BasePlugin {
	public:
		BasePlugin(const std::string& name, const std::string& version = "1.0.0")
			: name(name), version(version) {}
		virtual ~BasePlugin() = default;

		// Plugin info
		const std::string& GetName() const { return name; }
		const std::string& GetVersion() const { return version; }
		bool IsInitialized() const { return initialized; }

		// Internal state management - called by PluginManager
		void SetInitialized(bool init) { this->initialized = init; }

		// ============================================
		// IMGUI CONTEXT MANAGEMENT - ADD THESE METHODS
		// ============================================
		virtual void SetImGuiContext(ImGuiContext* context) {
			// Default implementation does nothing
			// Plugins that need ImGui should override this
		}

		virtual bool HasValidImGuiContext() const {
			return false;
		}

		// ============================================
		// SIMPLIFIED: Direct manager access, no registry!
		// ============================================

		// Engine-only plugins (ECS without GUI)
		virtual bool OnEngineInit(ECS::EntityManager& entityMgr) {
			return true;
		}

		// Studio plugins (ECS + GUI)
		// Default implementation just calls OnEngineInit
		virtual bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) {
			return OnEngineInit(entityMgr);
		}

		// Lifecycle callbacks
		virtual void OnShutdown() {}
		virtual void OnUpdate(float deltaTime) {}

	protected:
		void LogInfo(const std::string& msg) const {
			std::cout << "[" << name << "] " << msg << std::endl;
		}

		void LogError(const std::string& msg) const {
			std::cerr << "[" << name << "] ERROR: " << msg << std::endl;
		}

	private:
		std::string name;
		std::string version;
		bool initialized = false;
	};

} // namespace Plugins

// Plugin export macros
#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif