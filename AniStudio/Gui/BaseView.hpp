#pragma once
#include "ECS.h"

class BaseView {
public:
    explicit BaseView(ECS::EntityManager &entityManager) : mgr(entityManager) {}
    virtual ~BaseView() = default;

    virtual void Update(float deltaTime) {}
    virtual void Render() = 0;
    virtual void HandleInput(int key, int action) {}

    ECS::EntityManager &GetEntityManager() { return mgr; }

protected:
    ECS::EntityManager &mgr;
};
