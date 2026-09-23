#pragma once

#include "GUI.h"
#include "AniStudioComponents.hpp"
#include "AniEngineComponents.hpp"
#include <AniEngineSystems.hpp>

using namespace ECS;

namespace Net { class NetClient; }

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

        void SetNetClient(Net::NetClient* client) { m_netClient = client; }

    private:
        std::vector<EntityID> entities;
        EntityID selectedEntity = -1;
        int entityIndex = 0;

        Net::NetClient* m_netClient = nullptr;

        void RefreshEntities();
        void RenderClientPanel();
    };

} // namespace GUI