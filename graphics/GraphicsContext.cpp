#include <array>
#include <chrono>
#include <cstring>
#include <map>

#include "Command.h"
#include "Descriptor.h"
#include "Device.h"
#include "ExternalImage.h"
#include "GPUAllocator.h"
#include "Geometry.h"
#include "GraphicsContext.h"
#include "Pipeline.h"
#include "RingBuffer.h"
#include "Swapchain.h"
#include "Synchronization.h"
#include "Window.h"

const std::map<std::pair<VoxelChunk::Format, VoxelChunk::AttributeSet>,
               uint32_t>
    FORMAT_TO_SBT_OFFSET = {
        {{VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Color}, 1},
        {{VoxelChunk::Format::SVO, VoxelChunk::AttributeSet::Color}, 2},
        {{VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color}, 3},
        {{VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Emissive}, 4},
};

const uint32_t MAX_NUM_CHUNKS_LOADED_PER_FRAME = 32;

class GraphicsContext {
  public:
    GraphicsContext() = delete;
    GraphicsContext(std::shared_ptr<Window> window);
    ~GraphicsContext();

    std::shared_ptr<Device> device_ = nullptr;
    std::shared_ptr<GPUAllocator> gpu_allocator_ = nullptr;
    std::shared_ptr<Swapchain> swapchain_ = nullptr;
    std::shared_ptr<CommandPool> command_pool_ = nullptr;
    std::shared_ptr<RayTracePipeline> ray_trace_pipeline_ = nullptr;
    std::shared_ptr<RayTracePipeline> download_ray_trace_pipeline_ = nullptr;
    std::shared_ptr<DescriptorAllocator> descriptor_allocator_ = nullptr;
    std::shared_ptr<RingBuffer> ring_buffer_ = nullptr;

    std::shared_ptr<ComputePipeline> tonemap_pipeline_ = nullptr;
    std::vector<std::shared_ptr<DescriptorSet>> tonemap_descriptors_; // Needed per swapchain image?

    std::shared_ptr<Fence> frame_fence_ = nullptr;
    std::shared_ptr<Fence> download_fence_ = nullptr;
    std::shared_ptr<Semaphore> acquire_semaphore_ = nullptr;
    std::shared_ptr<Semaphore> render_semaphore_ = nullptr;
    std::shared_ptr<Command> render_command_buffer_ = nullptr;
    std::shared_ptr<Command> download_command_buffer_ = nullptr;

    std::shared_ptr<GPUBuffer> camera_buffer_ = nullptr;
    std::span<std::byte> camera_span_;

    std::shared_ptr<GPUBuffer> chunk_request_buffer_ = nullptr;
    std::span<std::byte> chunk_request_span_;

    std::shared_ptr<GPUBuffer> chunk_download_buffer_ = nullptr;
    std::span<std::byte> chunk_download_span_;

    std::shared_ptr<GPUImage> blue_noise_ = nullptr;

    std::shared_ptr<GPUImage> image_history_ = nullptr;
    std::shared_ptr<GPUImage> image_normals_ = nullptr;
    std::shared_ptr<GPUImage> image_positions_ = nullptr;

    std::shared_ptr<DescriptorSet> wide_descriptor_ = nullptr;

    std::shared_ptr<GraphicsScene> current_scene_ = nullptr;

    uint64_t frame_index_ = 0;

    std::chrono::time_point<std::chrono::system_clock> start_time_;
    uint64_t start_frame_ = 0;

    std::chrono::time_point<std::chrono::system_clock> first_time_;
    uint64_t elapsed_ms_;

    std::chrono::time_point<std::chrono::system_clock> last_time_;

    std::unordered_map<uint64_t, uint32_t> uploading_models_;
    std::unordered_set<uint64_t> downloading_models_;

    void check_chunk_request_buffer(
        std::vector<std::shared_ptr<Semaphore>> &render_wait_semaphores);

    void tick_download_models(std::vector<std::shared_ptr<Semaphore>> &render_wait_semaphores,
			      std::shared_ptr<GraphicsScene> scene,
			      std::span<std::byte> push_constants_span);
    
    void bind_scene_descriptors(
        DescriptorSetBuilder &builder, std::shared_ptr<GraphicsScene> scene,
        std::vector<std::shared_ptr<GraphicsModel>> &&models);
};

class GraphicsModel {
  public:
    GraphicsModel(std::shared_ptr<BLAS> blas, VoxelChunkPtr chunk,
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
    GraphicsScene(std::shared_ptr<GraphicsContext> context,
                  std::shared_ptr<TLAS> tlas,
                  std::vector<std::shared_ptr<GraphicsObject>> objects,
                  std::shared_ptr<DescriptorSet> scene_descriptor,
		  std::shared_ptr<GPUBuffer> chunk_dimensions)
        : tlas_(tlas), objects_(objects), scene_descriptor_(scene_descriptor), chunk_dimensions_(chunk_dimensions) {}

    std::shared_ptr<TLAS> tlas_;
    std::vector<std::shared_ptr<GraphicsObject>> objects_;
    std::shared_ptr<DescriptorSet> scene_descriptor_ = nullptr;
    std::shared_ptr<GPUBuffer> chunk_dimensions_ = nullptr;

    std::vector<std::shared_ptr<GraphicsModel>> assemble_models_in_order() {
        std::unordered_map<std::shared_ptr<GraphicsModel>, size_t>
            referenced_models;
        for (const auto &object : objects_) {
            if (!referenced_models.contains(object->model_)) {
                referenced_models.emplace(object->model_,
                                          referenced_models.size());
            }
        }
        std::vector<std::shared_ptr<GraphicsModel>> models(
            referenced_models.size(), nullptr);
        for (auto &&[model, idx] : referenced_models) {
            models.at(idx) = std::move(model);
        }
        return models;
    }
};

GraphicsContext::GraphicsContext(std::shared_ptr<Window> window) {
    device_ = std::make_shared<Device>(window);
    gpu_allocator_ = std::make_shared<GPUAllocator>(device_);
    command_pool_ = std::make_shared<CommandPool>(device_);
    descriptor_allocator_ = std::make_shared<DescriptorAllocator>(device_);
    swapchain_ =
        std::make_shared<Swapchain>(device_, window, descriptor_allocator_);
    ring_buffer_ =
        std::make_shared<RingBuffer>(gpu_allocator_, command_pool_, 1 << 27);

    DescriptorSetBuilder scene_builder(descriptor_allocator_);
    scene_builder.bind_acceleration_structure(0, {},
                                              VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    scene_builder.bind_images(1, {}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                              VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                  VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    scene_builder.bind_buffers(2, {}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                   VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    scene_builder.bind_buffer(3, {}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			      VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
			      VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    
    chunk_request_buffer_ = std::make_shared<GPUBuffer>(
        gpu_allocator_, MAX_NUM_CHUNKS_LOADED_PER_FRAME * sizeof(uint64_t),
        sizeof(uint64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    chunk_request_span_ = chunk_request_buffer_->cpu_map();
    uint64_t *crb = reinterpret_cast<uint64_t *>(chunk_request_span_.data());
    for (uint32_t i = 0; i < MAX_NUM_CHUNKS_LOADED_PER_FRAME; ++i) {
        crb[i] = 0x00000000FFFFFFFF;
    }

    chunk_download_buffer_ = std::make_shared<GPUBuffer>(
        gpu_allocator_, MAX_NUM_CHUNKS_LOADED_PER_FRAME * sizeof(uint32_t),
        sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    chunk_download_span_ = chunk_download_buffer_->cpu_map();
    uint32_t *cdb = reinterpret_cast<uint32_t *>(chunk_download_span_.data());
    for (uint32_t i = 0; i < MAX_MODELS; ++i) {
        cdb[i] = 0;
    }

    camera_buffer_ = std::make_shared<GPUBuffer>(
        gpu_allocator_, sizeof(CameraUB), sizeof(glm::vec4),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    camera_span_ = camera_buffer_->cpu_map();

    image_history_ = std::make_shared<GPUImage>(
        gpu_allocator_, swapchain_->get_extent(), swapchain_->get_format(), 0,
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 1, 1);

    image_normals_ = std::make_shared<GPUImage>(
        gpu_allocator_, swapchain_->get_extent(), swapchain_->get_format(), 0,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 1, 1);

    image_positions_ = std::make_shared<GPUImage>(
        gpu_allocator_, swapchain_->get_extent(), swapchain_->get_format(), 0,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 1, 1);

    blue_noise_ = load_image(gpu_allocator_, ring_buffer_, "LDR_RGBA_0.png");

    DescriptorSetBuilder wide_builder(descriptor_allocator_);
    wide_builder.bind_buffer(0,
                             {.buffer = chunk_request_buffer_->get_buffer(),
                              .offset = 0,
                              .range = chunk_request_buffer_->get_size()},
                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                             VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    wide_builder.bind_buffer(1,
                             {.buffer = chunk_download_buffer_->get_buffer(),
                              .offset = 0,
                              .range = chunk_download_buffer_->get_size()},
                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                             VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    wide_builder.bind_buffer(2,
                             {.buffer = camera_buffer_->get_buffer(),
                              .offset = 0,
                              .range = camera_buffer_->get_size()},
                             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    wide_builder.bind_image(3,
                            {.sampler = VK_NULL_HANDLE,
                             .imageView = blue_noise_->get_view(),
                             .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                            VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    wide_builder.bind_image(4,
                            {.sampler = VK_NULL_HANDLE,
                             .imageView = image_history_->get_view(),
                             .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                            VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    wide_builder.bind_image(5,
                            {.sampler = VK_NULL_HANDLE,
                             .imageView = image_normals_->get_view(),
                             .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                            VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    wide_builder.bind_image(6,
                            {.sampler = VK_NULL_HANDLE,
                             .imageView = image_positions_->get_view(),
                             .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                            VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    wide_descriptor_ = wide_builder.build();

    auto rgen = std::make_shared<Shader>(device_, "rgen");
    auto rmiss = std::make_shared<Shader>(device_, "rmiss");
    auto unloaded_rchit = std::make_shared<Shader>(device_, "Unloaded_rchit");
    auto unloaded_rint = std::make_shared<Shader>(device_, "Unloaded_rint");
    auto raw_color_rchit = std::make_shared<Shader>(device_, "Raw_Color_rchit");
    auto raw_color_rint = std::make_shared<Shader>(device_, "Raw_Color_rint");
    auto svo_color_rchit = std::make_shared<Shader>(device_, "SVO_Color_rchit");
    auto svo_color_rint = std::make_shared<Shader>(device_, "SVO_Color_rint");
    auto svdag_color_rchit = std::make_shared<Shader>(device_, "SVDAG_Color_rchit");
    auto svdag_color_rint = std::make_shared<Shader>(device_, "SVDAG_Color_rint");
    auto emissive_rchit = std::make_shared<Shader>(device_, "Emissive_rchit");
    auto download_rchit = std::make_shared<Shader>(device_, "Download_rchit");
    std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups = {
        {rgen},
        {rmiss},
        {unloaded_rchit, unloaded_rint},
        {raw_color_rchit, raw_color_rint},
        {svo_color_rchit, svo_color_rint},
        {svdag_color_rchit, svdag_color_rint},
        {emissive_rchit, raw_color_rint},
    };
    std::vector<std::vector<std::shared_ptr<Shader>>> download_shader_groups = {
        {rgen},
        {rmiss},
        {unloaded_rchit, unloaded_rint},
        {download_rchit, raw_color_rint},
        {download_rchit, svo_color_rint},
        {download_rchit, svdag_color_rint},
        {download_rchit, raw_color_rint},
    };
    std::vector<VkDescriptorSetLayout> layouts = {
        swapchain_->get_image_descriptor(0)->get_layout(),
        scene_builder.get_layout()->get_layout(),
        wide_builder.get_layout()->get_layout()};

    ray_trace_pipeline_ = std::make_shared<RayTracePipeline>(
        gpu_allocator_, shader_groups, layouts);
    download_ray_trace_pipeline_ = std::make_shared<RayTracePipeline>(
        gpu_allocator_, download_shader_groups, layouts);
    frame_fence_ = std::make_shared<Fence>(device_);
    download_fence_ = std::make_shared<Fence>(device_);
    acquire_semaphore_ = std::make_shared<Semaphore>(device_);
    render_semaphore_ = std::make_shared<Semaphore>(device_);
    render_command_buffer_ = std::make_shared<Command>(command_pool_);
    download_command_buffer_ = std::make_shared<Command>(command_pool_);

    auto tonemap_comp = std::make_shared<Shader>(device_, "tonemap_comp");
    const auto &image_views = swapchain_->get_image_views();
    for (int image_index = 0; image_index < image_views.size(); image_index++) {
        DescriptorSetBuilder tonemap_builder(descriptor_allocator_);
        tonemap_builder.bind_image(0,
                                   {.sampler = VK_NULL_HANDLE,
                                    .imageView = image_views[image_index],
                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                                   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                   VK_SHADER_STAGE_COMPUTE_BIT);
        tonemap_builder.bind_image(1,
                                   {.sampler = VK_NULL_HANDLE,
                                    .imageView = image_history_->get_view(),
                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                                   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                   VK_SHADER_STAGE_COMPUTE_BIT);
        tonemap_builder.bind_image(2,
                                   {.sampler = VK_NULL_HANDLE,
                                    .imageView = image_normals_->get_view(),
                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                                   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                   VK_SHADER_STAGE_COMPUTE_BIT);
        tonemap_builder.bind_image(3,
                                   {.sampler = VK_NULL_HANDLE,
                                    .imageView = image_positions_->get_view(),
                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
                                   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                   VK_SHADER_STAGE_COMPUTE_BIT);
        tonemap_descriptors_.push_back(tonemap_builder.build());
    }

    std::vector<VkDescriptorSetLayout> tonemap_layouts = {
        tonemap_descriptors_[0]->get_layout()};

    tonemap_pipeline_ = std::make_shared<ComputePipeline>(
        gpu_allocator_, tonemap_comp, tonemap_layouts);
}

GraphicsContext::~GraphicsContext() {
    vkDeviceWaitIdle(device_->get_device());
    chunk_request_buffer_->cpu_unmap();
    chunk_download_buffer_->cpu_unmap();
    camera_buffer_->cpu_unmap();
}

std::shared_ptr<GraphicsContext>
create_graphics_context(std::shared_ptr<Window> window) {
    auto context =
        std::shared_ptr<GraphicsContext>(new GraphicsContext(window));
    return context;
}

double render_frame(std::shared_ptr<GraphicsContext> context,
                    std::shared_ptr<GraphicsScene> scene,
                    glm::vec3 camera_position, glm::vec3 camera_front,
                    CameraUB camera_info) {
    context->frame_fence_->wait();
    context->frame_fence_->reset();

    std::vector<std::shared_ptr<Semaphore>> render_wait_semaphores{
        context->acquire_semaphore_};
    const bool changed_scenes = context->current_scene_ != scene;
    if (changed_scenes) {
        std::shared_ptr<Semaphore> tlas_semaphore =
            scene->tlas_->get_timeline();
        render_wait_semaphores.emplace_back(std::move(tlas_semaphore));
        for (auto object : scene->objects_) {
            std::shared_ptr<Semaphore> upload_semaphore =
                object->model_->chunk_->get_timeline();
            if (upload_semaphore != nullptr) {
                render_wait_semaphores.emplace_back(
                    std::move(upload_semaphore));
            }
        }
        context->current_scene_ = scene;
    }

    uint32_t download_bit = 1;
    std::vector<std::byte> push_constants(128, std::byte{0});
    memcpy(push_constants.data(), &context->elapsed_ms_, sizeof(double));
    memcpy(push_constants.data() + sizeof(double), &download_bit, sizeof(uint32_t));
    std::span<std::byte> push_constants_span(push_constants.data(),
                                             push_constants.size());

    memcpy(context->camera_span_.data(), &camera_info, sizeof(CameraUB));

    if (!changed_scenes) {
        context->check_chunk_request_buffer(render_wait_semaphores);
	context->tick_download_models(render_wait_semaphores, scene, push_constants_span);
        if (context->frame_index_ % 512 == 0) {
            context->ring_buffer_->reap_in_flight_copies();
        }
    }

    download_bit = 0;
    memcpy(push_constants.data() + sizeof(double), &download_bit, sizeof(uint32_t));

    // To-do: histories should be recreated if the swapchain was recreated.
    // Alternatively, we can size the history to the max size somehow
    // and exclusively sample within some subimage of the history.
    const uint32_t swapchain_image_index =
        context->swapchain_->acquire_next_image(context->acquire_semaphore_);

    // Todo: is there a better way to transition these images?
    Barrier prologue_barrier0(
        context->device_, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_2_SHADER_WRITE_BIT, context->image_history_->get_image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Barrier prologue_barrier1(
        context->device_, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_2_SHADER_WRITE_BIT, context->image_normals_->get_image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Barrier prologue_barrier2(
        context->device_, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_2_SHADER_WRITE_BIT, context->image_positions_->get_image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Barrier tonemap_barrier(
        context->device_, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
        context->swapchain_->get_image(swapchain_image_index),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Barrier epilogue_barrier(
        context->device_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_ACCESS_2_MEMORY_READ_BIT,
        context->swapchain_->get_image(swapchain_image_index),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    context->render_command_buffer_->record([&](VkCommandBuffer
                                                    command_buffer) {
        VkExtent2D extent = context->swapchain_->get_extent();

        // Make sure to prepare the images and clear the previous frame.
        prologue_barrier0.record(command_buffer);
        prologue_barrier1.record(command_buffer);
        prologue_barrier2.record(command_buffer);

        VkClearColorValue clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
        VkImageSubresourceRange range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        vkCmdClearColorImage(command_buffer,
                             context->image_normals_->get_image(),
                             VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
        vkCmdClearColorImage(command_buffer,
                             context->image_positions_->get_image(),
                             VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);

        // Render to the off-screen buffer
        context->ray_trace_pipeline_->record(
            command_buffer,
            {context->swapchain_->get_image_descriptor(
                 swapchain_image_index), // this is redundant for the time being
             scene->scene_descriptor_, context->wide_descriptor_},
            push_constants_span, extent.width, extent.height, 1);

        // Copy the off-screen buffer to the swapchain
        tonemap_barrier.record(command_buffer);

        int compute_width = (extent.width + 16 - 1) / 16;
        int compute_height = (extent.height + 16 - 1) / 16;

        context->tonemap_pipeline_->record(
            command_buffer,
            {context->tonemap_descriptors_[swapchain_image_index]},
            compute_width, compute_height, 1);

        epilogue_barrier.record(command_buffer);
    });

    context->device_->submit_command(
        context->render_command_buffer_, render_wait_semaphores,
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
        std::cout << "INFO: " << num_frames << " FPS   "
                  << context->gpu_allocator_->bookkeep_count() << " BYTES\n";

        context->start_time_ = current_time;
        context->start_frame_ = context->frame_index_;
    }
    context->elapsed_ms_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               current_time - context->first_time_)
                               .count() /
                           1000000;
    const auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        current_time - context->last_time_)
                        .count() /
                    1000000.0;
    context->last_time_ = current_time;
    ++context->frame_index_;
    return dt;
}

std::shared_ptr<GraphicsModel>
build_model(std::shared_ptr<GraphicsContext> context, VoxelChunkPtr chunk) {
    std::vector<VkAabbPositionsKHR> aabbs = {
        {0.0F, 0.0F, 0.0F, static_cast<float>(chunk->get_width()),
         static_cast<float>(chunk->get_height()),
         static_cast<float>(chunk->get_depth())}};
    std::shared_ptr<BLAS> blas =
        std::make_shared<BLAS>(context->gpu_allocator_, context->command_pool_,
                               context->ring_buffer_, aabbs);
    uint32_t sbt_offset = FORMAT_TO_SBT_OFFSET.at(
        {chunk->get_format(), chunk->get_attribute_set()});
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
	auto model_idx = referenced_models.at(object->model_);
	bool model_on_gpu = object->model_->chunk_->get_state() == VoxelChunk::State::GPU && !context->uploading_models_.contains(model_idx) && !context->downloading_models_.contains(model_idx);
        instances.emplace_back(transform, model_idx,
                               0xFF, model_on_gpu ? object->model_->sbt_offset_ : UNLOADED_SBT_OFFSET, 0,
                               object->model_->blas_->get_device_address());
    }
    std::shared_ptr<TLAS> tlas = std::make_shared<TLAS>(
        context->gpu_allocator_, context->command_pool_, context->ring_buffer_,
        bottom_structures, instances);

    DescriptorSetBuilder builder(context->descriptor_allocator_);
    std::vector<std::shared_ptr<GraphicsModel>> scene_models(
        referenced_models.size());
    for (auto &[model, idx] : referenced_models) {
        scene_models.at(idx) = std::move(model);
    }

    std::shared_ptr<GPUBuffer> chunk_dimensions = std::make_shared<GPUBuffer>(
									      context->gpu_allocator_, MAX_MODELS * sizeof(uint32_t) * 3,
									      sizeof(uint64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
									      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
									      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    auto chunk_dimensions_span = chunk_dimensions->cpu_map();
    uint32_t *chunk_dimensions_ptr = reinterpret_cast<uint32_t *>(chunk_dimensions_span.data());
    for (uint32_t i = 0; i < scene_models.size(); ++i) {
	chunk_dimensions_ptr[i * 3] = scene_models[i]->chunk_->get_width();
	chunk_dimensions_ptr[i * 3 + 1] = scene_models[i]->chunk_->get_height();
	chunk_dimensions_ptr[i * 3 + 2] = scene_models[i]->chunk_->get_depth();
    }
    chunk_dimensions->cpu_unmap();
    
    auto scene =
        std::make_shared<GraphicsScene>(context, tlas, objects, nullptr, chunk_dimensions);
    context->bind_scene_descriptors(builder, scene, std::move(scene_models));
    scene->scene_descriptor_ = builder.build();
    return scene;
}

void GraphicsContext::check_chunk_request_buffer(
    std::vector<std::shared_ptr<Semaphore>> &render_wait_semaphores) {
    uint32_t *crb = reinterpret_cast<uint32_t *>(chunk_request_span_.data());
    std::unordered_map<uint64_t, uint32_t> deduplicated_requests;
    std::vector<std::shared_ptr<GraphicsModel>> scene_models =
        current_scene_->assemble_models_in_order();
    for (uint32_t i = 0; i < MAX_NUM_CHUNKS_LOADED_PER_FRAME; ++i) {
	uint32_t num_rays = 2 * i;
	uint32_t crb_model = 2 * i + 1;
        if (crb[crb_model] != 0xFFFFFFFF && crb[num_rays] >= 128) {
            deduplicated_requests.emplace(crb[crb_model],
                                          scene_models.at(crb[crb_model])->sbt_offset_);
        }
	crb[num_rays] = 0;
	crb[crb_model] = 0xFFFFFFFF;
    }
    for (auto [model_idx, sbt_offset] : deduplicated_requests) {
        if (!uploading_models_.contains(model_idx) &&
            !downloading_models_.contains(model_idx)) {
            uploading_models_.emplace(model_idx, sbt_offset);
        }
    }

    deduplicated_requests.clear();
    for (auto [model_idx, sbt_offset] : uploading_models_) {
        auto &model = scene_models.at(model_idx);
	model->chunk_->tick_gpu_upload(device_, gpu_allocator_,
				       ring_buffer_);
        if (!model->chunk_->uploading()) {
            deduplicated_requests.emplace(model_idx, sbt_offset);
        }
    }
    for (auto [model_idx, _] : deduplicated_requests) {
        uploading_models_.erase(model_idx);
    }

    if (!deduplicated_requests.empty()) {
        current_scene_->tlas_->update_model_sbt_offsets(
            std::move(deduplicated_requests));
        render_wait_semaphores.emplace_back(
            current_scene_->tlas_->get_timeline());

        DescriptorSetBuilder builder(descriptor_allocator_);
        bind_scene_descriptors(builder, current_scene_,
                               std::move(scene_models));
        builder.update(current_scene_->scene_descriptor_);
    }
}

void GraphicsContext::tick_download_models(std::vector<std::shared_ptr<Semaphore>> &render_wait_semaphores,
					   std::shared_ptr<GraphicsScene> scene,
					   std::span<std::byte> push_constants_span) {
    std::vector<std::shared_ptr<GraphicsModel>> scene_models =
	current_scene_->assemble_models_in_order();
    std::erase_if(downloading_models_, 
		  [&](uint64_t model_idx) { return !scene_models.at(model_idx)->chunk_->downloading(); });

    if (frame_index_ % 128 == 0) {
	download_fence_->reset();
	memset(chunk_download_span_.data(), 0, chunk_download_span_.size());

	download_command_buffer_->record([&](VkCommandBuffer command_buffer) {
	    VkExtent2D extent = swapchain_->get_extent();
	    download_ray_trace_pipeline_->record(
						 command_buffer,
						 {swapchain_->get_image_descriptor(0), // this is redundant for the time being
						  scene->scene_descriptor_, wide_descriptor_},
						 push_constants_span, extent.width, extent.height, 1);
	});

	device_->submit_command(download_command_buffer_, {}, {}, download_fence_);

	download_fence_->wait();
	uint32_t *cdb = reinterpret_cast<uint32_t *>(chunk_download_span_.data());
	std::unordered_map<uint64_t, uint32_t> new_sbt_offsets;
	for (uint64_t model_idx = 0; model_idx < scene_models.size(); ++model_idx) {
	    auto &model = scene_models.at(model_idx);
	    if (!uploading_models_.contains(model_idx) &&
		!downloading_models_.contains(model_idx) &&
		cdb[model_idx] < 4 &&
		model->chunk_->get_state() == VoxelChunk::State::GPU) {
		downloading_models_.insert(model_idx);
		model->chunk_->tick_disk_download(device_, ring_buffer_);
		new_sbt_offsets.emplace(model_idx, 0);
	    }
	}
	
	if (!new_sbt_offsets.empty()) {
	    current_scene_->tlas_->update_model_sbt_offsets(std::move(new_sbt_offsets));
	    render_wait_semaphores.emplace_back(current_scene_->tlas_->get_timeline());
	    
	    DescriptorSetBuilder builder(descriptor_allocator_);
	    bind_scene_descriptors(builder, current_scene_,
				   std::move(scene_models));
	    builder.update(current_scene_->scene_descriptor_);
	}
    }
}

void GraphicsContext::bind_scene_descriptors(
    DescriptorSetBuilder &builder, std::shared_ptr<GraphicsScene> scene,
    std::vector<std::shared_ptr<GraphicsModel>> &&models) {
    builder.bind_acceleration_structure(
        0,
        {
            .accelerationStructureCount = 1,
            .pAccelerationStructures = &scene->tlas_->get_tlas(),
        },
        VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    std::vector<std::pair<VkDescriptorImageInfo, uint32_t>> raw_volume_infos;
    std::vector<std::pair<VkDescriptorBufferInfo, uint32_t>> sparse_buffer_infos;
    for (size_t i = 0; i < models.size(); ++i) {
        const auto &model = models.at(i);
        if (model->chunk_->get_state() == VoxelChunk::State::GPU &&
            model->chunk_->get_format() == VoxelChunk::Format::Raw) {
            auto volume = model->chunk_->get_gpu_volume();
            VkDescriptorImageInfo raw_volume_info{};
            raw_volume_info.imageView = volume->get_view();
            raw_volume_info.sampler = VK_NULL_HANDLE;
            raw_volume_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            raw_volume_infos.emplace_back(raw_volume_info, i);
        } else if (model->chunk_->get_state() == VoxelChunk::State::GPU &&
                   (model->chunk_->get_format() == VoxelChunk::Format::SVO ||
		    model->chunk_->get_format() == VoxelChunk::Format::SVDAG)) {
            auto buffer = model->chunk_->get_gpu_buffer();
            VkDescriptorBufferInfo sparse_buffer_info{};
            sparse_buffer_info.buffer = buffer->get_buffer();
            sparse_buffer_info.offset = 0;
            sparse_buffer_info.range = buffer->get_size();
            sparse_buffer_infos.emplace_back(sparse_buffer_info, i);
        }
    }
    builder.bind_images(1, raw_volume_infos, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                            VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    builder.bind_buffers(2, sparse_buffer_infos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                         VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                             VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

    VkDescriptorBufferInfo chunk_dimensions_info{};
    chunk_dimensions_info.buffer = scene->chunk_dimensions_->get_buffer();
    chunk_dimensions_info.offset = 0;
    chunk_dimensions_info.range = scene->chunk_dimensions_->get_size();
    builder.bind_buffer(3, chunk_dimensions_info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
			VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
}
