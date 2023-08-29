#include "utils/Assert.h"
#include "Synchronization.h"

Fence::Fence(std::shared_ptr<Device> device, bool create_signaled) {
    VkFenceCreateInfo fence_info {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (create_signaled) {
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    ASSERT(vkCreateFence(device->get_device(), &fence_info, nullptr, &fence_), "Unable to create fence.");

    device_ = device;
}

void Fence::wait() {
    vkWaitForFences(device_->get_device(), 1, &fence_, VK_TRUE, UINT64_MAX);
}

void Fence::reset() {
    vkResetFences(device_->get_device(), 1, &fence_);
}

VkFence Fence::get_fence() {
    return fence_;
}

Fence::~Fence() {
    vkDestroyFence(device_->get_device(), fence_, nullptr);
}

Semaphore::Semaphore(std::shared_ptr<Device> device) {
    VkSemaphoreCreateInfo semaphore_info {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    ASSERT(vkCreateSemaphore(device->get_device(), &semaphore_info, nullptr, &semaphore_), "Unable to create semaphore.");

    device_ = device;
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(device_->get_device(), semaphore_, nullptr);
}

VkSemaphore Semaphore::get_semaphore() {
    return semaphore_;
}

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses);
}

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses, buffer, offset, size);
}

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange range) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses, image, old_layout, new_layout, range);
}

void Barrier::add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses) {
    VkMemoryBarrier2 barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = src_stages;
    barrier.srcAccessMask = src_accesses;
    barrier.dstStageMask = dst_stages;
    barrier.dstAccessMask = dst_accesses;

    global_barriers_.push_back(barrier);
}

void Barrier::add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
    VkBufferMemoryBarrier2 barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = src_stages;
    barrier.srcAccessMask = src_accesses;
    barrier.dstStageMask = dst_stages;
    barrier.dstAccessMask = dst_accesses;
    barrier.srcQueueFamilyIndex = device_->get_queue_family();
    barrier.dstQueueFamilyIndex = device_->get_queue_family();
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    buffer_barriers_.push_back(barrier);
}

void Barrier::add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange range) {
    VkImageMemoryBarrier2 barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = src_stages;
    barrier.srcAccessMask = src_accesses;
    barrier.dstStageMask = dst_stages;
    barrier.dstAccessMask = dst_accesses;
    barrier.srcQueueFamilyIndex = device_->get_queue_family();
    barrier.dstQueueFamilyIndex = device_->get_queue_family();
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.image = image;
    barrier.subresourceRange = range;

    image_barriers_.push_back(barrier);
}

void Barrier::record(VkCommandBuffer command) {
    VkDependencyInfo dependency_info {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.pNext = nullptr;
    dependency_info.dependencyFlags = {};
    dependency_info.memoryBarrierCount = static_cast<uint32_t>(global_barriers_.size());
    dependency_info.pMemoryBarriers = global_barriers_.data();
    dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers_.size());
    dependency_info.pBufferMemoryBarriers = buffer_barriers_.data();
    dependency_info.imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers_.size());
    dependency_info.pImageMemoryBarriers = image_barriers_.data();

    vkCmdPipelineBarrier2(command, &dependency_info);
}
