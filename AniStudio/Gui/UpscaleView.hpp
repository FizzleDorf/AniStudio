#pragma once
#include "Base/BaseView.hpp"
#include "pch.h"

namespace GUI {

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
} // namespace GUI