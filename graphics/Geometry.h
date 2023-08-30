#pragma once

#include "GPUAllocator.h"
#include "RingBuffer.h"

class AccelerationStructureBuilder {
public:
    AccelerationStructureBuilder(std::shared_ptr<GPUAllocator> allocator, std::shared_ptr<RingBuffer> ring_buffer);
private:
    std::shared_ptr<GPUBuffer> cube_buffer_ = nullptr;
    std::shared_ptr<Semaphore> cube_buffer_timeline_ = nullptr;

    std::shared_ptr<Device> device_ = nullptr;
};
