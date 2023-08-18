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
