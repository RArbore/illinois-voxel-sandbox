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
    VkFence fence_;

    std::shared_ptr<Device> device_;
};

class Semaphore {
public:
    Semaphore(std::shared_ptr<Device> device);
    ~Semaphore();

    VkSemaphore get_semaphore();
private:
    VkSemaphore semaphore_;

    std::shared_ptr<Device> device_;
};

class Barrier {
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses);
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUBuffer> buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUImage> image, VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS});
    Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUVolume> volume, VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS});

    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses);
    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUBuffer> buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUImage> image, VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS});
    void add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUVolume> volume, VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS});
private:
    std::vector<VkMemoryBarrier2> global_barriers_;
    std::vector<VkBufferMemoryBarrier2> buffer_barriers_;
    std::vector<VkImageMemoryBarrier2> image_barriers_;

    std::shared_ptr<Device> device_;
    std::vector<std::shared_ptr<GPUBuffer>> referenced_buffers_;
    std::vector<std::shared_ptr<GPUImage>> referenced_images_;
    std::vector<std::shared_ptr<GPUVolume>> referenced_volumes_;
};
