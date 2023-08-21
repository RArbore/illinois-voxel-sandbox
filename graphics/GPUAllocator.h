#include <vulkan/vulkan.h>

#include "external/VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include "Device.h"

class GPUBuffer;

class GPUAllocator {
public:
    GPUAllocator(std::shared_ptr<Device> device);
    ~GPUAllocator();

    VmaAllocator get_vma();
private:
    VmaAllocator allocator_;

    std::shared_ptr<Device> device_ = nullptr;
};

class GPUBuffer {
public:
    GPUBuffer(std::shared_ptr<GPUAllocator> allocator, VkDeviceSize size, VkDeviceSize alignment, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags);
    ~GPUBuffer();
private:
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    VkDeviceSize size_;
    VkBufferUsageFlags usage_;
    VkMemoryPropertyFlags memory_flags_;
    VmaAllocationCreateFlags vma_flags_;

    std::shared_ptr<GPUAllocator> allocator_ = nullptr;

    friend class GPUAllocator;
};
