#pragma once
#include "ECS.h"
#include "../Events/Events.hpp"
#include <imgui.h>

class BaseView {
public:
    explicit BaseView() = default;
    virtual ~BaseView() = default;

    virtual void Update(float deltaTime) {}
    virtual void Render() = 0;
    virtual void HandleInput(int key, int action) {}
};
