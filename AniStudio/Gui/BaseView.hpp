#ifndef BASE_VIEW_HPP
#define BASE_VIEW_HPP

#include "../Engine/Engine.hpp"
#include "../Events/Events.hpp"
#include "filepaths.hpp"

class BaseView {
public:
    void Init(
        ECS::EntityManager &entityManager, 
        FilePaths &paths) 
    {
        mgr = entityManager;
        filePaths = paths;
    }

    virtual void Update(float deltaT) {}
    virtual void Render() = 0;

protected:
    ECS::EntityManager &mgr;
    FilePaths &filePaths;
};

#endif
