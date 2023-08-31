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

    std::shared_ptr<Device> device_ = nullptr;
};

class DescriptorSetLayout {
public:
    DescriptorSetLayout(std::shared_ptr<DescriptorAllocator> allocator, VkDescriptorSetLayoutCreateFlagBits flags = {});
    
    void add_binding(VkDescriptorSetLayoutBinding binding);
    VkDescriptorSetLayout get_layout();
private:
    std::shared_ptr<DescriptorAllocator> allocator_ = nullptr;
    VkDescriptorSetLayoutCreateFlagBits flags_ = {};

    std::vector<VkDescriptorSetLayoutBinding> bindings_;
};

class DescriptorSet {
public:
    DescriptorSet(std::shared_ptr<DescriptorAllocator> allocator, std::shared_ptr<DescriptorSetLayout> layout);

    VkDescriptorSet get_set();
    VkDescriptorSetLayout get_layout();
private:
    VkDescriptorSet set_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;

    std::shared_ptr<DescriptorAllocator> allocator_ = nullptr;
};

class DescriptorSetBuilder {
public:
    DescriptorSetBuilder(std::shared_ptr<DescriptorAllocator> allocator, VkDescriptorSetLayoutCreateFlagBits layout_flags = {});

    DescriptorSetBuilder &bind_buffer(uint32_t binding, VkDescriptorBufferInfo buffer_info, VkDescriptorType type, VkShaderStageFlagBits stages);
    DescriptorSetBuilder &bind_image(uint32_t binding, VkDescriptorImageInfo image_info, VkDescriptorType type, VkShaderStageFlagBits stages);
    DescriptorSetBuilder &bind_acceleration_structure(uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR acceleration_structure_info, VkShaderStageFlagBits stages);

    std::shared_ptr<DescriptorSet> build();
    void update(std::shared_ptr<DescriptorSet> set);
private:
    std::vector<VkWriteDescriptorSet> writes_;

    std::shared_ptr<DescriptorAllocator> allocator_ = nullptr;
    std::shared_ptr<DescriptorSetLayout> layout_ = nullptr;

    std::vector<VkDescriptorBufferInfo> buffer_infos_;
    std::vector<VkDescriptorImageInfo> image_infos_;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR > acceleration_structure_infos_;
};
