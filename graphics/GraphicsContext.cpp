#include "GraphicsContext.h"
#include "Window.h"
#include "Device.h"
#include "GPUAllocator.h"
#include "Swapchain.h"
#include "Command.h"
#include "Pipeline.h"
#include "Synchronization.h"
#include "Descriptor.h"
#include "RingBuffer.h"

class GraphicsContext {
public:
    GraphicsContext();
    ~GraphicsContext();
private:
    std::shared_ptr<Window> window_ = nullptr;
    std::shared_ptr<Device> device_ = nullptr;
    std::shared_ptr<GPUAllocator> gpu_allocator_ = nullptr;
    std::shared_ptr<Swapchain> swapchain_ = nullptr;
    std::shared_ptr<CommandPool> command_pool_ = nullptr;
    std::shared_ptr<RayTracePipeline> ray_trace_pipeline_ = nullptr;
    std::shared_ptr<DescriptorAllocator> descriptor_allocator_ = nullptr;
    std::shared_ptr<RingBuffer> ring_buffer_ = nullptr;

    std::vector<std::shared_ptr<DescriptorSet>> swapchain_descriptors_;
    std::shared_ptr<Fence> frame_fence_ = nullptr;
    std::shared_ptr<Semaphore> acquire_semaphore_ = nullptr;
    std::shared_ptr<Semaphore> render_semaphore_ = nullptr;
    std::shared_ptr<Command> render_command_buffer_ = nullptr;

    uint64_t frame_index_ = 0;

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context, std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel> build_model(std::shared_ptr<GraphicsContext> context, const VoxelChunk &chunk);
    friend std::shared_ptr<GraphicsScene> build_scene(std::shared_ptr<GraphicsContext> context, const std::vector<std::shared_ptr<GraphicsModel>> &models);
};

class GraphicsModel {
public:
    GraphicsModel();
private:

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context, std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel> build_model(std::shared_ptr<GraphicsContext> context, const VoxelChunk &chunk);
    friend std::shared_ptr<GraphicsScene> build_scene(std::shared_ptr<GraphicsContext> context, const std::vector<std::shared_ptr<GraphicsModel>> &models);
};

class GraphicsScene {
public:
    GraphicsScene();
private:

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context, std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel> build_model(std::shared_ptr<GraphicsContext> context, const VoxelChunk &chunk);
    friend std::shared_ptr<GraphicsScene> build_scene(std::shared_ptr<GraphicsContext> context, const std::vector<std::shared_ptr<GraphicsModel>> &models);
};

GraphicsContext::GraphicsContext() {
    window_ = std::make_shared<Window>();
    device_ = std::make_shared<Device>(window_);
    gpu_allocator_ = std::make_shared<GPUAllocator>(device_);
    swapchain_ = std::make_shared<Swapchain>(device_, window_);
    command_pool_ = std::make_shared<CommandPool>(device_);
    descriptor_allocator_ = std::make_shared<DescriptorAllocator>(device_);
    ring_buffer_ = std::make_shared<RingBuffer>(gpu_allocator_, command_pool_, 1 << 24);

    swapchain_descriptors_ = swapchain_->make_image_descriptors(descriptor_allocator_);
    auto shader = std::make_shared<Shader>(device_, "dumb_rgen");
    std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups = {{shader}};
    std::vector<VkDescriptorSetLayout> layouts = {swapchain_descriptors_.at(0)->get_layout()};
    ray_trace_pipeline_ = std::make_shared<RayTracePipeline>(gpu_allocator_, shader_groups, layouts);
    frame_fence_ = std::make_shared<Fence>(device_);
    acquire_semaphore_ = std::make_shared<Semaphore>(device_);
    render_semaphore_ = std::make_shared<Semaphore>(device_);
    render_command_buffer_ = std::make_shared<Command>(command_pool_);
}

GraphicsContext::~GraphicsContext() {
    vkDeviceWaitIdle(device_->get_device());
}

std::shared_ptr<GraphicsContext> create_graphics_context() {
    auto context = std::shared_ptr<GraphicsContext>(new GraphicsContext());
    return context;
}

void render_frame(std::shared_ptr<GraphicsContext> context, std::shared_ptr<GraphicsScene> scene) {
    context->window_->pollEvents();

    context->frame_fence_->wait();

    const uint32_t swapchain_image_index = context->swapchain_->acquire_next_image(context->acquire_semaphore_);

    Barrier prologue_barrier(context->device_,
			     VK_PIPELINE_STAGE_2_NONE,
			     VK_ACCESS_2_NONE,
			     VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			     VK_ACCESS_2_SHADER_WRITE_BIT,
			     context->swapchain_->get_image(swapchain_image_index),
			     VK_IMAGE_LAYOUT_UNDEFINED,
			     VK_IMAGE_LAYOUT_GENERAL
			     );

    Barrier epilogue_barrier(context->device_,
			     VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			     VK_ACCESS_2_SHADER_WRITE_BIT,
			     VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			     VK_ACCESS_2_MEMORY_READ_BIT,
			     context->swapchain_->get_image(swapchain_image_index),
			     VK_IMAGE_LAYOUT_GENERAL,
			     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			     );
    
    context->frame_fence_->reset();

    context->render_command_buffer_->record([&](VkCommandBuffer command_buffer) {
	VkExtent2D extent = context->swapchain_->get_extent();
	
	prologue_barrier.record(command_buffer);
	context->ray_trace_pipeline_->record(command_buffer, {context->swapchain_descriptors_.at(swapchain_image_index)}, extent.width, extent.height, 1);
	epilogue_barrier.record(command_buffer);
    });

    context->device_->submit_command(context->render_command_buffer_, {context->acquire_semaphore_}, {context->render_semaphore_}, context->frame_fence_);
    
    context->swapchain_->present_image(swapchain_image_index, context->render_semaphore_);

    ++context->frame_index_;
}

bool should_exit(std::shared_ptr<GraphicsContext> context) {
    return context->window_->shouldClose();
}

std::vector<std::shared_ptr<GraphicsModel>> build_models(std::shared_ptr<GraphicsContext> context, std::vector<std::reference_wrapper<const VoxelChunk>> chunks) {
    /*VkAabbPositionsKHR cube {};
    cube.minX = 0.0f;
    cube.minY = 0.0f;
    cube.minZ = 0.0f;
    cube.maxX = 1.0f;
    cube.maxY = 1.0f;
    cube.maxZ = 1.0f;

    std::shared_ptr<GPUBuffer> cube_buffer = std::make_shared<GPUBuffer>(context->gpu_allocator_,
									 sizeof(VkAabbPositionsKHR),
									 1,
									 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
									 0);
    std::shared_ptr<Semaphore> cube_upload = std::make_shared<Semaphore>(context->device_);
    context->ring_buffer_->copy_to_device(cube_buffer,
					  0,
					  std::as_bytes(std::span<VkAabbPositionsKHR>(&cube, 1)),
					  {},
					  {cube_upload});

    const VkDeviceSize alignment = context->device_->get_acceleration_structure_properties().minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryAabbsDataKHR geometry_aabbs_data {};
    geometry_aabbs_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry_aabbs_data.data.deviceAddress = cube_buffer->get_device_address();
    geometry_aabbs_data.stride = sizeof(VkAabbPositionsKHR);

    VkAccelerationStructureGeometryKHR blas_geometry {};
    blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas_geometry.geometry.aabbs = geometry_aabbs_data;

    VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info {};
    blas_build_range_info.firstVertex = 0;
    blas_build_range_info.primitiveCount = 1;
    blas_build_range_info.primitiveOffset = 0;
    blas_build_range_info.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR blas_build_range_infos[1][1] = {{blas_build_range_info}};

    VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info {};
    blas_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blas_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blas_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blas_build_geometry_info.geometryCount = 1;
    blas_build_geometry_info.pGeometries = &blas_geometry;

    const uint32_t max_primitive_counts[] = {1};

    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes_info {};
    blas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizes(context->device_->get_device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blas_build_geometry_info, max_primitive_counts, &blas_build_sizes_info);

    std::shared_ptr<GPUBuffer> scratch_buffer = std::make_shared<GPUBuffer>(context->gpu_allocator_,
									    blas_build_sizes_info.buildScratchSize,
									    alignment,
									    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
									    0);

    std::shared_ptr<GPUBuffer> storage_buffer = std::make_shared<GPUBuffer>(context->gpu_allocator_,
									    blas_build_sizes_info.accelerationStructureSize,
									    alignment,
									    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
									    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
									    0);

    VkAccelerationStructureCreateInfoKHR bottom_level_create_info {};
    bottom_level_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    bottom_level_create_info.buffer = storage_buffer->get_buffer();
    bottom_level_create_info.offset = 0;
    bottom_level_create_info.size = blas_build_sizes_info.accelerationStructureSize;
    bottom_level_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VkAccelerationStructureKHR bottom_level_acceleration_structure;
    ASSERT(vkCreateAccelerationStructure(context->device_->get_device(), &bottom_level_create_info, NULL, &bottom_level_acceleration_structure), "Unable to create bottom level acceleration structure.");

    blas_build_geometry_info.dstAccelerationStructure = bottom_level_acceleration_structure;
    blas_build_geometry_info.scratchData.deviceAddress = scratch_buffer->get_device_address();

    std::shared_ptr<Command> build_command = std::make_shared<Command>(context->command_pool_);
    build_command->record([&](VkCommandBuffer command) {
	vkCmdBuildAccelerationStructures(command, 1, &blas_build_geometry_info, reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR* const*>(blas_build_range_infos));
    });
    
    return nullptr;*/
    return {};
}

std::shared_ptr<GraphicsScene> build_scene(std::shared_ptr<GraphicsContext> context, const std::vector<std::shared_ptr<GraphicsModel>> &models) {
    return nullptr;
}
