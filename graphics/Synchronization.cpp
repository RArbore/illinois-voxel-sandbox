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

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses);
}

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUBuffer> buffer, VkDeviceSize offset, VkDeviceSize size) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses, buffer, offset, size);
}

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUImage> image, VkImageSubresourceRange range) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses, image, range);
}

Barrier::Barrier(std::shared_ptr<Device> device, VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUVolume> volume, VkImageSubresourceRange range) {
    device_ = device;
    add(src_stages, src_accesses, dst_stages, dst_accesses, volume, range);
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

void Barrier::add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUBuffer> buffer, VkDeviceSize offset, VkDeviceSize size) {
    VkBufferMemoryBarrier2 barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = src_stages;
    barrier.srcAccessMask = src_accesses;
    barrier.dstStageMask = dst_stages;
    barrier.dstAccessMask = dst_accesses;
    barrier.srcQueueFamilyIndex = device_->get_queue_family();
    barrier.dstQueueFamilyIndex = device_->get_queue_family();
    barrier.buffer = buffer->get_buffer();
    barrier.offset = offset;
    barrier.size = size;

    buffer_barriers_.push_back(barrier);
    referenced_buffers_.push_back(buffer);
}

void Barrier::add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUImage> image, VkImageSubresourceRange range) {
    VkImageMemoryBarrier2 barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = src_stages;
    barrier.srcAccessMask = src_accesses;
    barrier.dstStageMask = dst_stages;
    barrier.dstAccessMask = dst_accesses;
    barrier.srcQueueFamilyIndex = device_->get_queue_family();
    barrier.dstQueueFamilyIndex = device_->get_queue_family();
    barrier.image = image->get_image();
    barrier.subresourceRange = range;

    image_barriers_.push_back(barrier);
    referenced_images_.push_back(image);
}

void Barrier::add(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_accesses, VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_accesses, std::shared_ptr<GPUVolume> volume, VkImageSubresourceRange range) {
    VkImageMemoryBarrier2 barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = src_stages;
    barrier.srcAccessMask = src_accesses;
    barrier.dstStageMask = dst_stages;
    barrier.dstAccessMask = dst_accesses;
    barrier.srcQueueFamilyIndex = device_->get_queue_family();
    barrier.dstQueueFamilyIndex = device_->get_queue_family();
    barrier.image = volume->get_image();
    barrier.subresourceRange = range;

    image_barriers_.push_back(barrier);
    referenced_volumes_.push_back(volume);
}
