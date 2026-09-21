#pragma once

#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include "StudioContext.hpp"
#include "StudioPluginManager.hpp"

namespace ANI::Registration {

    // Registers all core ECS components used by the studio UI.
    void RegisterComponents(ECS::EntityManager& entityMgr);

    // Registers all core ECS systems and wires up their dependencies.
    void RegisterSystems(ECS::EntityManager& entityMgr,
        GUI::ViewManager* viewManager,
        void* windowHandle,
        StudioContext* studioContext);

    // Registers all core view types with the ViewManager.
    void RegisterViews(ECS::EntityManager& entityMgr,
        GUI::ViewManager& viewManager,
        Plugins::StudioPluginManager* pluginManager);

} // namespace ANI::Registration