#pragma once

#include <unordered_map>

#include "GPUAllocator.h"
#include "RingBuffer.h"

static const uint32_t UNLOADED_SBT_OFFSET = 0;

class BLAS {
  public:
    BLAS(std::shared_ptr<GPUAllocator> allocator,
         std::shared_ptr<CommandPool> command_pool,
         std::shared_ptr<RingBuffer> ring_buffer,
         std::vector<VkAabbPositionsKHR> aabbs);
    ~BLAS();

    VkAccelerationStructureKHR &get_blas();

    std::shared_ptr<Semaphore> get_timeline();

    VkDeviceAddress get_device_address();

    static const uint64_t GEOMETRY_BUFFER_TIMELINE = 1;
    static const uint64_t BLAS_BUILD_TIMELINE = 2;

  private:
    VkAccelerationStructureKHR blas_ = VK_NULL_HANDLE;

    std::shared_ptr<GPUBuffer> geometry_buffer_ = nullptr;
    std::shared_ptr<Semaphore> timeline_ = nullptr;
    std::shared_ptr<GPUBuffer> scratch_buffer_ = nullptr;
    std::shared_ptr<GPUBuffer> storage_buffer_ = nullptr;
};

class TLAS {
  public:
    TLAS(std::shared_ptr<GPUAllocator> allocator,
         std::shared_ptr<CommandPool> command_pool,
         std::shared_ptr<RingBuffer> ring_buffer,
         std::vector<std::shared_ptr<BLAS>> bottom_structures,
         std::vector<VkAccelerationStructureInstanceKHR> instances);
    ~TLAS();

    void
    update_model_sbt_offsets(std::unordered_map<uint64_t, uint32_t> models);

    VkAccelerationStructureKHR &get_tlas();

    std::shared_ptr<Semaphore> get_timeline();

    VkDeviceAddress get_device_address();

  private:
    VkAccelerationStructureKHR tlas_ = VK_NULL_HANDLE;

    std::vector<std::shared_ptr<BLAS>> contained_structures_;
    std::vector<VkAccelerationStructureInstanceKHR> instances_;

    std::shared_ptr<GPUBuffer> instances_buffer_ = nullptr;
    std::shared_ptr<Semaphore> timeline_ = nullptr;
    std::shared_ptr<GPUBuffer> scratch_buffer_ = nullptr;
    std::shared_ptr<GPUBuffer> storage_buffer_ = nullptr;

    std::shared_ptr<CommandPool> command_pool_;
    std::shared_ptr<RingBuffer> ring_buffer_;
};
