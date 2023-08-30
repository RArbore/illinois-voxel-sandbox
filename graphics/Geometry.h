#pragma once

#include "GPUAllocator.h"
#include "RingBuffer.h"

class BLAS {
public:
    BLAS(std::shared_ptr<GPUAllocator> allocator, std::shared_ptr<CommandPool> command_pool, std::shared_ptr<RingBuffer> ring_buffer, std::vector<VkAabbPositionsKHR> chunks);

    VkAccelerationStructureKHR get_blas();

    std::shared_ptr<Semaphore> get_timeline();
private:
    VkAccelerationStructureKHR blas_ = VK_NULL_HANDLE;
    
    std::shared_ptr<GPUBuffer> cube_buffer_ = nullptr;
    std::shared_ptr<Semaphore> timeline_ = nullptr;
    std::shared_ptr<GPUBuffer> scratch_buffer_ = nullptr;
    std::shared_ptr<GPUBuffer> storage_buffer_ = nullptr;
};
