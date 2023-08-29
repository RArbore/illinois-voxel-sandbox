#include "utils/Assert.h"
#include "RingBuffer.h"

RingBuffer::RingBuffer(std::shared_ptr<GPUAllocator> allocator, std::shared_ptr<CommandPool> command_pool, VkDeviceSize size) {
    device_ = allocator->get_device();
    command_pool_ = command_pool;
    buffer_ = std::make_shared<GPUBuffer>(allocator, size, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    buffer_map_ = buffer_->cpu_map();
}

RingBuffer::~RingBuffer() {
    buffer_->cpu_unmap();
}

void RingBuffer::copy_to_device(std::shared_ptr<GPUBuffer> dst,
				std::span<std::byte> src,
				std::vector<std::shared_ptr<Semaphore>> wait_semaphores,
				std::vector<std::shared_ptr<Semaphore>> signal_semaphores) {
    const VkDeviceSize size = buffer_->get_size(); 

    const bool blocked =
	!in_flight_copies_.empty() &&
	(in_flight_copies_.front().virtual_counter_ + size) < virtual_counter_ + src.size();
    if (blocked) {
	VkFence vk_fence = in_flight_copies_.front().fence_->get_fence();
	vkWaitForFences(device_->get_device(), 1, &vk_fence, VK_TRUE, UINT64_MAX);
	free_copy_resources_.emplace_back(in_flight_copies_.front().command_, in_flight_copies_.front().fence_);
	in_flight_copies_.pop_front();
    } else if (free_copy_resources_.empty()) {
	free_copy_resources_.emplace_back(std::make_shared<Command>(command_pool_), std::make_shared<Fence>(device_, false));
    }
    
    const bool wraparound_necessary = (virtual_counter_ + src.size()) > size;

}
