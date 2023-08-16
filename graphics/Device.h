#pragma once

#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
