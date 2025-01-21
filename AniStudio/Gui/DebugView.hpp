#pragma once
#include "Base/BaseView.hpp"
#include <components.h>
#include <systems.h>

using namespace ECS;

namespace GUI {

class DebugView : public BaseView {
public:
    DebugView(ECS::EntityManager &entityMgr) : BaseView(entityMgr) { viewName = "DebugView"; }
    void Init();
    void Render();
    void RenderEntityPanel();
    void RenderSystemPanel();
    
    // template <typename T>
    // void RenderComponentEditor();

private:
    std::vector<EntityID> entities;
    EntityID selectedEntity = -1;
    int entityIndex = 0;

    void RefreshEntities();
};
} // namespace UI