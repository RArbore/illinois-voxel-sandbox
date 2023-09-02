#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "GPUAllocator.h"

class Fence {
  public:
    Fence(std::shared_ptr<Device> device, bool create_signaled = true);
    ~Fence();

    void wait();
    void reset();

    VkFence get_fence();

  private:
    VkFence fence_ = VK_NULL_HANDLE;

    std::shared_ptr<Device> device_ = nullptr;
};

class Semaphore {
  public:
    Semaphore(std::shared_ptr<Device> device, bool timeline = false);
    ~Semaphore();

    VkSemaphore get_semaphore();

    bool is_timeline();

    uint64_t get_wait_value();
    uint64_t get_signal_value();

    void set_wait_value(uint64_t wait_value);
    void set_signal_value(uint64_t signal_value);
    void increment();

  private:
    VkSemaphore semaphore_ = VK_NULL_HANDLE;
    bool timeline_;
    uint64_t wait_value_;
    uint64_t signal_value_;

    std::shared_ptr<Device> device_ = nullptr;
};

class Barrier {
  public:
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages,
            VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages,
            VkAccessFlags2 dst_accesses);
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages,
            VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages,
            VkAccessFlags2 dst_accesses, VkBuffer buffer,
            VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages,
            VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages,
            VkAccessFlags2 dst_accesses, VkImage image,
            VkImageLayout old_layout, VkImageLayout new_layout,
            VkImageSubresourceRange range = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS});

    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses,
             VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses);
    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses,
             VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses,
             VkBuffer buffer, VkDeviceSize offset = 0,
             VkDeviceSize size = VK_WHOLE_SIZE);
    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses,
             VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses,
             VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
             VkImageSubresourceRange range = {
                 .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                 .baseMipLevel = 0,
                 .levelCount = VK_REMAINING_MIP_LEVELS,
                 .baseArrayLayer = 0,
                 .layerCount = VK_REMAINING_ARRAY_LAYERS});

    void record(VkCommandBuffer command);

  private:
    std::vector<VkMemoryBarrier2> global_barriers_;
    std::vector<VkBufferMemoryBarrier2> buffer_barriers_;
    std::vector<VkImageMemoryBarrier2> image_barriers_;

    std::shared_ptr<Device> device_ = nullptr;
};
