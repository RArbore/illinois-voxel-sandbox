#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

class CommandPool {
public:
    CommandPool(std::shared_ptr<Device> device);
    ~CommandPool();

    VkCommandPool get_command_pool();
    std::shared_ptr<Device> get_device();
private:
    VkCommandPool command_pool_;
    
    std::shared_ptr<Device> device_;
};

class Command {
public:
    Command(std::shared_ptr<CommandPool> command_pool);
    ~Command();
private:
    VkCommandBuffer buffer_;

    std::shared_ptr<CommandPool> command_pool_;
};
