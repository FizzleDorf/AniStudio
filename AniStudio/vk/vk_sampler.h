#pragma once

#include <vulkan/vulkan.h>

class VkSamplerWrapper {
public:
    VkSamplerWrapper(VkDevice device);
    ~VkSamplerWrapper();

    void CreateSampler();

    // Getters
    VkSampler GetSampler() const { return sampler; }

private:
    VkDevice device;
    VkSampler sampler = VK_NULL_HANDLE;

    void FreeResources();
};
