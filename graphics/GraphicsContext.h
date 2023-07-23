#pragma once

#include <memory>

struct GraphicsContext;

std::shared_ptr<GraphicsContext> createGraphicsContext();

void renderFrame(std::shared_ptr<GraphicsContext>);

bool shouldExit(std::shared_ptr<GraphicsContext>);

void destroyGraphicsContext(std::shared_ptr<GraphicsContext>);
