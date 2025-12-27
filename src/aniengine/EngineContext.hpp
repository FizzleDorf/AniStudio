#pragma once
#include <memory>
#include "FilePaths.hpp"

namespace ECS { class EntityManager; }
namespace GUI { class ViewManager; }
namespace Plugins { class PluginManager; }

namespace ANI {

	struct EngineContext {
		std::shared_ptr<ECS::EntityManager> entityManager;
		std::shared_ptr<Plugins::PluginManager> pluginManager;
		std::shared_ptr<Utils::FilePaths> filePaths;

		// Common engine settings
		std::string pluginDirectory = "../plugins";
		bool hotReloadEnabled = false;

		EngineContext() = default;

		virtual ~EngineContext() = default;

		bool isValid() const {
			return entityManager != nullptr && filePaths != nullptr;
		}

		// Helper factory
		static std::shared_ptr<EngineContext> Create() {
			auto context = std::make_shared<EngineContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			context->filePaths = std::make_shared<Utils::FilePaths>();

			// Initialize FilePaths
			context->filePaths->Init();

			return context;
		}

		// Factory with custom FilePaths
		static std::shared_ptr<EngineContext> CreateWithFilePaths(std::shared_ptr<Utils::FilePaths> filePaths) {
			if (!filePaths) {
				return nullptr;
			}

			auto context = std::make_shared<EngineContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			context->filePaths = filePaths;

			// Initialize FilePaths if not already
			if (!context->filePaths->IsInitialized()) {
				context->filePaths->Init();
			}

			return context;
		}
	};

} // namespace ANI