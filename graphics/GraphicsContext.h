#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <voxels/VoxelChunk.h>

class GraphicsContext;
class GraphicsModel;
class GraphicsScene;

std::shared_ptr<GraphicsContext> create_graphics_context();

void render_frame(std::shared_ptr<GraphicsContext> context, std::shared_ptr<GraphicsScene> scene);

bool should_exit(std::shared_ptr<GraphicsContext> context);

std::shared_ptr<GraphicsModel> build_models(std::shared_ptr<GraphicsContext> context, const VoxelChunk &chunk);

std::shared_ptr<GraphicsScene> build_scene(std::shared_ptr<GraphicsContext> context, const std::vector<std::shared_ptr<GraphicsModel>> &models);
