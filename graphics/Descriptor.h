#pragma once

#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "GPUAllocator.h"

class DescriptorAllocator {
public:
    DescriptorAllocator(std::shared_ptr<Device> device);
    ~DescriptorAllocator();

    VkDescriptorPool grab_pool();
    VkDescriptorPool grab_new_pool();

    VkDescriptorSetLayout grab_layout(const std::vector<VkDescriptorSetLayoutBinding> &bindings, VkDescriptorSetLayoutCreateFlagBits flags);

    void reset_pools();

    std::shared_ptr<Device> get_device();
private:
    std::vector<VkDescriptorPool> used_pools_;
    std::vector<VkDescriptorPool> free_pools_;

    struct LayoutHasher {
	std::size_t operator()(const std::pair<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayoutCreateFlagBits> &k) const{
	    std::size_t result = std::hash<std::size_t>()(k.first.size());
	    for (const auto &b : k.first) {
		std::size_t bits = static_cast<std::size_t>(b.binding) | static_cast<std::size_t>(b.descriptorType) << 16 | static_cast<std::size_t>(b.descriptorCount) << 32 | static_cast<std::size_t>(b.stageFlags) << 48;
		result ^= std::hash<std::size_t>()(bits);
	    }
	    return result;
	}
    };

    std::unordered_map<std::pair<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayoutCreateFlagBits>, VkDescriptorSetLayout, LayoutHasher> layout_cache_;

    std::shared_ptr<Device> device_;
};

class DescriptorSetLayout {
public:
    DescriptorSetLayout(std::shared_ptr<DescriptorAllocator> allocator, VkDescriptorSetLayoutCreateFlagBits flags = {});
    
    void add_binding(VkDescriptorSetLayoutBinding binding);
    VkDescriptorSetLayout get_layout();
private:
    std::shared_ptr<DescriptorAllocator> allocator_;
    VkDescriptorSetLayoutCreateFlagBits flags_;

    std::vector<VkDescriptorSetLayoutBinding> bindings_;
};

class DescriptorSet {
public:
    DescriptorSet(std::shared_ptr<DescriptorAllocator> allocator, std::shared_ptr<DescriptorSetLayout> layout);
private:
    VkDescriptorSet set_;

    std::shared_ptr<DescriptorAllocator> allocator_;
};
