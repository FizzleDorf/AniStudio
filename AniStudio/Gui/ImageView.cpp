#include "ImageView.hpp"
#include <stdexcept>

ECS::ImageView::ImageView(ECS::ImageComponent *imageComponent) : imageComponent(imageComponent) {
    imageComponent->loadImageFromPath("C:\\Users\\julie\\Downloads\\134553466146.png");
    CreateDescriptorSet(); // Generate Vulkan descriptor set for ImGui
}

void ECS::ImageView::CreateDescriptorSet() {
    // This method assumes Vulkan resources such as VkImageView and VkSampler are already created.
    // The descriptor set should be created using the imageComponent's Vulkan resources (imageView, sampler, etc.).
    // You would typically populate a `VkDescriptorSet` here.

    if (!imageComponent->imageData) {
        throw std::runtime_error("Image data is missing in ImageComponent!");
    }

    // Create Vulkan descriptor set for the image (using Vulkan objects from ImageComponent)
    // -- Placeholder for Vulkan descriptor set creation code --
    // (Make sure you have VkImageView, VkSampler, and image resources in ImageComponent)

    // Assign descriptorSet based on the created Vulkan descriptor
    // descriptorSet = ...;
}

void ECS::ImageView::Render() {
    if (imageComponent && imageComponent->imageData) {
        ImGui::Begin("Image Viewer");

        // Display the image using ImGui's Vulkan binding
        if (descriptorSet != VK_NULL_HANDLE) {
            // Set the size of the image as per the loaded image
            ImVec2 imageSize(static_cast<float>(imageComponent->width), static_cast<float>(imageComponent->height));

            // Use ImGui::Image to display the Vulkan image
            ImGui::Image((void *)(intptr_t)descriptorSet, imageSize);
        } else {
            ImGui::Text("Descriptor set is not ready for rendering.");
        }

        ImGui::End();
    } else {
        ImGui::Text("No image loaded to render.");
    }
}
