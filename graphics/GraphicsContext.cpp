#include "GraphicsContext.h"
#include "Window.h"
#include "Device.h"
#include "GPUAllocator.h"
#include "Swapchain.h"
#include "Command.h"
#include "Pipeline.h"

class GraphicsContext {
public:
    GraphicsContext();
private:
    std::shared_ptr<Window> window_ = nullptr;
    std::shared_ptr<Device> device_ = nullptr;
    std::shared_ptr<GPUAllocator> gpu_allocator_ = nullptr;
    std::shared_ptr<Swapchain> swapchain_ = nullptr;
    std::shared_ptr<CommandPool> command_pool_ = nullptr;
    std::shared_ptr<RayTracePipeline> ray_trace_pipeline_ = nullptr;

    friend std::shared_ptr<GraphicsContext> createGraphicsContext();
    friend void renderFrame(std::shared_ptr<GraphicsContext>);
    friend bool shouldExit(std::shared_ptr<GraphicsContext>);
};

GraphicsContext::GraphicsContext() {
    window_ = std::make_shared<Window>();
    device_ = std::make_shared<Device>(window_);
    gpu_allocator_ = std::make_shared<GPUAllocator>(device_);

    auto buffer = GPUBuffer(gpu_allocator_, 1024, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    swapchain_ = std::make_shared<Swapchain>(device_, window_);
    command_pool_ = std::make_shared<CommandPool>(device_);

    auto shader = std::make_shared<Shader>(device_, "dumb_rgen");
    std::vector<std::vector<std::shared_ptr<Shader>>> shader_groups = {{shader}};
    ray_trace_pipeline_ = std::make_shared<RayTracePipeline>(device_, shader_groups);
}

std::shared_ptr<GraphicsContext> createGraphicsContext() {
    auto context = std::shared_ptr<GraphicsContext>(new GraphicsContext());
    return context;
}

void renderFrame(std::shared_ptr<GraphicsContext> context) {
    context->window_->pollEvents();
}

bool shouldExit(std::shared_ptr<GraphicsContext> context) {
    return context->window_->shouldClose();
}
