#pragma once

#include <string_view>

#include <vulkan/vulkan.h>

class Device {
public:
    Device();
    ~Device();

private:
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
};
