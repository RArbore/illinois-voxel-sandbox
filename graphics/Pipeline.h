#include <vulkan/vulkan.h>

#include "Device.h"

class Shader {
public:
    Shader(std::shared_ptr<Device> device, std::string_view shader_name);
    ~Shader();
private:
    VkShaderModule module_ = VK_NULL_HANDLE;

    std::shared_ptr<Device> device_ = nullptr;
};
