#pragma once

#include "PluginAPI.hpp"
#include "ECS.h"
#include "GUI.h"
#include <string>
#include <memory>
#include <vector>

// Forward declarations
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugin {

	/**
	 * Base class for all AniStudio plugins
	 * Provides the interface that all plugins must implement
	 */
	class BasePlugin {
	public:
		BasePlugin() = default;
		virtual ~BasePlugin() = default;

		// Core plugin lifecycle
		virtual bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager& viewManager) = 0;
		virtual void Shutdown() = 0;
		virtual void Update(float deltaTime) {}

		// Plugin information
		virtual const std::string& GetName() const = 0;
		virtual const std::string& GetVersion() const = 0;
		virtual const std::string& GetDescription() const {
			static std::string empty;
			return empty;
		}

		// Optional overrides for plugin capabilities
		virtual bool HasSettings() const { return false; }
		virtual void ShowSettings() {}

		virtual bool CanReload() const { return true; }
		virtual void OnPreReload() {}
		virtual void OnPostReload() {}

		// Plugin state management
		virtual void SaveState() {}
		virtual void LoadState() {}

		// Plugin dependencies (optional)
		virtual std::vector<std::string> GetDependencies() const {
			return {};
		}

		virtual std::vector<std::string> GetConflicts() const {
			return {};
		}

	protected:
		// Helper for plugins to access common functionality
		bool IsInitialized() const { return initialized; }
		void SetInitialized(bool state) { initialized = state; }

	private:
		bool initialized = false;
	};

} // namespace Plugin

// Required C interface that all plugins must implement
extern "C" {
	// Plugin creation/destruction - REQUIRED
	PLUGIN_API Plugin::BasePlugin* CreatePlugin();
	PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin);

	// Plugin information - REQUIRED
	PLUGIN_API const char* GetPluginName();
	PLUGIN_API const char* GetPluginVersion();

	// Plugin capabilities - OPTIONAL (defaults provided)
	// Don't declare these here to avoid conflicts, let plugins define them if needed
}