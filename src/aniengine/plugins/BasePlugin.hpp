#pragma once
#include <string>
#include <iostream>
#include <memory>

// Forward declarations only
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace ANI {
	class EngineContext;
	class StudioContext;
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

		virtual void SetImGuiContext(ImGuiContext* context) {
			// Default implementation does nothing
			// Plugins that need ImGui should override this
		}

		virtual bool HasValidImGuiContext() const {
			return false;
		}

		virtual void SetEngineContext(std::shared_ptr<ANI::EngineContext> context) {
			// Default implementation stores the context
			engineContext = context;
		}

		virtual void SetStudioContext(std::shared_ptr<ANI::StudioContext> context) {
			// Default implementation stores the context
			studioContext = context;
		}

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

		// Protected accessors for derived plugins
		std::shared_ptr<ANI::EngineContext> GetEngineContext() const { return engineContext; }
		std::shared_ptr<ANI::StudioContext> GetStudioContext() const { return studioContext; }

	private:
		std::string name;
		std::string version;
		bool initialized = false;

		// Store contexts for plugin use
		std::shared_ptr<ANI::EngineContext> engineContext;
		std::shared_ptr<ANI::StudioContext> studioContext;
	};

} // namespace Plugins

// Plugin export macros
#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif