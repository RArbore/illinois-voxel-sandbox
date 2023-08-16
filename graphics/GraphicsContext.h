#pragma once

#include <memory>

class GraphicsContext;

std::shared_ptr<GraphicsContext> createGraphicsContext();

void renderFrame(std::shared_ptr<GraphicsContext>);

bool shouldExit(std::shared_ptr<GraphicsContext>);
