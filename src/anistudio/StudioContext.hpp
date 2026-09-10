#pragma once
#include "EngineContext.hpp"
#include <memory>

namespace GUI { class ViewManager; }
namespace Plugins { class StudioPluginManager; }
namespace Utils { class WindowState; }
namespace Net { class NetClient; }

namespace ANI {

    class StudioContext : public EngineContext {
    public:
        enum class Mode { Local, Client, Server };

        std::shared_ptr<GUI::ViewManager> viewManager;
        std::shared_ptr<Utils::WindowState> windowState;
        std::shared_ptr<Plugins::StudioPluginManager> studioPluginManager;

        Mode             mode = Mode::Local;
        Net::NetClient* netClient = nullptr;
        bool             networkConnected = false;

        void* windowHandle = nullptr;
        void* imguiContext = nullptr;

        bool showProjectManagerView = true;

        StudioContext() = default;

        bool isValid() const {
            return EngineContext::isValid() && viewManager && windowState;
        }

        bool isLocal()  const { return mode == Mode::Local; }
        bool isClient() const { return mode == Mode::Client; }
        bool isServer() const { return mode == Mode::Server; }
        bool isRemote() const { return mode == Mode::Client; }

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

            studioContext->viewManager = std::make_shared<GUI::ViewManager>();
            studioContext->windowState = std::make_shared<Utils::WindowState>();

            return studioContext;
        }
    };

} // namespace ANI