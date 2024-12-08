#pragma once
#include <ECS.h>
#include <imgui.h>

using namespace ECS;

class DebugView {
public:
    DebugView(EntityManager &mgr) : manager(mgr) {}
    void Init();
    void Render();
    void RenderEntityPanel();
    void RenderSystemPanel();
    template <typename T>
    void RenderComponentEditor();

private:
    EntityManager &manager;
    std::vector<EntityID> entities;
    EntityID selectedEntity = -1;
    int entityIndex = 0;

    void RefreshEntities();
};
