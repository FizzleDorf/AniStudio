#pragma once

#include "ExampleComponent.hpp"
#include "ECS.h"
#include "GUI.h"
#include <imgui.h>

namespace GUI {

class ANI_API ExampleView : public BaseView {
    
    static const ViewTypeID viewTypeID;

public:
    explicit ExampleView(ECS::EntityManager &entityMgr);
    virtual ~ExampleView() override;
    void Render() override;
};

// const ViewTypeID ExampleView::viewTypeID = GetRuntimeViewTypeID();

} // namespace GUI