#pragma once

#include <vector>

#include "RawVoxelChunk.h"
#include "VoxelChunk.h"

class VoxelManager {
  public:
    VoxelManager() = default;
    ~VoxelManager();

    RawVoxelChunk *generate_test_cube(const glm::vec3 &position);
    RawVoxelChunk *generate_sphere_chunk(const glm::vec3 &position, const glm::vec3 &size, const int radius);
    RawVoxelChunk *generate_procedural_chunk(const glm::vec3 &position, const glm::vec3 &size);
  private:
    std::vector<VoxelChunk *> chunks_;
    double densityfunction_(const glm::vec3 &position);
};
