#pragma once

#include "Voxel.h"
#include <graphics/GraphicsContext.h>

std::vector<std::byte> generate_basic_sphere_chunk(uint32_t width,
                                                   uint32_t height,
                                                   uint32_t depth,
                                                   float radius);
std::vector<std::byte>
generate_basic_filled_chunk(uint32_t width, uint32_t height, uint32_t depth);
std::vector<std::byte> generate_basic_procedural_chunk(uint32_t width,
                                                       uint32_t height,
                                                       uint32_t depth);
double densityfunction_(const glm::vec3 &position);
std::vector<std::byte> generate_terrain(uint32_t width, uint32_t height,
                                        uint32_t depth, float density);

std::shared_ptr<GraphicsScene>
test_loader(const std::string &filepath, ChunkManager &chunk_manager,
            std::shared_ptr<GraphicsContext> context);
std::shared_ptr<GraphicsScene>
load_vox_scene(const std::string &filepath, ChunkManager &chunk_manager,
               std::shared_ptr<GraphicsContext> context);
