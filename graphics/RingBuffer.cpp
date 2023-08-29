#include <cstring>

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
    ASSERT(src.size() <= size, "Copy size is too large for ring buffer.");

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
    
    const auto [command, fence] = free_copy_resources_.back();
    free_copy_resources_.pop_back();

    const bool wraparound_necessary = (virtual_counter_ + src.size()) > size;
    if (!wraparound_necessary) {
	void *const dst_ptr = buffer_map_.data() + (virtual_counter_ % size);
	memcpy(dst_ptr, src.data(), src.size());

	command->record([&](VkCommandBuffer command) {
	    VkBufferCopy copy {};
	    copy.srcOffset = virtual_counter_ % size;
	    copy.dstOffset = 0;
	    copy.size = src.size();
	    vkCmdCopyBuffer(command, buffer_->get_buffer(), dst->get_buffer(), 1, &copy);
	});
	device_->submit_command(command, wait_semaphores, signal_semaphores, fence);

	InFlightCopy copy {};
	copy.command_ = command;
	copy.fence_ = fence;
	copy.virtual_counter_ = virtual_counter_;
	copy.size_ = src.size();
	copy.gpu_buffer_ = dst;
	in_flight_copies_.push_back(copy);
	virtual_counter_ += src.size();
    } else {
	void *const first_dst_ptr = buffer_map_.data() + (virtual_counter_ % size);
	void *const second_dst_ptr = buffer_map_.data();
	const VkDeviceSize first_size = size - (virtual_counter_ % size);
	const VkDeviceSize second_size = src.size() - first_size;
	memcpy(first_dst_ptr, src.data(), first_size);
	memcpy(second_dst_ptr, src.data() + first_size, second_size);

	command->record([&](VkCommandBuffer command) {
	    VkBufferCopy first_copy {};
	    first_copy.srcOffset = virtual_counter_ % size;
	    first_copy.dstOffset = 0;
	    first_copy.size = first_size;
	    VkBufferCopy second_copy {};
	    second_copy.srcOffset = 0;
	    second_copy.dstOffset = first_size;
	    second_copy.size = second_size;
	    VkBufferCopy copies[2] = {first_copy, second_copy};
	    vkCmdCopyBuffer(command, buffer_->get_buffer(), dst->get_buffer(), 2, copies);
	});
	device_->submit_command(command, wait_semaphores, signal_semaphores, fence);
	
	InFlightCopy copy {};
	copy.command_ = command;
	copy.fence_ = fence;
	copy.virtual_counter_ = virtual_counter_;
	copy.size_ = src.size();
	copy.gpu_buffer_ = dst;
	in_flight_copies_.push_back(copy);
	virtual_counter_ += src.size();
    }
}
