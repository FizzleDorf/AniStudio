#pragma once
#include "../Events/Events.hpp"
#include "ECS.h"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>

namespace GUI {
class BaseView {
public:
    std::string viewName = "Base_View";
    BaseView(ECS::EntityManager &entityMgr) : mgr(entityMgr) {}
    virtual ~BaseView() {}

    inline const ViewID GetID() const { return viewID; }

    virtual void Init() {}
    virtual void Render() = 0;
    virtual void HandleInput(int key, int action) {}

    virtual nlohmann::json Serialize() const {
        nlohmann::json j;
        j["viewName"] = viewName;
        return j;
    }

    virtual void Deserialize(const nlohmann::json &j) {
        if (j.contains("viewName"))
            viewName = j["viewName"];
    }

protected:
    ECS::EntityManager &mgr;

private:
    friend class ViewManager;
    ViewID viewID;
};
} // namespace GUI