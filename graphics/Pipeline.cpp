#include <filesystem>
#include <fstream>
#include <vector>

#include "utils/Assert.h"
#include "Pipeline.h"

Shader::Shader(std::shared_ptr<Device> device, std::string_view shader_name) {
    std::vector<std::string_view> possible_prefixes = {"", "build/", "illinois-voxel-sandbox/build/"};
    for (auto prefix : possible_prefixes) {
		std::string relative_path = std::string(prefix).append(shader_name).append(".spv");
		if (std::filesystem::exists(relative_path)) {
			std::ifstream fstream(relative_path, std::ios::in | std::ios::binary);
			const auto size = std::filesystem::file_size(relative_path);
			std::string result(size, '\0');
			fstream.read(result.data(), size);

			VkShaderModuleCreateInfo create_info {};
			create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			create_info.codeSize = result.size();
			create_info.pCode = (uint32_t*) result.data();

			ASSERT(vkCreateShaderModule(device->get_device(), &create_info, nullptr, &module_), "Unable to create shader module.");
			std::cout << "INFO: Loaded shader " << shader_name << ".\n";
			break;
		}
    }
    ASSERT(module_ != VK_NULL_HANDLE, std::string("Couldn't find shader named \"").append(shader_name).append("\"."));

    device_ = device;
}

Shader::~Shader() {
    vkDestroyShaderModule(device_->get_device(), module_, nullptr);
}
