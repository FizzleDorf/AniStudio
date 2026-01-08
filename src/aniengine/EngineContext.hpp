#pragma once
#include <memory>
#include "FilePaths.hpp"
#include "PluginManager.hpp"

namespace ECS { class EntityManager; }
namespace GUI { class ViewManager; }

namespace ANI {

	struct EngineContext {
		std::shared_ptr<ECS::EntityManager> entityManager;
		std::shared_ptr<Plugins::PluginManager> pluginManager;
		std::shared_ptr<Utils::FilePaths> filePaths;

		std::string pluginDirectory = "../plugins";
		bool hotReloadEnabled = false;

		EngineContext() = default;

		virtual ~EngineContext() = default;

		bool isValid() const {
			return entityManager != nullptr && filePaths != nullptr;
		}

		static std::shared_ptr<EngineContext> Create() {
			auto context = std::make_shared<EngineContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			context->filePaths = std::make_shared<Utils::FilePaths>();
			context->pluginManager = std::make_shared<Plugins::PluginManager>(*context->entityManager);

			context->filePaths->Init();

			return context;
		}

		static std::shared_ptr<EngineContext> CreateWithFilePaths(std::shared_ptr<Utils::FilePaths> filePaths) {
			if (!filePaths) {
				return nullptr;
			}

			auto context = std::make_shared<EngineContext>();
			context->entityManager = std::make_shared<ECS::EntityManager>();
			context->filePaths = filePaths;
			context->pluginManager = std::make_shared<Plugins::PluginManager>(*context->entityManager);

			if (!context->filePaths->IsInitialized()) {
				context->filePaths->Init();
			}

			return context;
		}
	};

} // namespace ANI