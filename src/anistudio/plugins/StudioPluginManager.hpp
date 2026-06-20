#pragma once
#include "PluginManager.hpp"
#include "ProjectManager.hpp"
#include "WindowState.hpp"
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations
struct ImGuiContext;

namespace GUI {
	struct ViewManager;
}

namespace ANI {
	struct StudioContext;
}

namespace Plugins {

	class StudioPluginManager : public PluginManager {
	public:
		StudioPluginManager(
			ECS::EntityManager& entityMgr,
			GUI::ViewManager& viewMgr,
			ImGuiContext* mainContext = nullptr
		);
		~StudioPluginManager() = default;

		// Override to provide studio-specific initialization
		bool enablePlugin(const std::string& pluginName) override;

		// Set studio context
		void SetStudioContext(std::shared_ptr<ANI::StudioContext> context) {
			studioContext = context;
			std::cout << "[StudioPluginManager] Studio context set" << std::endl;
		}

		std::shared_ptr<ANI::StudioContext> GetStudioContext() const { return studioContext; }

		// Public accessors for plugins (if needed, but plugins get direct access in init)
		GUI::ViewManager& GetViewManager() { return viewManager; }
		ECS::EntityManager& GetEntityManager() { return entityManager; }
		ImGuiContext* GetImGuiContext() { return mainImGuiContext; }

	protected:
		// Override view cleanup
		void cleanupPluginViews(const std::string& pluginName) override;

	private:
		// References to managers
		GUI::ViewManager& viewManager;
		ImGuiContext* mainImGuiContext = nullptr;

		// Track plugin views for cleanup
		// We track view names because plugins register them directly with ViewManager
		std::unordered_map<std::string, std::vector<std::string>> pluginViewNames;

		// Store studio context
		std::shared_ptr<ANI::StudioContext> studioContext;
	};

} // namespace Plugins