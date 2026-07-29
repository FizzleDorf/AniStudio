#pragma once
#include "PluginManager.hpp"
#include "ProjectManager.hpp"
#include "WindowState.hpp"
#include <unordered_map>
#include <vector>
#include <string>

struct ImGuiContext;

namespace GUI {
    class ViewManager;
}

namespace ANI {
    class StudioContext;
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

        bool enablePlugin(const std::string& pluginName) override;
        bool disablePlugin(const std::string& pluginName) override;

        void SetStudioContext(std::shared_ptr<ANI::StudioContext> context) {
            studioContext = context;
            std::cout << "[StudioPluginManager] Studio context set" << std::endl;
        }

        std::shared_ptr<ANI::StudioContext> GetStudioContext() const { return studioContext; }

        GUI::ViewManager& GetViewManager() { return viewManager; }
        ECS::EntityManager& GetEntityManager() { return entityManager; }
        ImGuiContext* GetImGuiContext() { return mainImGuiContext; }

    private:
        GUI::ViewManager& viewManager;
        ImGuiContext* mainImGuiContext = nullptr;
        std::shared_ptr<ANI::StudioContext> studioContext;

        std::unordered_map<std::string, std::vector<std::string>> pluginViewNames;
    };

} // namespace Plugins