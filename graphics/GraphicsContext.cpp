#include <cstring>
#include <chrono>
#include <map>

#include "Command.h"
#include "Descriptor.h"
#include "Device.h"
#include "GPUAllocator.h"
#include "Geometry.h"
#include "GraphicsContext.h"
#include "Pipeline.h"
#include "RingBuffer.h"
#include "Swapchain.h"
#include "Synchronization.h"
#include "Window.h"

const uint32_t UNLOADED_SBT_OFFSET = 0;
const std::map<std::pair<VoxelChunk::Format, VoxelChunk::AttributeSet>, uint32_t> FORMAT_TO_SBT_OFFSET = {
    {{VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Color}, 1},
};
const uint64_t MAX_NUM_CHUNKS_LOADED_PER_FRAME = 32;

class GraphicsContext {
  public:
    GraphicsContext();
    ~GraphicsContext();

    std::shared_ptr<Window> window_ = nullptr;
    std::shared_ptr<Device> device_ = nullptr;
    std::shared_ptr<GPUAllocator> gpu_allocator_ = nullptr;
    std::shared_ptr<Swapchain> swapchain_ = nullptr;
    std::shared_ptr<CommandPool> command_pool_ = nullptr;
    std::shared_ptr<RayTracePipeline> ray_trace_pipeline_ = nullptr;
    std::shared_ptr<DescriptorAllocator> descriptor_allocator_ = nullptr;
    std::shared_ptr<RingBuffer> ring_buffer_ = nullptr;

    std::shared_ptr<Fence> frame_fence_ = nullptr;
    std::shared_ptr<Semaphore> acquire_semaphore_ = nullptr;
    std::shared_ptr<Semaphore> render_semaphore_ = nullptr;
    std::shared_ptr<Command> render_command_buffer_ = nullptr;

    std::shared_ptr<GPUBuffer> unloaded_chunks_buffer_ = nullptr;
    std::span<std::byte> unloaded_chunks_span_;

    std::shared_ptr<DescriptorSet> wide_descriptor_ = nullptr;

    std::shared_ptr<GraphicsScene> current_scene_ = nullptr;

    uint64_t frame_index_ = 0;

    std::chrono::time_point<std::chrono::system_clock> start_time_;
    uint64_t start_frame_ = 0;

    std::chrono::time_point<std::chrono::system_clock> first_time_;
    uint64_t elapsed_ms_;

    void check_chunk_request_buffer(std::vector<std::shared_ptr<Semaphore>> &render_wait_semaphores);
};

class GraphicsModel {
  public:
    GraphicsModel(std::shared_ptr<BLAS> blas,
                  VoxelChunkPtr chunk,
		  uint32_t sbt_offset)
        : blas_(blas), chunk_(chunk), sbt_offset_(sbt_offset) {}

    std::shared_ptr<BLAS> blas_;
    VoxelChunkPtr chunk_;
    uint32_t sbt_offset_;
};

class GraphicsObject {
  public:
    GraphicsObject(std::shared_ptr<GraphicsModel> model, glm::mat3x4 transform)
        : model_(model), transform_(transform) {}

    std::shared_ptr<GraphicsModel> model_;
    glm::mat3x4 transform_;
};

class GraphicsScene {
  public:
    GraphicsScene(std::shared_ptr<TLAS> tlas,
                  std::vector<std::shared_ptr<GraphicsObject>> objects,
                  std::shared_ptr<DescriptorSet> scene_descriptor)
        : tlas_(tlas), objects_(objects), scene_descriptor_(scene_descriptor) {}

    std::shared_ptr<TLAS> tlas_;
    std::vector<std::shared_ptr<GraphicsObject>> objects_;
    std::shared_ptr<DescriptorSet> scene_descriptor_ = nullptr;

    std::vector<std::shared_ptr<GraphicsModel>> assemble_models_in_order() {
	std::unordered_map<std::shared_ptr<GraphicsModel>, size_t> referenced_models;
	for (const auto &object : objects_) {
	    if (!referenced_models.contains(object->model_)) {
		referenced_models.emplace(object->model_, referenced_models.size());
	    }
	}
	std::vector<std::shared_ptr<GraphicsModel>> models(referenced_models.size(), nullptr);
        for (auto &&[model, idx] : referenced_models) {
	    models.at(idx) = std::move(model);
	}
	return models;
    }
};

GraphicsContext::GraphicsContext() {
    window_ = std::make_shared<Window>();
    device_ = std::make_shared<Device>(window_);
    gpu_allocator_ = std::make_shared<GPUAllocator>(device_);
    command_pool_ = std::make_shared<CommandPool>(device_);
    descriptor_allocator_ = std::make_shared<DescriptorAllocator>(device_);
    swapchain_ =
        std::make_shared<Swapchain>(device_, window_, descriptor_allocator_);
    ring_buffer_ =
        std::make_shared<RingBuffer>(gpu_allocator_, command_pool_, 1 << 24);

    DescriptorSetBuilder scene_builder(descriptor_allocator_);
    scene_builder.bind_acceleration_structure(0, {}, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    scene_builder.bind_images(1, {}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                            VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

    unloaded_chunks_buffer_ = std::make_shared<GPUBuffer>(gpu_allocator_, MAX_NUM_CHUNKS_LOADED_PER_FRAME * sizeof(uint64_t), sizeof(uint64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    unloaded_chunks_span_ = unloaded_chunks_buffer_->cpu_map();
    uint64_t *crb = reinterpret_cast<uint64_t *>(unloaded_chunks_span_.data());
    for (uint32_t i = 0; i < MAX_NUM_CHUNKS_LOADED_PER_FRAME; ++i) {
	crb[i] = 0xFFFFFFFF;
    }

    DescriptorSetBuilder wide_builder(descriptor_allocator_);
    wide_builder.bind_buffer(0, {unloaded_chunks_buffer_->get_buffer(), 0, unloaded_chunks_buffer_->get_size()}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    wide_descriptor_ = wide_builder.build();

    auto rgen = std::make_shared<Shader>(device_, "rgen");
    auto rmiss = std::make_shared<Shader>(device_, "rmiss");
    auto unloaded_rchit = std::make_shared<Shader>(device_, "Unloaded_rchit");
    auto unloaded_rint = std::make_shared<Shader>(device_, "Unloaded_rint");
    auto raw_color_rchit = std::make_shared<Shader>(device_, "Raw_Color_rchit");
    auto raw_color_rint = std::make_shared<Shader>(device_, "Raw_Color_rint");
    std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups = {
        {rgen}, {rmiss}, {unloaded_rchit, unloaded_rint}, {raw_color_rchit, raw_color_rint}};
    std::vector<VkDescriptorSetLayout> layouts = {
        swapchain_->get_image_descriptor(0)->get_layout(),
        scene_builder.get_layout()->get_layout(),
        wide_builder.get_layout()->get_layout()};

    ray_trace_pipeline_ = std::make_shared<RayTracePipeline>(
        gpu_allocator_, shader_groups, layouts);
    frame_fence_ = std::make_shared<Fence>(device_);
    acquire_semaphore_ = std::make_shared<Semaphore>(device_);
    render_semaphore_ = std::make_shared<Semaphore>(device_);
    render_command_buffer_ = std::make_shared<Command>(command_pool_);
}

GraphicsContext::~GraphicsContext() {
    vkDeviceWaitIdle(device_->get_device());
    unloaded_chunks_buffer_->cpu_unmap();
}

std::shared_ptr<GraphicsContext> create_graphics_context() {
    auto context = std::shared_ptr<GraphicsContext>(new GraphicsContext());
    return context;
}

void render_frame(std::shared_ptr<GraphicsContext> context,
                  std::shared_ptr<GraphicsScene> scene) {
    std::vector<std::shared_ptr<Semaphore>> render_wait_semaphores {context->acquire_semaphore_};
    const bool changed_scenes = context->current_scene_ != scene;
    if (changed_scenes) {
	std::shared_ptr<Semaphore> tlas_semaphore = scene->tlas_->get_timeline();
        tlas_semaphore->set_wait_value(TLAS::TLAS_BUILD_TIMELINE);
	render_wait_semaphores.emplace_back(std::move(tlas_semaphore));
	for (auto object : scene->objects_) {
	    std::shared_ptr<Semaphore> upload_semaphore = object->model_->chunk_->get_timeline();
	    if (upload_semaphore != nullptr) {
		render_wait_semaphores.emplace_back(std::move(upload_semaphore));
	    }
	}
        context->current_scene_ = scene;
    }

    context->window_->pollEvents();

    context->frame_fence_->wait();

    if (!changed_scenes) {
	context->check_chunk_request_buffer(render_wait_semaphores);
    }
    
    context->frame_fence_->reset();

    const uint32_t swapchain_image_index =
        context->swapchain_->acquire_next_image(context->acquire_semaphore_);

    Barrier prologue_barrier(
        context->device_, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        context->swapchain_->get_image(swapchain_image_index),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Barrier epilogue_barrier(
        context->device_, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT,
        context->swapchain_->get_image(swapchain_image_index),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    std::vector<std::byte> push_constants(128);
    memcpy(push_constants.data(), &context->elapsed_ms_, sizeof(double));
    std::span<std::byte> push_constants_span(push_constants.data(),
                                             push_constants.size());

    context->render_command_buffer_->record(
        [&](VkCommandBuffer command_buffer) {
            VkExtent2D extent = context->swapchain_->get_extent();

            prologue_barrier.record(command_buffer);
            context->ray_trace_pipeline_->record(
                command_buffer,
                {context->swapchain_->get_image_descriptor(
                     swapchain_image_index),
                 scene->scene_descriptor_, context->wide_descriptor_},
                push_constants_span, extent.width, extent.height, 1);
            epilogue_barrier.record(command_buffer);
        });

    context->device_->submit_command(
				     context->render_command_buffer_,
				     render_wait_semaphores,
				     {context->render_semaphore_}, context->frame_fence_);
    
    context->swapchain_->present_image(swapchain_image_index,
                                       context->render_semaphore_);

    const auto current_time = std::chrono::system_clock::now();
    if (context->frame_index_ == 0) {
        context->start_time_ = current_time;
        context->start_frame_ = 0;
        context->first_time_ = current_time;
    } else if (std::chrono::duration_cast<std::chrono::nanoseconds>(
                   current_time - context->start_time_)
                   .count() >= 1000000000) {
        const uint64_t num_frames =
            context->frame_index_ - context->start_frame_;
        std::cout << "INFO: " << num_frames << " FPS\n";

        context->start_time_ = current_time;
        context->start_frame_ = context->frame_index_;
    }
    context->elapsed_ms_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               current_time - context->first_time_)
                               .count() /
                           1000000;
    ++context->frame_index_;
}

bool should_exit(std::shared_ptr<GraphicsContext> context) {
    return context->window_->shouldClose();
}

std::shared_ptr<GraphicsModel>
build_model(std::shared_ptr<GraphicsContext> context, VoxelChunkPtr chunk) {
    std::vector<VkAabbPositionsKHR> aabbs = {
        {0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}};
    std::shared_ptr<BLAS> blas =
        std::make_shared<BLAS>(context->gpu_allocator_, context->command_pool_,
                               context->ring_buffer_, aabbs);
    uint32_t sbt_offset = FORMAT_TO_SBT_OFFSET.at({chunk->get_format(), chunk->get_attribute_set()});
    return std::make_shared<GraphicsModel>(blas, chunk, sbt_offset);
}

std::shared_ptr<GraphicsObject>
build_object(std::shared_ptr<GraphicsContext> context,
             std::shared_ptr<GraphicsModel> model,
             const glm::mat3x4 &transform) {
    return std::make_shared<GraphicsObject>(model, transform);
}

static_assert(sizeof(VkTransformMatrixKHR) == sizeof(glm::mat3x4));
std::shared_ptr<GraphicsScene>
build_scene(std::shared_ptr<GraphicsContext> context,
            const std::vector<std::shared_ptr<GraphicsObject>> &objects) {
    std::vector<std::shared_ptr<BLAS>> bottom_structures;
    std::unordered_map<std::shared_ptr<GraphicsModel>, size_t>
        referenced_models;
    for (const auto &object : objects) {
        bottom_structures.emplace_back(object->model_->blas_);
	if (!referenced_models.contains(object->model_)) {
	    referenced_models.emplace(object->model_, referenced_models.size());
	}
    }
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    for (const auto &object : objects) {
        VkTransformMatrixKHR transform{};
        memcpy(&transform, &object->transform_, sizeof(VkTransformMatrixKHR));
        instances.emplace_back(transform, referenced_models.at(object->model_),
                               0xFF, UNLOADED_SBT_OFFSET, 0,
                               object->model_->blas_->get_device_address());
    }
    std::shared_ptr<TLAS> tlas = std::make_shared<TLAS>(
        context->gpu_allocator_, context->command_pool_, context->ring_buffer_,
        bottom_structures, instances);
    VkAccelerationStructureKHR vk_tlas = tlas->get_tlas();
    DescriptorSetBuilder builder(context->descriptor_allocator_);
    builder.bind_acceleration_structure(0,
                                        {
                                            .accelerationStructureCount = 1,
                                            .pAccelerationStructures = &vk_tlas,
                                        },
                                        VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    std::vector<std::pair<VkDescriptorImageInfo, uint32_t>> image_infos;
    for (auto [model, idx] : referenced_models) {
	if (model->chunk_->get_state() == VoxelChunk::State::GPU) {
	    auto volume = model->chunk_->get_gpu_volume();
	    VkDescriptorImageInfo image_info{};
	    image_info.imageView = volume->get_view();
	    image_info.sampler = VK_NULL_HANDLE;
	    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	    image_infos.emplace_back(image_info, idx);
	}
    }
    builder.bind_images(1, image_infos, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                            VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

    auto scene_descriptor = builder.build();
    return std::make_shared<GraphicsScene>(tlas, objects, scene_descriptor);
}

void GraphicsContext::check_chunk_request_buffer(std::vector<std::shared_ptr<Semaphore>> &render_wait_semaphores) {
	uint64_t *crb = reinterpret_cast<uint64_t *>(unloaded_chunks_span_.data());
	std::unordered_map<uint64_t, uint32_t> deduplicated_requests;
	std::vector<std::shared_ptr<GraphicsModel>> scene_models;
	for (uint32_t i = 0; i < MAX_NUM_CHUNKS_LOADED_PER_FRAME; ++i) {
	    if (crb[i] != 0xFFFFFFFF) {
		if (scene_models.empty()) {
		    scene_models = current_scene_->assemble_models_in_order();
		}
		deduplicated_requests.emplace(crb[i], scene_models.at(crb[i])->sbt_offset_);
		crb[i] = 0xFFFFFFFF;
	    }
	}
	if (!deduplicated_requests.empty()) {
	    for (auto [model_idx, _] : deduplicated_requests) {
		auto &model = scene_models.at(model_idx);
		if (model->chunk_->get_state() == VoxelChunk::State::CPU) {
		    model->chunk_->queue_gpu_upload(device_, gpu_allocator_, ring_buffer_);
		    render_wait_semaphores.emplace_back(model->chunk_->get_timeline());
		} else {
		    std::cout << "WARNING: A voxel chunk whose data is already resident in GPU memory was found as unloaded.\n";
		    std::cout << "The chunk has width " << model->chunk_->get_width() << ", height " << model->chunk_->get_height() << ", and depth " << model->chunk_->get_depth();
		}
	    }
	    current_scene_->tlas_->update_model_sbt_offsets(std::move(deduplicated_requests));
	    render_wait_semaphores.emplace_back(scene->tlas_->get_timeline());

	    DescriptorSetBuilder builder(descriptor_allocator_);
	    VkAccelerationStructureKHR vk_tlas = scene->tlas_->get_tlas();
	    builder.bind_acceleration_structure(0,
						{
						    .accelerationStructureCount = 1,
						    .pAccelerationStructures = &vk_tlas,
						},
						VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	    
	    std::vector<std::pair<VkDescriptorImageInfo, uint32_t>> image_infos;
	    for (size_t i = 0; i < scene_models.size(); ++i) {
		const auto &model = scene_models.at(i);
		if (model->chunk_->get_state() == VoxelChunk::State::GPU) {
		    auto volume = model->chunk_->get_gpu_volume();
		    VkDescriptorImageInfo image_info{};
		    image_info.imageView = volume->get_view();
		    image_info.sampler = VK_NULL_HANDLE;
		    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		    image_infos.emplace_back(image_info, i);
		}
	    }
	    builder.bind_images(1, image_infos, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
				VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
	    builder.update(scene->scene_descriptor_);
	}
}
