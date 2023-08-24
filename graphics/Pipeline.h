#include <vector>

#include <vulkan/vulkan.h>

#include "Device.h"

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
    RayTracePipeline(std::shared_ptr<Device> device, std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups);
    ~RayTracePipeline();
private:
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;

    std::shared_ptr<Device> device_ = nullptr;
};
