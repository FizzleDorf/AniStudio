#pragma once
#include <memory>

namespace ECS { class EntityManager; }
namespace GUI { class ViewManager; }
namespace Plugins { class PluginManager; }

namespace ANI {

	struct EngineContext {
		std::shared_ptr<ECS::EntityManager> entityManager;
		std::shared_ptr<Plugins::PluginManager> pluginManager;

		// Common engine settings
		std::string pluginDirectory = "../plugins";
		bool hotReloadEnabled = false;

		EngineContext() = default;

		virtual ~EngineContext() = default;

		bool isValid() const {
			return entityManager != nullptr;
		}

		// Helper factory
		static std::shared_ptr<EngineContext> Create() {
			auto context = std::make_shared<EngineContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			return context;
		}
	};

} // namespace ANI