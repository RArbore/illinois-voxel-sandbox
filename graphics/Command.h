#pragma once

#include <vulkan/vulkan.h>

#include "utils/Assert.h"
#include "Device.h"

class CommandPool {
public:
    CommandPool(std::shared_ptr<Device> device);
    ~CommandPool();

    VkCommandPool get_pool();
    std::shared_ptr<Device> get_device();
private:
    VkCommandPool command_pool_;
    
    std::shared_ptr<Device> device_;
};

class Command {
public:
    Command(std::shared_ptr<CommandPool> command_pool);

    VkCommandBuffer get_buffer();
private:
    VkCommandBuffer buffer_;

    std::shared_ptr<CommandPool> command_pool_;
public:
    void record(auto F) {
	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	ASSERT(vkBeginCommandBuffer(buffer_, &begin_info), "Unable to begin recording command buffer.");
	F(buffer_);
	ASSERT(vkEndCommandBuffer(buffer_), "Something went wrong recording command buffer.");
    }
};
