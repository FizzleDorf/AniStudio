#pragma once
#include "VulkanEngine.h"
#include "ECS.h" // Assuming you have an ECS framework in place

class RenderSystem {
public:
    void init(VulkanEngine* vulkanEngine) {
        this->vulkanEngine = vulkanEngine;
        vulkanEngine->init();
    }

    void update() {
        vulkanEngine->draw();
    }

private:
    VulkanEngine* vulkanEngine;
};
