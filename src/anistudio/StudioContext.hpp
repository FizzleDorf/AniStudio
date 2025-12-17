#pragma once
#include "EngineContext.hpp"
#include <memory>

namespace GUI { class ViewManager; }
namespace ANI {
	class ProjectManager;
	class WindowState;
	class StudioPluginManager;
}

namespace ANI {

	struct StudioContext : public EngineContext {
		std::shared_ptr<GUI::ViewManager> viewManager;
		std::shared_ptr<ProjectManager> projectManager;
		std::shared_ptr<WindowState> windowState;
		std::shared_ptr<StudioPluginManager> studioPluginManager;

		// Window/UI settings
		void* windowHandle = nullptr;
		void* imguiContext = nullptr;

		// UI state
		bool showProjectManagerView = true;

		StudioContext() = default;

		bool isValid() const {
			return EngineContext::isValid() &&
				viewManager &&
				projectManager &&
				windowState;
		}

		// Helper factory
		static std::shared_ptr<StudioContext> Create() {
			auto context = std::make_shared<StudioContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			context->viewManager = std::make_shared<GUI::ViewManager>();
			context->windowState = std::make_shared<WindowState>();
			context->projectManager = std::make_shared<ProjectManager>(
				*context->viewManager,
				*context->entityManager
				);
			return context;
		}

		// Convert from base context
		static std::shared_ptr<StudioContext> FromEngine(
			std::shared_ptr<EngineContext> engineContext) {

			if (!engineContext) return nullptr;

			auto studioContext = std::make_shared<StudioContext>();
			// Copy base context members
			studioContext->entityManager = engineContext->entityManager;
			studioContext->pluginManager = engineContext->pluginManager;
			studioContext->pluginDirectory = engineContext->pluginDirectory;
			studioContext->hotReloadEnabled = engineContext->hotReloadEnabled;

			// Initialize studio-specific members
			studioContext->viewManager = std::make_shared<GUI::ViewManager>();
			studioContext->windowState = std::make_shared<WindowState>();
			studioContext->projectManager = std::make_shared<ProjectManager>(
				*studioContext->viewManager,
				*studioContext->entityManager
				);

			return studioContext;
		}
	};

} // namespace ANI