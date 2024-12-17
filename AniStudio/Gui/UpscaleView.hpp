#pragma once
#include "../backends/imgui_impl_opengl3.h"
#include "ECS.h"
#include "pch.h"

using namespace ECS;

class UpscaleView {
public:
    UpscaleView() {}
    ~UpscaleView() {}

    void Render();

private:
    // ECS-related variables
    EntityID entity;

    // Variables to handle the parameters for upscaling
    EsrganComponent modelComp;
    InputImageComponent inputImageComp;
    ImageComponent imageComp;
};