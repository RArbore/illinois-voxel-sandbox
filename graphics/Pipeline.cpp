#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "Pipeline.h"
#include "utils/Assert.h"

Shader::Shader(std::shared_ptr<Device> device, std::string_view shader_name) {
    std::vector<std::string_view> possible_prefixes = {
        "",
        "build/",
        "illinois-voxel-sandbox/build/",
        "../",
        "../../",
        "../../../",
        "../../../../"};
    for (auto prefix : possible_prefixes) {
        std::string relative_path =
            std::string(prefix).append(shader_name).append(".spv");
        if (std::filesystem::exists(relative_path)) {
            std::ifstream fstream(relative_path,
                                  std::ios::in | std::ios::binary);
            const auto size = std::filesystem::file_size(relative_path);
            std::string result(size, '\0');
            fstream.read(result.data(), size);

            VkShaderModuleCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = result.size();
            create_info.pCode = (uint32_t *)result.data();

            ASSERT(vkCreateShaderModule(device->get_device(), &create_info,
                                        nullptr, &module_),
                   "Unable to create shader module.");
            std::cout << "INFO: Loaded shader " << shader_name << ".\n";
            break;
        }
    }
    ASSERT(module_ != VK_NULL_HANDLE,
           std::string("Couldn't find shader named \"")
               .append(shader_name)
               .append("\"."));
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

Shader::~Shader() {
    vkDestroyShaderModule(device_->get_device(), module_, nullptr);
}

VkShaderModule Shader::get_module() { return module_; }

VkShaderStageFlagBits Shader::get_stage() { return stage_; }

std::vector<std::string> gather_custom_intersection_shader_names() {
    std::vector<std::string_view> possible_prefixes = {
        "./",
        "build/",
        "illinois-voxel-sandbox/build/",
        "../",
        "../../",
        "../../../",
        "../../../../"};

    std::unordered_set<std::string> shader_names;
    for (const auto &prefix : possible_prefixes) {
        if (std::filesystem::exists(prefix)) {
	    for (const auto &dir_entry : std::filesystem::directory_iterator(prefix)) {
		if (std::string(dir_entry.path()).ends_with("_intersect.spv")) {
		    shader_names.emplace(dir_entry.path().filename());
		}
	    }
	}
    }

    return std::vector<std::string>(shader_names.begin(), shader_names.end());
}

RayTracePipeline::RayTracePipeline(
    std::shared_ptr<GPUAllocator> allocator,
    std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups,
    std::vector<VkDescriptorSetLayout> descriptor_layouts) {
    device_ = allocator->get_device();

    std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;
    for (auto group : shader_groups) {
        ASSERT(group.size() > 0, "Shader groups must be non-empty.");
        for (auto shader : group) {
            stage_create_infos.emplace_back();
            stage_create_infos.back().sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage_create_infos.back().stage = shader->get_stage();
            stage_create_infos.back().module = shader->get_module();
            stage_create_infos.back().pName = "main";
        }
    }

    uint32_t shader_idx = 0;
    std::vector<uint32_t> gen_groups;
    std::vector<uint32_t> miss_groups;
    std::vector<uint32_t> hit_groups;
    std::vector<uint32_t> call_groups;
    for (auto group : shader_groups) {
        groups_.emplace_back();
        groups_.back().sType =
            VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        groups_.back().type =
            VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        groups_.back().anyHitShader = VK_SHADER_UNUSED_KHR;
        groups_.back().closestHitShader = VK_SHADER_UNUSED_KHR;
        groups_.back().generalShader = VK_SHADER_UNUSED_KHR;
        groups_.back().intersectionShader = VK_SHADER_UNUSED_KHR;
        for (auto it = group.begin(); it != group.end(); ++it) {
            bool first_shader = it == group.begin();
            auto shader = *it;
            VkShaderStageFlags stage = shader->get_stage();
            if ((stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR ||
                 stage == VK_SHADER_STAGE_MISS_BIT_KHR ||
                 stage == VK_SHADER_STAGE_CALLABLE_BIT_KHR) &&
                first_shader) {
                groups_.back().type =
                    VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                groups_.back().generalShader = shader_idx;
            } else if (
                (stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) &&
                (groups_.back().type ==
                     VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR ||
                 groups_.back().type ==
                     VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)) {
                groups_.back().closestHitShader = shader_idx;
            } else if (
                (stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR) &&
                (groups_.back().type ==
                     VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR ||
                 groups_.back().type ==
                     VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)) {
                groups_.back().anyHitShader = shader_idx;
            } else if (
                (stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR) &&
                (groups_.back().type ==
                     VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR ||
                 groups_.back().type ==
                     VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)) {
                groups_.back().type =
                    VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                groups_.back().intersectionShader = shader_idx;
            } else {
                ASSERT(false, "Shader group is mal-formed.");
            }
            if (first_shader) {
                if (stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR) {
                    gen_groups.push_back(
                        static_cast<uint32_t>(groups_.size() - 1));
                } else if (stage == VK_SHADER_STAGE_MISS_BIT_KHR) {
                    miss_groups.push_back(
                        static_cast<uint32_t>(groups_.size() - 1));
                } else if (stage == VK_SHADER_STAGE_CALLABLE_BIT_KHR) {
                    call_groups.push_back(
                        static_cast<uint32_t>(groups_.size() - 1));
                } else {
                    hit_groups.push_back(
                        static_cast<uint32_t>(groups_.size() - 1));
                }
            }
            ++shader_idx;
        }
    }

    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
    push_constant_range.offset = 0;
    push_constant_range.size = 128;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
    pipeline_layout_create_info.setLayoutCount =
        static_cast<uint32_t>(descriptor_layouts.size());
    pipeline_layout_create_info.pSetLayouts = descriptor_layouts.data();
    ASSERT(vkCreatePipelineLayout(device_->get_device(),
                                  &pipeline_layout_create_info, nullptr,
                                  &layout_),
           "Unable to create ray trace pipeline layout.");

    VkRayTracingPipelineCreateInfoKHR ray_trace_pipeline_create_info{};
    ray_trace_pipeline_create_info.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    ray_trace_pipeline_create_info.stageCount =
        static_cast<uint32_t>(stage_create_infos.size());
    ray_trace_pipeline_create_info.pStages = stage_create_infos.data();
    ray_trace_pipeline_create_info.groupCount =
        static_cast<uint32_t>(groups_.size());
    ray_trace_pipeline_create_info.pGroups = groups_.data();
    ray_trace_pipeline_create_info.maxPipelineRayRecursionDepth = 1;
    ray_trace_pipeline_create_info.layout = layout_;
    ASSERT(vkCreateRayTracingPipelines(device_->get_device(), {}, {}, 1,
                                       &ray_trace_pipeline_create_info, nullptr,
                                       &pipeline_),
           "Unable to create ray trace pipeline.");

    const auto align_up = [](uint32_t size, uint32_t alignment) {
        return (size + (alignment - 1)) & ~(alignment - 1);
    };

    const auto ray_tracing_properties = device_->get_ray_tracing_properties();
    const uint32_t handle_count = gen_groups.size() + miss_groups.size() +
                                  hit_groups.size() + call_groups.size();
    const uint32_t handle_size_aligned =
        align_up(ray_tracing_properties.shaderGroupHandleSize,
                 ray_tracing_properties.shaderGroupHandleAlignment);

    rgen_sbt_region_.stride = align_up(
        handle_size_aligned, ray_tracing_properties.shaderGroupBaseAlignment);
    rgen_sbt_region_.size =
        align_up(gen_groups.size() * handle_size_aligned,
                 ray_tracing_properties.shaderGroupBaseAlignment);
    miss_sbt_region_.stride = handle_size_aligned;
    miss_sbt_region_.size =
        align_up(miss_groups.size() * handle_size_aligned,
                 ray_tracing_properties.shaderGroupBaseAlignment);
    hit_sbt_region_.stride = handle_size_aligned;
    hit_sbt_region_.size =
        align_up(hit_groups.size() * handle_size_aligned,
                 ray_tracing_properties.shaderGroupBaseAlignment);
    call_sbt_region_.stride = handle_size_aligned;
    call_sbt_region_.size =
        align_up(call_groups.size() * handle_size_aligned,
                 ray_tracing_properties.shaderGroupBaseAlignment);

    const uint32_t handles_size =
        handle_count * ray_tracing_properties.shaderGroupHandleSize;
    std::vector<char> handles(handles_size);
    ASSERT(vkGetRayTracingShaderGroupHandles(device_->get_device(), pipeline_,
                                             0, handle_count, handles_size,
                                             handles.data()),
           "Unable to fetch shader group handles from ray trace pipeline.");

    const VkDeviceSize sbt_buffer_size =
        rgen_sbt_region_.size + miss_sbt_region_.size + hit_sbt_region_.size +
        call_sbt_region_.size + 16;
    sbt_buffer_ = std::make_shared<GPUBuffer>(
        allocator, sbt_buffer_size, 1,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    const VkDeviceAddress sbt_buffer_address =
        sbt_buffer_->get_device_address();
    rgen_sbt_region_.deviceAddress = sbt_buffer_address;
    miss_sbt_region_.deviceAddress = sbt_buffer_address + rgen_sbt_region_.size;
    hit_sbt_region_.deviceAddress =
        sbt_buffer_address + rgen_sbt_region_.size + miss_sbt_region_.size;
    call_sbt_region_.deviceAddress =
        sbt_buffer_address + rgen_sbt_region_.size + miss_sbt_region_.size +
        hit_sbt_region_.size;

    const auto get_handle = [&](uint32_t i) {
        return handles.data() +
               i * ray_tracing_properties.shaderGroupHandleSize;
    };
    sbt_buffer_->cpu_map([&](char *base_dst) {
        char *dst = base_dst;
        for (uint32_t i = 0; i < gen_groups.size(); ++i) {
            memcpy(dst, get_handle(gen_groups.at(i)),
                   ray_tracing_properties.shaderGroupHandleSize);
            dst += rgen_sbt_region_.stride;
        }

        dst = base_dst + rgen_sbt_region_.size;
        for (uint32_t i = 0; i < miss_groups.size(); ++i) {
            memcpy(dst, get_handle(miss_groups.at(i)),
                   ray_tracing_properties.shaderGroupHandleSize);
            dst += miss_sbt_region_.stride;
        }

        dst = base_dst + rgen_sbt_region_.size + miss_sbt_region_.size;
        for (uint32_t i = 0; i < hit_groups.size(); ++i) {
            memcpy(dst, get_handle(hit_groups.at(i)),
                   ray_tracing_properties.shaderGroupHandleSize);
            dst += hit_sbt_region_.stride;
        }

        dst = base_dst + rgen_sbt_region_.size + miss_sbt_region_.size +
              hit_sbt_region_.size;
        for (uint32_t i = 0; i < call_groups.size(); ++i) {
            memcpy(dst, get_handle(call_groups.at(i)),
                   ray_tracing_properties.shaderGroupHandleSize);
            dst += call_sbt_region_.stride;
        }
    });
}

RayTracePipeline::~RayTracePipeline() {
    vkDestroyPipeline(device_->get_device(), pipeline_, nullptr);
    vkDestroyPipelineLayout(device_->get_device(), layout_, nullptr);
}

void RayTracePipeline::record(
    VkCommandBuffer command,
    std::vector<std::shared_ptr<DescriptorSet>> descriptor_sets,
    std::span<std::byte> push_constants, uint32_t width, uint32_t height,
    uint32_t depth) {
    ASSERT(push_constants.size() == 128,
           "Push constants must take exactly 128 bytes.");

    std::vector<VkDescriptorSet> vk_descriptor_sets;
    for (auto set : descriptor_sets) {
        vk_descriptor_sets.push_back(set->get_set());
    }

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                      pipeline_);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            layout_, 0,
                            static_cast<uint32_t>(vk_descriptor_sets.size()),
                            vk_descriptor_sets.data(), 0, nullptr);
    vkCmdPushConstants(command, layout_, VK_SHADER_STAGE_ALL, 0, 128,
                       push_constants.data());
    vkCmdTraceRays(command, &rgen_sbt_region_, &miss_sbt_region_,
                   &hit_sbt_region_, &call_sbt_region_, width, height, depth);
}

ComputePipeline::ComputePipeline(
    std::shared_ptr<GPUAllocator> allocator,
    std::shared_ptr<Shader> compute_shader,
    std::vector<VkDescriptorSetLayout> descriptor_layouts) {
    device_ = allocator->get_device();

    VkPipelineShaderStageCreateInfo compute_info{};
    compute_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compute_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compute_info.module = compute_shader->get_module();
    compute_info.pName = "main";

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount =
        static_cast<uint32_t>(descriptor_layouts.size());
    pipeline_layout_create_info.pSetLayouts = descriptor_layouts.data();
    ASSERT(vkCreatePipelineLayout(device_->get_device(),
                                  &pipeline_layout_create_info, nullptr,
                                  &layout_),
           "Unable to create compute pipeline layout.");

    VkComputePipelineCreateInfo compute_pipeline_info{};
    compute_pipeline_info.sType =
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_info.layout = layout_;
    compute_pipeline_info.stage = compute_info;
    ASSERT(vkCreateComputePipelines(device_->get_device(), VK_NULL_HANDLE, 1,
                                    &compute_pipeline_info, nullptr,
                                    &pipeline_),
           "Unabled to create compute pipeline.");
}

ComputePipeline::~ComputePipeline() {
    vkDestroyPipeline(device_->get_device(), pipeline_, nullptr);
    vkDestroyPipelineLayout(device_->get_device(), layout_, nullptr);
}

void ComputePipeline::record(
    VkCommandBuffer command,
    std::vector<std::shared_ptr<DescriptorSet>> descriptor_sets, uint32_t width,
    uint32_t height, uint32_t depth) {

    std::vector<VkDescriptorSet> vk_descriptor_sets;
    for (auto set : descriptor_sets) {
        vk_descriptor_sets.push_back(set->get_set());
    }

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, layout_, 0,
                            static_cast<uint32_t>(vk_descriptor_sets.size()),
                            vk_descriptor_sets.data(), 0, nullptr);

    vkCmdDispatch(command, width, height, depth);
}
