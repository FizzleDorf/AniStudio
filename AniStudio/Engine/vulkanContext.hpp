#ifndef VULKAN_CONTEXT_HPP
#define VULKAN_CONTEXT_HPP

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vk_mem_alloc.h>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace ANI {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    void Init(GLFWwindow *window);
    void Cleanup();
    void RenderFrame(ImDrawData *draw_data);
    void PresentFrame();
    void DisplayImage(VkImageView imageView, VkSampler sampler, ImVec2 size);

private:
    void CreateInstance();
    void CreateDevice();
    void CreateSwapchain();
    void CreateDescriptorPool();
    void CreateCommandPool();
    void SetupImGui(GLFWwindow *window);

    vk::CommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);
    vk::ImageView CreateImageView(vk::Image image, vk::Format format);
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device);

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::CommandPool commandPool;
    vk::SwapchainKHR swapchain;
    VmaAllocator allocator;
    vk::DescriptorPool descriptorPool;

    ImGui_ImplVulkanH_Window mainWindowData; // Declare this here

    std::vector<vk::ImageView> swapchainImageViews;
};

} // namespace ANI

#endif // VULKAN_CONTEXT_HPP
