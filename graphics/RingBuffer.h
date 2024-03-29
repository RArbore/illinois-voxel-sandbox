#pragma once

#include <deque>

#include "Command.h"
#include "GPUAllocator.h"
#include "Synchronization.h"

class RingBuffer {
  public:
    RingBuffer(std::shared_ptr<GPUAllocator> allocator,
               std::shared_ptr<CommandPool> command_pool, VkDeviceSize size);
    ~RingBuffer();

    void
    copy_to_device(std::shared_ptr<GPUBuffer> dst, VkDeviceSize dst_offset,
                   std::span<const std::byte> src,
                   std::vector<std::shared_ptr<Semaphore>> wait_semaphores,
                   std::vector<std::shared_ptr<Semaphore>> signal_semaphores);

    void
    copy_to_device(std::shared_ptr<GPUImage> dst, VkImageLayout dst_layout,
                   std::span<const std::byte> src,
                   std::vector<std::shared_ptr<Semaphore>> wait_semaphores,
                   std::vector<std::shared_ptr<Semaphore>> signal_semaphores);

    void
    copy_to_device(std::shared_ptr<GPUVolume> dst, VkImageLayout dst_layout,
                   std::span<const std::byte> src,
                   std::vector<std::shared_ptr<Semaphore>> wait_semaphores,
                   std::vector<std::shared_ptr<Semaphore>> signal_semaphores);

    VkDeviceSize get_size();
    void reap_in_flight_copies();

  private:
    struct InFlightCopy {
        std::shared_ptr<Command> command_ = nullptr;
        std::shared_ptr<Fence> fence_ = nullptr;
        VkDeviceSize virtual_counter_;
        VkDeviceSize size_;
        std::shared_ptr<GPUBuffer> gpu_buffer_ = nullptr;
        std::shared_ptr<GPUImage> gpu_image_ = nullptr;
        std::shared_ptr<GPUVolume> gpu_volume_ = nullptr;
    };
    VkDeviceSize virtual_counter_ = 0;

    std::deque<InFlightCopy> in_flight_copies_;
    std::vector<std::pair<std::shared_ptr<Command>, std::shared_ptr<Fence>>>
        free_copy_resources_;
    std::span<std::byte> buffer_map_;

    std::shared_ptr<Device> device_ = nullptr;
    std::shared_ptr<CommandPool> command_pool_ = nullptr;
    std::shared_ptr<GPUBuffer> buffer_ = nullptr;
};
