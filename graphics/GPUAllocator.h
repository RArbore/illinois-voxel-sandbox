#pragma once

#include <vulkan/vulkan.h>

#include "external/VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include "Device.h"

class GPUBuffer;
class GPUImage;
class GPUVolume;

class GPUAllocator {
public:
    GPUAllocator(std::shared_ptr<Device> device);
    ~GPUAllocator();

    VmaAllocator get_vma();
    std::shared_ptr<Device> get_device();
private:
    VmaAllocator allocator_;

    std::shared_ptr<Device> device_ = nullptr;
};

class GPUBuffer {
public:
    GPUBuffer(std::shared_ptr<GPUAllocator> allocator, VkDeviceSize size, VkDeviceSize alignment, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags);
    ~GPUBuffer();

    VkDeviceAddress get_device_address();
private:
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkDeviceSize size_;
    VkBufferUsageFlags usage_;
    VkMemoryPropertyFlags memory_flags_;
    VmaAllocationCreateFlags vma_flags_;

    std::shared_ptr<GPUAllocator> allocator_ = nullptr;
};

class GPUImage {
public:
    GPUImage(std::shared_ptr<GPUAllocator> allocator, VkExtent2D extent, VkFormat format, VkImageCreateFlags create_flags, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags, uint32_t mip_levels, uint32_t array_layers);
    ~GPUImage();
private:
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkExtent2D extent_;
    VkFormat format_;
    VkImageCreateFlags create_flags_;
    VkImageUsageFlags usage_flags_;
    VkMemoryPropertyFlags memory_flags_;
    VmaAllocationCreateFlags vma_flags_;
    uint32_t mip_levels_;
    uint32_t array_layers_;
    VkImageLayout last_set_layout_;

    std::shared_ptr<GPUAllocator> allocator_;
};

class GPUVolume {
public:
    GPUVolume(std::shared_ptr<GPUAllocator> allocator, VkExtent3D extent, VkFormat format, VkImageCreateFlags create_flags, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags, uint32_t mip_levels, uint32_t array_layers);
    ~GPUVolume();
private:
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkExtent3D extent_;
    VkFormat format_;
    VkImageCreateFlags create_flags_;
    VkImageUsageFlags usage_flags_;
    VkMemoryPropertyFlags memory_flags_;
    VmaAllocationCreateFlags vma_flags_;
    uint32_t mip_levels_;
    uint32_t array_layers_;
    VkImageLayout last_set_layout_;

    std::shared_ptr<GPUAllocator> allocator_;
};
