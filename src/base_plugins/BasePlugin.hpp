//============================================================================
// BasePlugin.hpp - Base Plugin Interface - CRASH FIXED
//============================================================================

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

		// Plugin dependencies (optional) - these should match the manifest
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

		// Protected access to managers (set during initialization)
		ECS::EntityManager* GetEntityManager() const {
			// Return stored pointer - this should be set by SetManagers before Initialize
			return entityManager;
		}

		GUI::ViewManager* GetViewManager() const {
			// Return stored pointer - this should be set by SetManagers before Initialize
			return viewManager;
		}

	public:
		// CRITICAL: This method MUST be called by PluginManager BEFORE Initialize
		// It sets up the manager pointers that the plugin will use
		void SetManagers(ECS::EntityManager* eMgr, GUI::ViewManager* vMgr) {
			if (!eMgr || !vMgr) {
				std::cerr << "BasePlugin::SetManagers - NULL POINTERS PASSED!" << std::endl;
				std::cerr << "EntityManager: " << eMgr << ", ViewManager: " << vMgr << std::endl;
				return;
			}

			entityManager = eMgr;
			viewManager = vMgr;

			std::cout << "BasePlugin::SetManagers - Managers set successfully" << std::endl;
			std::cout << "EntityManager: " << entityManager << ", ViewManager: " << viewManager << std::endl;
		}

	private:
		bool initialized = false;
		ECS::EntityManager* entityManager = nullptr;
		GUI::ViewManager* viewManager = nullptr;
	};

} // namespace Plugin