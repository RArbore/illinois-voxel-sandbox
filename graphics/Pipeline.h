#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Device.h"
#include "GPUAllocator.h"

class Shader {
public:
    Shader(std::shared_ptr<Device> device, std::string_view shader_name);
    ~Shader();

    VkShaderModule get_module(); 
    VkShaderStageFlagBits get_stage();
private:
    VkShaderModule module_ = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage_ = VK_SHADER_STAGE_ALL;

    std::shared_ptr<Device> device_ = nullptr;
    std::string_view shader_name_;
};

class RayTracePipeline {
public:
    RayTracePipeline(std::shared_ptr<GPUAllocator> allocator, std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups);
    ~RayTracePipeline();
private:
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR > groups_;

    std::shared_ptr<GPUBuffer> sbt_buffer_ = nullptr;
    VkStridedDeviceAddressRegionKHR rgen_sbt_region_;
    VkStridedDeviceAddressRegionKHR miss_sbt_region_;
    VkStridedDeviceAddressRegionKHR hit_sbt_region_;
    VkStridedDeviceAddressRegionKHR call_sbt_region_;

    std::shared_ptr<Device> device_ = nullptr;
};