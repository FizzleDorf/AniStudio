#pragma once

#include "GUI.h"
#include "FilePaths.hpp"
#include <components.h>
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

    DebugView(ECS::EntityManager &entityMgr) : BaseView(entityMgr) { viewName = "DebugView"; }
    ~DebugView(){}
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