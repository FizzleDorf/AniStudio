#pragma once
#include "../Engine/Engine.hpp"
#include "../backends/imgui_impl_opengl3.h"
#include "ECS.h"
#include "pch.h"

using namespace ECS;

class UpscaleView {
public:
    UpscaleView(FilePaths &paths) : filePaths(paths) {}
    ~UpscaleView() {}

    void Render();

private:
    // ECS-related variables
    EntityManager &mgr = ECS::EntityManager::Ref();
    EntityID entity;

    // saved default filepaths
    FilePaths &filePaths;

    // Variables to handle the parameters for upscaling
    EsrganComponent modelComp;
    InputImageComponent inputImageComp;
    ImageComponent imageComp;
};