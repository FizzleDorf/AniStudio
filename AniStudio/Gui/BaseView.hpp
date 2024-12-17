#pragma once
#include "ECS.h"
#include "../Events/Events.hpp"
namespace ANI {
class Events;
}
namespace ECS {
class EntityManager;
}
class BaseView {
public:
    explicit BaseView() = default;
    virtual ~BaseView() = default;

    virtual void Update(float deltaTime) {}
    virtual void Render() = 0;
    virtual void HandleInput(int key, int action) {}
};
