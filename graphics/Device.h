#pragma once

#include <string_view>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Window.h"

class Device {
public:
    Device(std::shared_ptr<Window> window);
    ~Device();

    VkInstance get_instance();
    VkPhysicalDevice get_physical_device();
    VkDevice get_device();
    VkSurfaceKHR get_surface();
private:
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue queue_;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties_;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties_;

    std::shared_ptr<Window> window_ = nullptr;
};
