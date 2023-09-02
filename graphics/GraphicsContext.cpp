#include <cstring>

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

#include "voxels/RawVoxelChunk.h"

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
    std::shared_ptr<DescriptorSet> scene_descriptor_ = nullptr;
    std::shared_ptr<Fence> frame_fence_ = nullptr;
    std::shared_ptr<Semaphore> acquire_semaphore_ = nullptr;
    std::shared_ptr<Semaphore> render_semaphore_ = nullptr;
    std::shared_ptr<Command> render_command_buffer_ = nullptr;

    std::shared_ptr<GraphicsScene> current_scene_ = nullptr;

    uint64_t frame_index_ = 0;

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context,
                             std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel>
    build_model(std::shared_ptr<GraphicsContext> context,
                const RawVoxelChunk &chunk);
    friend std::shared_ptr<GraphicsObject>
    build_object(std::shared_ptr<GraphicsContext> context,
                 std::shared_ptr<GraphicsModel> model,
                 const glm::mat3x4 &transform);
    friend std::shared_ptr<GraphicsScene>
    build_scene(std::shared_ptr<GraphicsContext> context,
                const std::vector<std::shared_ptr<GraphicsObject>> &objects);
};

class GraphicsModel {
  public:
    GraphicsModel(std::shared_ptr<BLAS> blas,
                  std::shared_ptr<GPUVolume> raw_data)
        : blas_(blas), raw_data_(raw_data) {}

  private:
    std::shared_ptr<BLAS> blas_;
    std::shared_ptr<GPUVolume> raw_data_;

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context,
                             std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel>
    build_model(std::shared_ptr<GraphicsContext> context,
                const RawVoxelChunk &chunk);
    friend std::shared_ptr<GraphicsObject>
    build_object(std::shared_ptr<GraphicsContext> context,
                 std::shared_ptr<GraphicsModel> model,
                 const glm::mat3x4 &transform);
    friend std::shared_ptr<GraphicsScene>
    build_scene(std::shared_ptr<GraphicsContext> context,
                const std::vector<std::shared_ptr<GraphicsObject>> &objects);
};

class GraphicsObject {
  public:
    GraphicsObject(std::shared_ptr<GraphicsModel> model, glm::mat3x4 transform)
        : model_(model), transform_(transform) {}

  private:
    std::shared_ptr<GraphicsModel> model_;
    glm::mat3x4 transform_;

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context,
                             std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel>
    build_model(std::shared_ptr<GraphicsContext> context,
                const RawVoxelChunk &chunk);
    friend std::shared_ptr<GraphicsObject>
    build_object(std::shared_ptr<GraphicsContext> context,
                 std::shared_ptr<GraphicsModel> model,
                 const glm::mat3x4 &transform);
    friend std::shared_ptr<GraphicsScene>
    build_scene(std::shared_ptr<GraphicsContext> context,
                const std::vector<std::shared_ptr<GraphicsObject>> &objects);
};

class GraphicsScene {
  public:
    GraphicsScene(std::shared_ptr<TLAS> tlas,
                  std::vector<std::shared_ptr<GraphicsObject>> objects)
        : tlas_(tlas), objects_(objects) {}

  private:
    std::shared_ptr<TLAS> tlas_;
    std::vector<std::shared_ptr<GraphicsObject>> objects_;

    friend std::shared_ptr<GraphicsContext> create_graphics_context();
    friend void render_frame(std::shared_ptr<GraphicsContext> context,
                             std::shared_ptr<GraphicsScene> scene);
    friend bool should_exit(std::shared_ptr<GraphicsContext> context);
    friend std::shared_ptr<GraphicsModel>
    build_model(std::shared_ptr<GraphicsContext> context,
                const RawVoxelChunk &chunk);
    friend std::shared_ptr<GraphicsObject>
    build_object(std::shared_ptr<GraphicsContext> context,
                 std::shared_ptr<GraphicsModel> model,
                 const glm::mat3x4 &transform);
    friend std::shared_ptr<GraphicsScene>
    build_scene(std::shared_ptr<GraphicsContext> context,
                const std::vector<std::shared_ptr<GraphicsObject>> &objects);
};

GraphicsContext::GraphicsContext() {
    window_ = std::make_shared<Window>();
    device_ = std::make_shared<Device>(window_);
    gpu_allocator_ = std::make_shared<GPUAllocator>(device_);
    swapchain_ = std::make_shared<Swapchain>(device_, window_);
    command_pool_ = std::make_shared<CommandPool>(device_);
    descriptor_allocator_ = std::make_shared<DescriptorAllocator>(device_);
    ring_buffer_ =
        std::make_shared<RingBuffer>(gpu_allocator_, command_pool_, 1 << 24);

    swapchain_descriptors_ =
        swapchain_->make_image_descriptors(descriptor_allocator_);

    DescriptorSetBuilder builder(descriptor_allocator_);
    builder.bind_acceleration_structure(0, {}, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    builder.bind_image(1, {}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                       VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

    auto rgen = std::make_shared<Shader>(device_, "dumb_rgen");
    auto rmiss = std::make_shared<Shader>(device_, "dumb_rmiss");
    auto rchit = std::make_shared<Shader>(device_, "dumb_rchit");
    auto rint = std::make_shared<Shader>(device_, "dumb_rint");
    std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups = {
        {rgen}, {rmiss}, {rchit, rint}};
    std::vector<VkDescriptorSetLayout> layouts = {
        swapchain_descriptors_.at(0)->get_layout(),
        builder.get_layout()->get_layout()};

    ray_trace_pipeline_ = std::make_shared<RayTracePipeline>(
        gpu_allocator_, shader_groups, layouts);
    frame_fence_ = std::make_shared<Fence>(device_);
    acquire_semaphore_ = std::make_shared<Semaphore>(device_);
    render_semaphore_ = std::make_shared<Semaphore>(device_);
    render_command_buffer_ = std::make_shared<Command>(command_pool_);
}

GraphicsContext::~GraphicsContext() { vkDeviceWaitIdle(device_->get_device()); }

std::shared_ptr<GraphicsContext> create_graphics_context() {
    auto context = std::shared_ptr<GraphicsContext>(new GraphicsContext());
    return context;
}

void render_frame(std::shared_ptr<GraphicsContext> context,
                  std::shared_ptr<GraphicsScene> scene) {
    if (context->current_scene_ != scene) {
        vkDeviceWaitIdle(context->device_->get_device());
        VkAccelerationStructureKHR tlas = scene->tlas_->get_tlas();
        DescriptorSetBuilder builder(context->descriptor_allocator_);
        builder.bind_acceleration_structure(
            0,
            {
                .accelerationStructureCount = 1,
                .pAccelerationStructures = &tlas,
            },
            VK_SHADER_STAGE_RAYGEN_BIT_KHR);

        auto volume = scene->objects_.at(0)->model_->raw_data_;
        builder.bind_image(1,
                           {
                               .sampler = VK_NULL_HANDLE,
                               .imageView = volume->get_view(),
                               .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                           },
                           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                           VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

        context->scene_descriptor_ = builder.build();
        context->current_scene_ = scene;
    }

    context->window_->pollEvents();

    context->frame_fence_->wait();

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

    context->frame_fence_->reset();

    std::vector<std::byte> push_constants(128);
    memcpy(push_constants.data(), &context->frame_index_, sizeof(uint64_t));
    std::span<std::byte> push_constants_span(push_constants.data(),
                                             push_constants.size());

    context->render_command_buffer_->record(
        [&](VkCommandBuffer command_buffer) {
            VkExtent2D extent = context->swapchain_->get_extent();

            prologue_barrier.record(command_buffer);
            context->ray_trace_pipeline_->record(
                command_buffer,
                {context->swapchain_descriptors_.at(swapchain_image_index),
                 context->scene_descriptor_},
                push_constants_span, extent.width, extent.height, 1);
            epilogue_barrier.record(command_buffer);
        });

    context->device_->submit_command(
        context->render_command_buffer_, {context->acquire_semaphore_},
        {context->render_semaphore_}, context->frame_fence_);

    context->swapchain_->present_image(swapchain_image_index,
                                       context->render_semaphore_);

    ++context->frame_index_;
}

bool should_exit(std::shared_ptr<GraphicsContext> context) {
    return context->window_->shouldClose();
}

std::shared_ptr<GraphicsModel>
build_model(std::shared_ptr<GraphicsContext> context,
            const RawVoxelChunk &chunk) {
    std::vector<VkAabbPositionsKHR> aabbs = {
        {0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}};
    std::shared_ptr<BLAS> blas =
        std::make_shared<BLAS>(context->gpu_allocator_, context->command_pool_,
                               context->ring_buffer_, aabbs);
    VkExtent3D extent = {8, 8, 8};
    std::shared_ptr<GPUVolume> volume = std::make_shared<GPUVolume>(
        context->gpu_allocator_, extent, VK_FORMAT_R8G8B8A8_UNORM, 0,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 1, 1);
    std::vector<RawVoxel> voxels(chunk.voxels_.begin(), chunk.voxels_.end());
    std::span<RawVoxel> span(voxels.data(), voxels.size());
    context->ring_buffer_->copy_to_device(volume, VK_IMAGE_LAYOUT_GENERAL,
                                          std::as_bytes(span), {}, {});
    return std::make_shared<GraphicsModel>(blas, volume);
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
    for (const auto object : objects) {
        bottom_structures.emplace_back(object->model_->blas_);
    }
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    for (const auto object : objects) {
        VkTransformMatrixKHR transform{};
        memcpy(&transform, &object->transform_, sizeof(VkTransformMatrixKHR));
        instances.emplace_back(transform, 0, 0xFF, 0, 0,
                               object->model_->blas_->get_device_address());
    }
    std::shared_ptr<TLAS> tlas = std::make_shared<TLAS>(
        context->gpu_allocator_, context->command_pool_, context->ring_buffer_,
        bottom_structures, instances);
    return std::make_shared<GraphicsScene>(tlas, objects);
}
