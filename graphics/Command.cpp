#include "Command.h"

CommandPool::CommandPool(std::shared_ptr<Device> device) {
    VkCommandPoolCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = device->get_queue_family();

    ASSERT(vkCreateCommandPool(device->get_device(), &create_info, nullptr, &command_pool_), "Unable to create command pool.");

    device_ = device;
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(device_->get_device(), command_pool_, nullptr);
}

VkCommandPool CommandPool::get_pool() {
    return command_pool_;
}

std::shared_ptr<Device> CommandPool::get_device() {
    return device_;
}

Command::Command(std::shared_ptr<CommandPool> command_pool) {
    VkCommandBufferAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool->get_pool();
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    ASSERT(vkAllocateCommandBuffers(command_pool->get_device()->get_device(), &allocate_info, &buffer_), "Unable to create command buffers.");

    command_pool_ = command_pool;
}

VkCommandBuffer Command::get_buffer() {
    return buffer_;
}
