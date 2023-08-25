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
    if (shader_name.find("rgen") != std::string_view::npos) {
	stage_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    } else if (shader_name.find("rchit") != std::string_view::npos) {
	stage_ = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    } else if (shader_name.find("rmiss") != std::string_view::npos) {
	stage_ = VK_SHADER_STAGE_MISS_BIT_KHR;
    } else if (shader_name.find("rint") != std::string_view::npos) {
	stage_ = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    } else if (shader_name.find("call") != std::string_view::npos) {
	stage_ = VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    } else if (shader_name.find("comp") != std::string_view::npos) {
	stage_ = VK_SHADER_STAGE_COMPUTE_BIT;
    } else {
	ASSERT(false, "Didn't recognize shader type in shader name.");
    }

    device_ = device;
    shader_name_ = shader_name;
}

VkShaderModule Shader::get_module() {
    return module_;
}

VkShaderStageFlagBits Shader::get_stage() {
    return stage_;
}

Shader::~Shader() {
    vkDestroyShaderModule(device_->get_device(), module_, nullptr);
}

RayTracePipeline::RayTracePipeline(std::shared_ptr<Device> device, std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups) {
    std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;
    for (auto group : shader_groups) {
	ASSERT(group.size() > 0, "Shader groups must be non-empty.");
	for (auto shader : group) {
	    stage_create_infos.emplace_back();
	    stage_create_infos.back().sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	    stage_create_infos.back().stage = shader->get_stage();
	    stage_create_infos.back().module = shader->get_module();
	    stage_create_infos.back().pName = "main";
	}
    }

    uint32_t shader_idx = 0;
    for (auto group : shader_groups) {
	groups_.emplace_back();
	groups_.back().sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groups_.back().type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	groups_.back().anyHitShader = VK_SHADER_UNUSED_KHR;
	groups_.back().closestHitShader = VK_SHADER_UNUSED_KHR;
	groups_.back().generalShader = VK_SHADER_UNUSED_KHR;
	groups_.back().intersectionShader = VK_SHADER_UNUSED_KHR;
	for (auto it = group.begin(); it != group.end(); ++it) {
	    bool first_shader = it == group.begin();
	    auto shader = *it;
	    VkShaderStageFlags stage = shader->get_stage();
	    if ((stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR || stage == VK_SHADER_STAGE_MISS_BIT_KHR || stage == VK_SHADER_STAGE_CALLABLE_BIT_KHR) && first_shader) {
		groups_.back().type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groups_.back().generalShader = shader_idx;
	    } else if ((stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) && (groups_.back().type == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR || groups_.back().type == VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)) {
		groups_.back().closestHitShader = shader_idx;
	    } else if ((stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR) && (groups_.back().type == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR || groups_.back().type == VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)) {
		groups_.back().anyHitShader = shader_idx;
	    } else if ((stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR) && (first_shader || groups_.back().type == VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)) {
		groups_.back().type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
		groups_.back().intersectionShader = shader_idx;
	    } else {
		ASSERT(false, "Shader group is mal-formed.");
	    }
	    ++shader_idx;
	}
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.setLayoutCount = 0;
    ASSERT(vkCreatePipelineLayout(device->get_device(), &pipeline_layout_create_info, nullptr, &layout_), "Unable to create ray trace pipeline layout.");
    
    VkRayTracingPipelineCreateInfoKHR ray_trace_pipeline_create_info {};
    ray_trace_pipeline_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    ray_trace_pipeline_create_info.stageCount = static_cast<uint32_t>(stage_create_infos.size());
    ray_trace_pipeline_create_info.pStages = stage_create_infos.data();
    ray_trace_pipeline_create_info.groupCount = static_cast<uint32_t>(groups_.size());
    ray_trace_pipeline_create_info.pGroups = groups_.data();
    ray_trace_pipeline_create_info.maxPipelineRayRecursionDepth = 1;
    ray_trace_pipeline_create_info.layout = layout_;
    ASSERT(vkCreateRayTracingPipelines(device->get_device(), {}, {}, 1, &ray_trace_pipeline_create_info, nullptr, &pipeline_), "Unable to create ray trace pipeline.");
    
    device_ = device;
}

RayTracePipeline::~RayTracePipeline() {
    vkDestroyPipeline(device_->get_device(), pipeline_, nullptr);
    vkDestroyPipelineLayout(device_->get_device(), layout_, nullptr);
}
