#pragma once

#include <string_view>
#include <vector>
#include <memory>
#include <mutex>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Window.h"

#define VKFN_EXTERN(fn) extern PFN_##fn##KHR fn

#define VKFN_DEF(fn) PFN_##fn##KHR fn

#define VKFN_INIT(fn)                                                          \
    fn = (PFN_##fn##KHR)vkGetDeviceProcAddr(device_, #fn "KHR")

VKFN_EXTERN(vkGetAccelerationStructureBuildSizes);
VKFN_EXTERN(vkCreateAccelerationStructure);
VKFN_EXTERN(vkCmdBuildAccelerationStructures);
VKFN_EXTERN(vkGetAccelerationStructureDeviceAddress);
VKFN_EXTERN(vkCreateRayTracingPipelines);
VKFN_EXTERN(vkGetRayTracingShaderGroupHandles);
VKFN_EXTERN(vkDestroyAccelerationStructure);
VKFN_EXTERN(vkCmdTraceRays);

class Command;
class Semaphore;
class Fence;

class Device {
  public:
    Device(std::shared_ptr<Window> window);
    ~Device();

    VkInstance get_instance();
    VkPhysicalDevice get_physical_device();
    VkDevice get_device();
    VkSurfaceKHR get_surface();
    VkQueue get_queue();
    uint32_t get_queue_family();

    void submit_command(
        std::shared_ptr<Command> command,
        std::vector<std::shared_ptr<Semaphore>> wait_semaphores = {},
        std::vector<std::shared_ptr<Semaphore>> signal_semaphores = {},
        std::shared_ptr<Fence> fence = nullptr);

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR
    get_ray_tracing_properties();
    VkPhysicalDeviceAccelerationStructurePropertiesKHR
    get_acceleration_structure_properties();

  private:
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue queue_;

    std::mutex queue_mutex_;

    uint32_t queue_family_;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties_;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR
        acceleration_structure_properties_;

    std::shared_ptr<Window> window_ = nullptr;
};
