#pragma once
#include "BaseView.hpp"
#include <ECS.h>
#include <imgui.h>

using namespace ECS;

class DebugView : public BaseView {
public:
    DebugView(){}
    void Init();
    void Render();
    void RenderEntityPanel();
    void RenderSystemPanel();
    template <typename T>
    void RenderComponentEditor();

private:
    std::vector<EntityID> entities;
    EntityID selectedEntity = -1;
    int entityIndex = 0;

    void RefreshEntities();
};
