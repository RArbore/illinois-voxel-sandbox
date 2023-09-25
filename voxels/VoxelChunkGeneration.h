#pragma once

#include "Voxel.h"

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
std::vector<std::vector<std::byte>> load_vox_scene(std::string filepath,
                                                   auto context);