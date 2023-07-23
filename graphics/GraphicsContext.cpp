#include "GraphicsContext.h"
#include "Window.h"

struct GraphicsContext {
    void create() {
	valid_ = true;
	window_ = std::make_unique<Window>();
    }

    void destroy() {
	valid_ = false;
	window_ = nullptr;
    }

    bool valid_ = false;

    std::unique_ptr<Window> window_ = nullptr;
};

std::shared_ptr<GraphicsContext> createGraphicsContext() {
    auto context = std::make_shared<GraphicsContext>();
    context->create();
    return context;
}

void renderFrame(std::shared_ptr<GraphicsContext> context) {
    context->window_->pollEvents();
}

bool shouldExit(std::shared_ptr<GraphicsContext> context) {
    return context->window_->shouldClose();
}

void destroyGraphicsContext(std::shared_ptr<GraphicsContext> context) {
    context->destroy();
}
