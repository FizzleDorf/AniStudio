#include "VulkanContext.hpp"
#define VMA_IMPLEMENTATION
#include <stdexcept>
#include <vk_mem_alloc.h>
#include <iostream>
namespace ANI {

VulkanContext::VulkanContext() {}

VulkanContext::~VulkanContext() { Cleanup(); }

void VulkanContext::Init(GLFWwindow *window) {
    // Create VMA allocator
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    
    CreateInstance();
    std::cout << "VK Instance Created" << std::endl;
    CreateDevice();
    std::cout << "VK Device Created" << std::endl;
    CreateCommandPool();
    std::cout << "VK Command Pool Created" << std::endl;
    CreateSwapchain();
    std::cout << "VK Swapchain Created" << std::endl;
    CreateDescriptorPool();
    std::cout << "VK Descriptor Pool Created" << std::endl;
    SetupImGui(window);
    std::cout << "ImGui Setup Completed" << std::endl;

    

    if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
    std::cout << "VK Context Successful" << std::endl;
}


void VulkanContext::CreateInstance() {
    vk::InstanceCreateInfo createInfo;

#ifdef DEBUG
    const char *validationLayerNames[] = {"VK_LAYER_KHRONOS_validation"};
    createInfo.setPEnabledLayerNames(validationLayerNames);
    createInfo.setEnabledLayerCount(1);
#else
    createInfo.setEnabledLayerCount(0);
#endif

    instance = vk::createInstance(createInfo);
}

void VulkanContext::CreateDevice() {
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        throw std::runtime_error("No physical devices available!");
    }
    physicalDevice = physicalDevices.front();

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo({}, 0, 1, &queuePriority);

    device = physicalDevice.createDevice(vk::DeviceCreateInfo({}, 1, &queueCreateInfo));
    graphicsQueue = device.getQueue(0, 0);
}

void VulkanContext::CreateSwapchain() {
    // Specify swapchain details
    // Ensure swapchain is created and swapchainImageViews are populated here
    // Example:
    vk::SwapchainCreateInfoKHR swapchainInfo = {};
    vk::Extent2D swapchainExtent;

    swapchainInfo.setSurface(surface);
    swapchainInfo.setMinImageCount(2);
    swapchainInfo.setImageFormat(vk::Format::eB8G8R8A8Srgb);
    swapchainInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    swapchainInfo.setImageExtent(swapchainExtent);
    swapchainInfo.setImageArrayLayers(1);
    swapchainInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    swapchainInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
    swapchainInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    swapchainInfo.setPresentMode(vk::PresentModeKHR::eFifo);
    swapchainInfo.setClipped(VK_TRUE);

    swapchain = device.createSwapchainKHR(swapchainInfo);
    std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain);
    for (auto image : swapchainImages) {
        swapchainImageViews.push_back(CreateImageView(image, vk::Format::eB8G8R8A8Srgb));
    }
}


void VulkanContext::SetupImGui(GLFWwindow *window) {
    // Initialize ImGui for Vulkan
    if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
        std::cerr << "Failed to initialize ImGui with GLFW for Vulkan!" << std::endl;
        return;
    }
    std::cout << "ImGui & Glfw Init For Vulkan Successful" << std::endl;

    // prepare init info
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.Queue = graphicsQueue;
    init_info.DescriptorPool = descriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(swapchainImageViews.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;

    if (ImGui_ImplVulkan_Init(&init_info) == false) {
        std::cerr << "Failed to initialize ImGui for Vulkan!" << std::endl;
        return;
    }
    std::cout << "ImGui For Vulkan Initialized" << std::endl;

    // Upload Fonts
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    EndSingleTimeCommands(commandBuffer);
    std::cout << "Font Loaded" << std::endl;
}


void VulkanContext::Cleanup() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    vmaDestroyAllocator(allocator);
    device.destroySwapchainKHR(swapchain);
    device.destroy();
    instance.destroy();
}

void VulkanContext::RenderFrame(ImDrawData *draw_data) {
    if (!draw_data)
        return; 
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
    EndSingleTimeCommands(commandBuffer);
}


void VulkanContext::PresentFrame() {
    // Implement Vulkan frame presentation using vulkan-hpp
}

void VulkanContext::DisplayImage(VkImageView imageView, VkSampler sampler, ImVec2 size) {
    ImGui::Image(reinterpret_cast<ImTextureID>(imageView), size, ImVec2(0, 0), ImVec2(1, 1));
}

vk::CommandBuffer VulkanContext::BeginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo;
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void VulkanContext::EndSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();

    device.freeCommandBuffers(commandPool, commandBuffer);
}

void VulkanContext::CreateDescriptorPool() {
    std::vector<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 1000; // Adjust as needed
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[1].descriptorCount = 1000; // Adjust as needed

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1000; // Maximum number of descriptor sets

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void VulkanContext::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);
    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    commandPool = device.createCommandPool(poolInfo);
}

QueueFamilyIndices VulkanContext::FindQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;

    // Get the list of queue families supported by the physical device
    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }

        // Check if the queue family supports presentation to the window surface
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}


vk::ImageView VulkanContext::CreateImageView(vk::Image image, vk::Format format) {
    vk::ImageViewCreateInfo viewInfo = {};
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.a = vk::ComponentSwizzle::eIdentity;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return device.createImageView(viewInfo);
}


} // namespace ANI
