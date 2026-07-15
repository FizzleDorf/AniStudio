#pragma once

#include <memory>
#include "PluginManager.hpp"

namespace ECS { class EntityManager; }
namespace GUI { class ViewManager; }

namespace ANI {

    class EngineContext {
    public:
        std::shared_ptr<ECS::EntityManager> entityManager;
        std::shared_ptr<Plugins::PluginManager> pluginManager;

        std::string pluginDirectory = "../plugins";
        bool hotReloadEnabled = false;

        EngineContext() = default;

        virtual ~EngineContext() = default;

        bool isValid() const {
            return entityManager != nullptr;
        }

        static std::shared_ptr<EngineContext> Create() {
            auto context = std::make_shared<EngineContext>();
            context->entityManager = std::make_shared<ECS::EntityManager>();
            context->pluginManager = std::make_shared<Plugins::PluginManager>(*context->entityManager);
            return context;
        }
    };

} // namespace ANI