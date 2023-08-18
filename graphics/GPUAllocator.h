#include <vulkan/vulkan.h>

#include "external/VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include "Device.h"

class GPUAllocator {
public:
    GPUAllocator(std::shared_ptr<Device> device);
    ~GPUAllocator();

private:
    VmaAllocator allocator_;

    std::shared_ptr<Device> device_;
};
