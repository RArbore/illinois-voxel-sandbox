#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <vulkan/vulkan.h>

#include "GPUAllocator.h"

static const uint32_t MAX_MODELS = 2 << 16;

class DescriptorAllocator {
  public:
    DescriptorAllocator(std::shared_ptr<Device> device);
    ~DescriptorAllocator();

    VkDescriptorPool grab_pool();
    VkDescriptorPool grab_new_pool();

    VkDescriptorSetLayout
    grab_layout(const std::vector<VkDescriptorSetLayoutBinding> &bindings,
                VkDescriptorSetLayoutCreateFlags flags,
                std::unordered_set<uint32_t> bindless_indices = {});

    void reset_pools();

    std::shared_ptr<Device> get_device();

  private:
    std::vector<VkDescriptorPool> used_pools_;
    std::vector<VkDescriptorPool> free_pools_;

    struct LayoutHasher {
        std::size_t
        operator()(const std::pair<std::vector<VkDescriptorSetLayoutBinding>,
                                   VkDescriptorSetLayoutCreateFlags> &k) const {
            std::size_t result = std::hash<std::size_t>()(k.first.size());
            for (const auto &b : k.first) {
                std::size_t bits =
                    static_cast<std::size_t>(b.binding) |
                    static_cast<std::size_t>(b.descriptorType) << 16 |
                    static_cast<std::size_t>(b.descriptorCount) << 32 |
                    static_cast<std::size_t>(b.stageFlags) << 48;
                result ^= std::hash<std::size_t>()(bits);
            }
            return result;
        }
    };

    std::unordered_map<std::pair<std::vector<VkDescriptorSetLayoutBinding>,
                                 VkDescriptorSetLayoutCreateFlags>,
                       VkDescriptorSetLayout, LayoutHasher>
        layout_cache_;

    std::shared_ptr<Device> device_ = nullptr;
};

class DescriptorSetLayout {
  public:
    DescriptorSetLayout(std::shared_ptr<DescriptorAllocator> allocator,
                        VkDescriptorSetLayoutCreateFlags flags = {});

    void add_binding(VkDescriptorSetLayoutBinding binding,
                     bool bindless = false);
    VkDescriptorSetLayout get_layout();

  private:
    std::shared_ptr<DescriptorAllocator> allocator_ = nullptr;
    VkDescriptorSetLayoutCreateFlags flags_ = {};

    std::vector<VkDescriptorSetLayoutBinding> bindings_;
    std::unordered_set<uint32_t> bindless_indices_;
};

class DescriptorSet {
  public:
    DescriptorSet(std::shared_ptr<DescriptorAllocator> allocator,
                  std::shared_ptr<DescriptorSetLayout> layout);

    VkDescriptorSet get_set();
    VkDescriptorSetLayout get_layout();

  private:
    VkDescriptorSet set_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;

    std::shared_ptr<DescriptorAllocator> allocator_ = nullptr;
};

class DescriptorSetBuilder {
  public:
    DescriptorSetBuilder(std::shared_ptr<DescriptorAllocator> allocator,
                         VkDescriptorSetLayoutCreateFlags layout_flags = {});

    DescriptorSetBuilder &bind_buffer(uint32_t binding,
                                      VkDescriptorBufferInfo buffer_info,
                                      VkDescriptorType type,
                                      VkShaderStageFlags stages);
    DescriptorSetBuilder &bind_buffers(
        uint32_t binding,
        std::vector<std::pair<VkDescriptorBufferInfo, uint32_t>> buffer_infos,
        VkDescriptorType type, VkShaderStageFlags stages);
    DescriptorSetBuilder &bind_image(uint32_t binding,
                                     VkDescriptorImageInfo image_info,
                                     VkDescriptorType type,
                                     VkShaderStageFlags stages);
    DescriptorSetBuilder &bind_images(
        uint32_t binding,
        std::vector<std::pair<VkDescriptorImageInfo, uint32_t>> image_infos,
        VkDescriptorType type, VkShaderStageFlags stages);
    DescriptorSetBuilder &
    bind_acceleration_structure(uint32_t binding,
                                VkWriteDescriptorSetAccelerationStructureKHR
                                    acceleration_structure_info,
                                VkShaderStageFlags stages);

    std::shared_ptr<DescriptorSet> build();
    void update(std::shared_ptr<DescriptorSet> set);
    std::shared_ptr<DescriptorSetLayout> get_layout();

  private:
    std::vector<VkWriteDescriptorSet> writes_;

    std::shared_ptr<DescriptorAllocator> allocator_ = nullptr;
    std::shared_ptr<DescriptorSetLayout> layout_ = nullptr;

    std::vector<VkDescriptorBufferInfo> buffer_infos_;
    std::vector<VkDescriptorImageInfo> image_infos_;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR>
        acceleration_structure_infos_;
};
