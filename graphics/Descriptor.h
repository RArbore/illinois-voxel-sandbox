#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "GPUAllocator.h"

class DescriptorAllocator {
public:
    DescriptorAllocator(std::shared_ptr<Device> device);
    ~DescriptorAllocator();

    VkDescriptorPool grab_pool();
    VkDescriptorPool grab_new_pool();

    void reset_pools();

    std::shared_ptr<Device> get_device();
private:
    std::vector<VkDescriptorPool> used_pools_;
    std::vector<VkDescriptorPool> free_pools_;

    std::shared_ptr<Device> device_;
};

class DescriptorSetLayout {
public:
    VkDescriptorSetLayout get_layout();
private:
    VkDescriptorSetLayout layout_;
};

class DescriptorSet {
public:
    DescriptorSet(std::shared_ptr<DescriptorAllocator> allocator, std::shared_ptr<DescriptorSetLayout> layout);
private:
    VkDescriptorSet set_;

    std::shared_ptr<DescriptorAllocator> allocator_;
};
