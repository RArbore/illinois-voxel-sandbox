#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <voxels/Voxel.h>

#include <external/glm/glm/glm.hpp>

#include "Camera.h"

class GraphicsContext;
class GraphicsModel;
class GraphicsObject;
class GraphicsScene;

class VoxelChunk;
class VoxelChunkPtr;

std::shared_ptr<GraphicsContext> create_graphics_context(std::shared_ptr<Window> window);

void render_frame(std::shared_ptr<GraphicsContext> context,
                  std::shared_ptr<GraphicsScene> scene,
                  CameraUB camera_info);

std::shared_ptr<GraphicsModel>
build_model(std::shared_ptr<GraphicsContext> context, VoxelChunkPtr chunk);

std::shared_ptr<GraphicsObject>
build_object(std::shared_ptr<GraphicsContext> context,
             std::shared_ptr<GraphicsModel> model,
             const glm::mat3x4 &transform);

std::shared_ptr<GraphicsScene>
build_scene(std::shared_ptr<GraphicsContext> context,
            const std::vector<std::shared_ptr<GraphicsObject>> &objects);
