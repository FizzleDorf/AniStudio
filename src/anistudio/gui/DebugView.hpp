#pragma once

#include "GUI.h"
#include "Components.h"
#include <systems.h>

using namespace ECS;

namespace GUI {

    class DebugView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Debug View",
            "category": "Debug",
            "description": "A simple debugger."
        })";
        }

        DebugView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm) { 
            viewName = "DebugView"; 
        }
        ~DebugView() = default;
        void Init();
        void Render();
        void RenderEntityPanel();
        void RenderSystemPanel();

    private:
        std::vector<EntityID> entities;
        EntityID selectedEntity = -1;
        int entityIndex = 0;

        void RefreshEntities();
    };
} // namespace GUI