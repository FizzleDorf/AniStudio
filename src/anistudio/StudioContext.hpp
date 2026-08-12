#pragma once
#include "EngineContext.hpp"
#include <memory>

namespace GUI { class ViewManager; }
namespace Plugins { class StudioPluginManager; }
namespace Utils { class WindowState; }

namespace ANI {

	class StudioContext : public EngineContext {
	public:
		std::shared_ptr<GUI::ViewManager> viewManager;
		std::shared_ptr<Utils::WindowState> windowState;
		std::shared_ptr<Plugins::StudioPluginManager> studioPluginManager;

		// Window/UI settings
		void* windowHandle = nullptr;
		void* imguiContext = nullptr;

		// UI state
		bool showProjectManagerView = true;

		StudioContext() = default;

		bool isValid() const {
			return EngineContext::isValid() &&
				viewManager &&
				windowState;
		}

		// Helper factory
		static std::shared_ptr<StudioContext> Create() {
			auto context = std::make_shared<StudioContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			context->viewManager = std::make_shared<GUI::ViewManager>();
			context->windowState = std::make_shared<Utils::WindowState>();
			return context;
		}

		static std::shared_ptr<StudioContext> FromEngine(
			std::shared_ptr<EngineContext> engineContext) {

			if (!engineContext) return nullptr;

			auto studioContext = std::make_shared<StudioContext>();

			studioContext->entityManager = engineContext->entityManager;
			studioContext->pluginManager = engineContext->pluginManager;
			studioContext->pluginDirectory = engineContext->pluginDirectory;
			studioContext->hotReloadEnabled = engineContext->hotReloadEnabled;

			// Initialize studio-specific members
			studioContext->viewManager = std::make_shared<GUI::ViewManager>();
			studioContext->windowState = std::make_shared<Utils::WindowState>();

			return studioContext;
		}
	};

} // namespace ANI