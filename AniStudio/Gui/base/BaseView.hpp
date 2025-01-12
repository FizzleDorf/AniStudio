#pragma once

#include "ECS.h"
#include "../Events/Events.hpp"
#include <imgui.h>
#include "ViewTypes.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace GUI {
class BaseView {
public:
    std::string viewName = "Base_View";
    BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr) {}
    virtual ~BaseView() {}

    inline const ViewID GetID() const { return viewID; }

    // Core functions that views implement
    virtual void Init() {}
    virtual void Render() = 0;
    virtual void HandleInput(int key, int action) {}

    // Serialize to JSON
    virtual nlohmann::json Serialize() const {
        nlohmann::json j;
        j["viewName"] = viewName;
        return j;
    }

    // Deserialize from JSON
    virtual void Deserialize(const nlohmann::json &j) {
        if (j.contains("viewName"))
            viewName = j["viewName"];
    }

private:
    friend class ViewManager;
    ViewID viewID;
    ECS::EntityManager &mgr;
};
} // namespace GUI