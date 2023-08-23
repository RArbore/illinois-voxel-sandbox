#include "GraphicsContext.h"
#include "Window.h"
#include "Device.h"
#include "GPUAllocator.h"
#include "Swapchain.h"

class GraphicsContext {
public:
    GraphicsContext();
private:
    std::shared_ptr<Window> window_ = nullptr;
    std::shared_ptr<Device> device_ = nullptr;
    std::shared_ptr<GPUAllocator> gpu_allocator_ = nullptr;
    std::shared_ptr<Swapchain> swapchain_ = nullptr;

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
