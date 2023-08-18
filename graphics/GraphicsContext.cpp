#include "GraphicsContext.h"
#include "Window.h"
#include "Device.h"
#include "GPUAllocator.h"

class GraphicsContext {
public:
    GraphicsContext();
private:
    std::shared_ptr<Window> window_;
    std::shared_ptr<Device> device_;
    std::shared_ptr<GPUAllocator> gpu_allocator_;

    friend std::shared_ptr<GraphicsContext> createGraphicsContext();
    friend void renderFrame(std::shared_ptr<GraphicsContext>);
    friend bool shouldExit(std::shared_ptr<GraphicsContext>);
};

GraphicsContext::GraphicsContext() {
    window_ = std::make_shared<Window>();
    device_ = std::make_shared<Device>(window_);
    gpu_allocator_ = std::make_shared<GPUAllocator>(device_);
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
