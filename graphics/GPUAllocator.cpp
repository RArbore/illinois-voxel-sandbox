#include "utils/Assert.h"
#include "GPUAllocator.h"

GPUAllocator::GPUAllocator(std::shared_ptr<Device> device): device_(device) {
    VmaAllocatorCreateInfo create_info {};
    create_info.instance = device->get_instance();
    create_info.physicalDevice = device->get_physical_device();
    create_info.device = device->get_device();
    create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    ASSERT(vmaCreateAllocator(&create_info, &allocator_), "Couldn't create allocator.");
}

GPUAllocator::~GPUAllocator() {
    vmaDestroyAllocator(allocator_);
}

VmaAllocator GPUAllocator::get_vma() {
    return allocator_;
}

GPUBuffer::GPUBuffer(std::shared_ptr<GPUAllocator> allocator, VkDeviceSize size, VkDeviceSize alignment, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags) {
    VkBufferCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info {};
    alloc_info.flags = vma_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.requiredFlags = memory_flags;

    allocator_ = allocator;
    if (size > 0) {
	ASSERT(vmaCreateBufferWithAlignment(allocator_->get_vma(), &create_info, &alloc_info, alignment, &buffer_, &allocation_, nullptr), "Unable to create buffer.");
    }

    usage_ = usage;
    memory_flags_ = memory_flags;
    vma_flags_ = vma_flags;
}

GPUBuffer::~GPUBuffer() {
    vmaDestroyBuffer(allocator_->get_vma(), buffer_, allocation_);
}
