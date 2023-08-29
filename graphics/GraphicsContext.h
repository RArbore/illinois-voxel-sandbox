#pragma once

#include <memory>
#include <vector>

#include <voxels/VoxelChunk.h>

class GraphicsContext;
class GraphicsModel;
class GraphicsScene;

std::shared_ptr<GraphicsContext> createGraphicsContext();

void renderFrame(std::shared_ptr<GraphicsContext>);

bool shouldExit(std::shared_ptr<GraphicsContext>);

std::shared_ptr<GraphicsModel> add_model(std::shared_ptr<GraphicsContext>, const VoxelChunk &chunk);

std::shared_ptr<GraphicsScene> build_scene(std::shared_ptr<GraphicsContext>, const std::vector<std::shared_ptr<GraphicsModel>> &models);
