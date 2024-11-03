#pragma once

#include "ImageComponent.hpp"
#include <imgui.h>
#include <vulkan/vulkan.h>

namespace ECS {
class ImageView {
public:
    ImageView(ECS::ImageComponent *imageComponent);

    void Render(); // Handles the rendering of the image

private:
    ECS::ImageComponent *imageComponent; // Holds the image data (raw image, width, height, etc.)

    // The Vulkan objects for rendering the image (VkDescriptorSet for ImGui::Image)
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    // Helper method to create ImGui-compatible Vulkan descriptor
    void CreateDescriptorSet();
};
} // namespace ECS